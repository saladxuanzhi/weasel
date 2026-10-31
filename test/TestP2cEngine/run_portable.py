"""Test the production worker with fake Win32 loading and a delayed C ABI model.

Run on Linux: python3 test/TestP2cEngine/run_portable.py
This checks cancellation/result handling, not Windows DLL or TSF behavior.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
shim = r'''
#pragma once
#include <cstdint>
#include <string>
#include <cwchar>
#include <type_traits>
#define __cdecl
#define CP_UTF8 65001
using HMODULE = void*;
using FARPROC = void(*)();
HMODULE LoadLibraryW(const wchar_t*);
FARPROC GetProcAddress(HMODULE, const char*);
inline unsigned long GetLastError() { return 0; }
inline void FreeLibrary(HMODULE) {}
inline int WideCharToMultiByte(int,int,const wchar_t* s,int n,char* out,int,void*,void*) {
  if(out) for(int i=0;i<n;++i) out[i]=static_cast<char>(s[i]);
  return n;
}
inline int MultiByteToWideChar(int,int,const char* s,int n,wchar_t* out,int) {
  if(n<0) n=static_cast<int>(std::char_traits<char>::length(s))+1;
  if(out) for(int i=0;i<n;++i) out[i]=s[i];
  return n;
}
'''
test = r'''
#include "shim.h"
#include "P2cEngine.h"
#include "P2cCandidateIndex.h"
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>
using namespace std::chrono_literals;
struct Candidate { char* text; double score; };
std::atomic<int> entered{0}, notified{0};
void* load(const char*) { return reinterpret_cast<void*>(1); }
void release(void*) {}
const char* error() { return ""; }
int convert(void*, const char* keys, int, int, Candidate** out) {
  ++entered;
  std::this_thread::sleep_for(60ms);
  if(std::strcmp(keys,"empty")==0) return 0;
  if(std::strcmp(keys,"error")==0) return -1;
  *out = new Candidate[1];
  (*out)[0].text = new char[std::strlen(keys)+1];
  std::strcpy((*out)[0].text,keys);
  return 1;
}
void freeCandidates(Candidate* c,int) { delete[] c[0].text; delete[] c; }
HMODULE LoadLibraryW(const wchar_t*) { return reinterpret_cast<void*>(1); }
FARPROC GetProcAddress(HMODULE,const char* name) {
  if(!std::strcmp(name,"p2c_load")) return reinterpret_cast<FARPROC>(load);
  if(!std::strcmp(name,"p2c_free")) return reinterpret_cast<FARPROC>(release);
  if(!std::strcmp(name,"p2c_last_error")) return reinterpret_cast<FARPROC>(error);
  if(!std::strcmp(name,"p2c_convert")) return reinterpret_cast<FARPROC>(convert);
  return reinterpret_cast<FARPROC>(freeCandidates);
}
template<class F> void waitFor(F predicate) {
  auto until=std::chrono::steady_clock::now()+2s;
  while(!predicate()) {
    assert(std::chrono::steady_clock::now()<until);
    std::this_thread::sleep_for(1ms);
  }
}
int main() {
  // Default view: dictionary first, generated second, remaining dictionary items.
  assert(weasel::P2cLibrimeIndex(0,1)==0);
  assert(weasel::P2cLibrimeIndex(1,1)==-1);
  assert(weasel::P2cLibrimeIndex(2,1)==1);
  // Opt-in generated-first view must also select the displayed item.
  assert(weasel::P2cLibrimeIndex(0,0)==-1);
  assert(weasel::P2cLibrimeIndex(1,0)==0);
  for(int position=0;position<=1;++position)
    for(int visual=position+1;visual<11;++visual)
      assert(weasel::P2cLibrimeIndex(visual,position)==visual-1);
  weasel::P2cEngine engine;
  weasel::P2cEngine::Config config;
  config.dll_path=L"fake"; config.model_path=L"fake"; config.debounce_ms=1;
  engine.SetReadyCallback([](uintptr_t,void*){++notified;},nullptr);
  assert(engine.Start(config));
  engine.Request(1,"same"); waitFor([]{return entered==1;});
  engine.Clear(1);
  std::this_thread::sleep_for(90ms);
  assert(!engine.Result(1,"same",nullptr) && notified==0);
  engine.Request(1,"same"); waitFor([]{return notified==1;});
  assert(engine.Result(1,"same",nullptr));
  engine.ClearAll();
  assert(!engine.Result(1,"same",nullptr));
  engine.Request(1,"old"); waitFor([]{return entered==3;});
  engine.Request(1,"new"); waitFor([]{return notified==2;});
  assert(!engine.Result(1,"old",nullptr));
  assert(engine.Result(1,"new",nullptr));
  engine.Request(1,"empty"); waitFor([]{return notified==3;});
  std::vector<std::wstring> texts;
  assert(engine.Result(1,"empty",&texts) && texts.empty());
  engine.Request(1,"error"); waitFor([]{return notified==4;});
  assert(engine.Result(1,"error",&texts) && texts.empty());
  engine.Stop();
  assert(!engine.available());
  std::cout<<"P2cEngine worker: all tests passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix="weasel-worker-") as directory:
    path = Path(directory)
    (path / "shim.h").write_text(shim)
    source = (root / "RimeWithWeasel/P2cEngine.cpp").read_text()
    (path / "engine.cpp").write_text(source.replace('#include "stdafx.h"', '#include "shim.h"'))
    (path / "test.cpp").write_text(test)
    subprocess.run(["g++", "-std=c++17", "-pthread", "-Wall", "-Wextra", "-Werror",
                    "-I", str(root / "include"), str(path / "engine.cpp"),
                    str(path / "test.cpp"), "-o", str(path / "test")], check=True)
    subprocess.run([str(path / "test")], check=True)
