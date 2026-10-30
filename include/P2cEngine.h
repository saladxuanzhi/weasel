#pragma once
#include <memory>
#include <string>
#include <vector>

namespace weasel {

// 含章·通变整句模型（weasel_p2c.dll）的运行时封装。
//
// 用法是「登记请求 → 防抖 → 后台线程推理 → 主线程取结果」：
// 实测一次推理 20~230 ms，绝对不能放在处理按键的线程上，所以这里只负责登记，
// 结果什么时候好由工作线程决定，调用方事后用 Result() 取。
//
// 同一会话以最后一次 Request() 为准：打字比模型快时，中间那些按键的请求直接丢掉。
// dll 加载失败或模型不存在时，available() 为 false，所有接口退化成空操作，
// weasel 照旧只按词库出候选。
class P2cEngine {
 public:
  // 结果就绪的通知。在**工作线程**上调用，实现方必须自己切到 UI 线程
  // （weasel 走 WM_WEASEL_SERVICE_NOTIFY）。参数是会话标识。
  using ReadyCallback = void (*)(uintptr_t session_key, void* context);

  struct Config {
    std::wstring dll_path;
    std::wstring model_path;
    // 停键多久之后才推理。一次推理 20~230 ms，80 ms 已经能把连续击键合并掉。
    int debounce_ms = 80;
    int beam = 5;
    // 生成字数上限，0 表示按按键长度估算
    int max_chars = 0;
    // 少于这么多字母的输入不问模型：短输入词库给得更准，模型只会拖慢
    int min_keys = 12;
  };

  P2cEngine();
  ~P2cEngine();

  P2cEngine(const P2cEngine&) = delete;
  P2cEngine& operator=(const P2cEngine&) = delete;

  // 加载 dll 与模型、起工作线程。返回 false 时原因见 error()。
  bool Start(const Config& config);
  void Stop();

  bool available() const;
  std::wstring error() const;

  // 这段输入值不值得问模型（长度门槛 + 全是小写字母）。
  bool WorthAsking(const std::string& keys) const;

  // 登记一次请求，keys 是不带分隔的小写全拼。
  void Request(uintptr_t session_key, const std::string& keys);

  // 丢弃这个会话的请求与结果。
  void Clear(uintptr_t session_key);
  void ClearAll();

  // 取结果。只有结果确实对应当前这串 keys 时才返回 true。
  bool Result(uintptr_t session_key,
              const std::string& keys,
              std::vector<std::wstring>* texts) const;

  void SetReadyCallback(ReadyCallback cb, void* context);

 private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};

}  // namespace weasel
