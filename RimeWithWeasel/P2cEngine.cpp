#include "stdafx.h"
#include <P2cEngine.h>

#include <chrono>
#include <condition_variable>
#include <cstring>
#include <map>
#include <mutex>
#include <thread>

// weasel_p2c.dll 的 C 接口，与 weasel-neural/include/p2c.h 一致。
// 这里刻意不 include 那个头文件：dll 是可选的，编 weasel 时不一定在旁边。
namespace {

typedef void P2cHandle;

struct P2cCandidate {
  char* text;
  double score;
};

std::string UTF16toUTF8(const std::wstring& w) {
  if (w.empty())
    return std::string();
  int len = ::WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr,
                                  0, nullptr, nullptr);
  std::string out(len, '\0');
  ::WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), &out[0], len,
                        nullptr, nullptr);
  return out;
}

std::wstring UTF8toUTF16(const char* s) {
  if (!s || !*s)
    return std::wstring();
  int len = ::MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
  if (len <= 1)
    return std::wstring();
  std::wstring out(len - 1, L'\0');
  ::MultiByteToWideChar(CP_UTF8, 0, s, -1, &out[0], len);
  return out;
}

}  // namespace

namespace weasel {

struct P2cEngine::Impl {
  // 一条算好的结果：与当时登记的 keys 绑在一起，输入一变就不再用
  struct Stored {
    std::string keys;
    std::vector<std::wstring> texts;
    unsigned long long seq = 0;
  };

  typedef const char* (__cdecl *LastErrorFn)(void);
  typedef P2cHandle* (__cdecl *LoadFn)(const char*);
  typedef void (__cdecl *FreeFn)(P2cHandle*);
  typedef int (__cdecl *ConvertFn)(P2cHandle*, const char*, int, int, P2cCandidate**);
  typedef void (__cdecl *FreeCandidatesFn)(P2cCandidate*, int);

  Config config;
  void* module = nullptr;
  P2cHandle* handle = nullptr;
  std::wstring error;

  LastErrorFn last_error = nullptr;
  LoadFn load = nullptr;
  FreeFn free = nullptr;
  ConvertFn convert = nullptr;
  FreeCandidatesFn free_candidates = nullptr;

  std::thread worker;
  mutable std::mutex mutex;
  std::condition_variable cv;
  bool stopping = false;

  // 待处理请求：同一会话以最后一次为准
  uintptr_t request_key = 0;
  std::string request_keys;
  unsigned long long request_seq = 0;
  unsigned long long done_seq = 0;

  std::map<uintptr_t, Stored> results;

  ReadyCallback ready = nullptr;
  void* ready_context = nullptr;
};

P2cEngine::P2cEngine() : m_impl(new Impl) {}

P2cEngine::~P2cEngine() {
  Stop();
}

bool P2cEngine::available() const {
  return m_impl->handle != nullptr;
}

std::wstring P2cEngine::error() const {
  return m_impl->error;
}

void P2cEngine::SetReadyCallback(ReadyCallback cb, void* context) {
  m_impl->ready = cb;
  m_impl->ready_context = context;
}

bool P2cEngine::WorthAsking(const std::string& keys) const {
  if (m_impl->handle == nullptr)
    return false;
  if ((int)keys.size() < m_impl->config.min_keys)
    return false;
  for (char c : keys) {
    if (c < 'a' || c > 'z')
      return false;
  }
  return true;
}

bool P2cEngine::Start(const Config& config) {
  Stop();
  m_impl->config = config;
  m_impl->error.clear();

  if (config.dll_path.empty() || config.model_path.empty()) {
    m_impl->error = L"没有配置 dll 或模型路径";
    return false;
  }

  HMODULE module = ::LoadLibraryW(config.dll_path.c_str());
  if (!module) {
    m_impl->error = L"加载 " + config.dll_path + L" 失败，错误码 " +
                    std::to_wstring(::GetLastError());
    return false;
  }
  m_impl->module = module;

  bool ok = true;
  auto bind = [&](auto& fn, const char* name) {
    fn = reinterpret_cast<std::decay_t<decltype(fn)>>(
        ::GetProcAddress(module, name));
    if (!fn) {
      m_impl->error = L"dll 里找不到 " + UTF8toUTF16(name);
      ok = false;
    }
  };
  bind(m_impl->last_error, "p2c_last_error");
  bind(m_impl->load, "p2c_load");
  bind(m_impl->free, "p2c_free");
  bind(m_impl->convert, "p2c_convert");
  bind(m_impl->free_candidates, "p2c_candidates_free");
  if (!ok) {
    Stop();
    return false;
  }

  std::string model_utf8 = UTF16toUTF8(config.model_path);
  m_impl->handle = m_impl->load(model_utf8.c_str());
  if (!m_impl->handle) {
    const char* reason = m_impl->last_error ? m_impl->last_error() : "";
    m_impl->error = L"加载模型失败：" + UTF8toUTF16(reason);
    Stop();
    return false;
  }

  m_impl->stopping = false;
  m_impl->done_seq = m_impl->request_seq;
  m_impl->worker = std::thread(&P2cEngine::_Worker, this);
  return true;
}

void P2cEngine::Stop() {
  if (m_impl->worker.joinable()) {
    {
      std::lock_guard<std::mutex> lock(m_impl->mutex);
      m_impl->stopping = true;
    }
    m_impl->cv.notify_all();
    m_impl->worker.join();
  }
  {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->results.clear();
  }
  if (m_impl->handle && m_impl->free)
    m_impl->free(m_impl->handle);
  m_impl->handle = nullptr;
  if (m_impl->module)
    ::FreeLibrary((HMODULE)m_impl->module);
  m_impl->module = nullptr;
  m_impl->last_error = nullptr;
  m_impl->load = nullptr;
  m_impl->free = nullptr;
  m_impl->convert = nullptr;
  m_impl->free_candidates = nullptr;
}

void P2cEngine::Request(uintptr_t session_key, const std::string& keys) {
  if (m_impl->handle == nullptr)
    return;
  {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->request_key = session_key;
    m_impl->request_keys = keys;
    ++m_impl->request_seq;
  }
  m_impl->cv.notify_one();
}

void P2cEngine::Clear(uintptr_t session_key) {
  std::lock_guard<std::mutex> lock(m_impl->mutex);
  m_impl->results.erase(session_key);
  if (m_impl->request_key == session_key) {
    ++m_impl->request_seq;
    m_impl->done_seq = m_impl->request_seq;
  }
}

void P2cEngine::ClearAll() {
  std::lock_guard<std::mutex> lock(m_impl->mutex);
  m_impl->results.clear();
  ++m_impl->request_seq;
  m_impl->done_seq = m_impl->request_seq;
}

bool P2cEngine::Result(uintptr_t session_key,
                       const std::string& keys,
                       std::vector<std::wstring>* texts) const {
  std::lock_guard<std::mutex> lock(m_impl->mutex);
  auto it = m_impl->results.find(session_key);
  if (it == m_impl->results.end() || it->second.keys != keys)
    return false;
  if (texts)
    *texts = it->second.texts;
  return true;
}

void P2cEngine::_Worker() {
  const int debounce =
      m_impl->config.debounce_ms > 0 ? m_impl->config.debounce_ms : 80;
  const int beam = m_impl->config.beam > 0 ? m_impl->config.beam : 5;

  for (;;) {
    uintptr_t key = 0;
    std::string keys;
    unsigned long long seq = 0;
    std::vector<std::wstring> texts;

    {
      std::unique_lock<std::mutex> lock(m_impl->mutex);
      m_impl->cv.wait(lock, [this] {
        return m_impl->stopping || m_impl->request_seq != m_impl->done_seq;
      });
      if (m_impl->stopping)
        return;

      // 防抖：这段时间里又来新请求就重新等
      seq = m_impl->request_seq;
      key = m_impl->request_key;
      keys = m_impl->request_keys;
      bool interrupted = m_impl->cv.wait_for(
          lock, std::chrono::milliseconds(debounce), [this, seq] {
            return m_impl->stopping || m_impl->request_seq != seq;
          });
      if (m_impl->stopping)
        return;
      if (interrupted)
        continue;

      // 到此为止没有新请求，这条可以算了
      m_impl->done_seq = seq;
      lock.unlock();

      P2cCandidate* cands = nullptr;
      int n = m_impl->convert(m_impl->handle, keys.c_str(), beam,
                              m_impl->config.max_chars, &cands);
      if (n > 0 && cands) {
        texts.reserve(n);
        for (int i = 0; i < n; ++i) {
          if (cands[i].text)
            texts.push_back(UTF8toUTF16(cands[i].text));
        }
        m_impl->free_candidates(cands, n);
      }

      // 算完才发现又敲了新键，结果直接丢，避免把过期的句子给出去
      lock.lock();
      if (m_impl->stopping)
        return;
      if (m_impl->request_seq != seq)
        continue;
    }

    Impl::Stored result;
    result.keys = keys;
    result.texts = std::move(texts);
    result.seq = seq;
    {
      std::lock_guard<std::mutex> lock(m_impl->mutex);
      if (m_impl->stopping || m_impl->request_seq != seq)
        continue;
      m_impl->results[key] = std::move(result);
    }
    if (m_impl->ready)
      m_impl->ready(key, m_impl->ready_context);
  }
}

}  // namespace weasel
