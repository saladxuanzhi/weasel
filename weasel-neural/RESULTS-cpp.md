# x64 + x86 编译日志

由 `.github/workflows/cpp-check.yml` 生成，sha `b6b91e4c689610a09dca09b919e7292d6c428119`。

- build: `failure`

## 错误行（前 120 条，共 46 条）

```
[0m[1;31;1merror: [0mP2cEngine.cpp
RimeWithWeasel\P2cEngine.cpp(169): error C2039: '_Worker': is not a member of 'weasel::P2cEngine'
RimeWithWeasel\P2cEngine.cpp(169): error C2065: '_Worker': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(233): error C2039: '_Worker': is not a member of 'weasel::P2cEngine'
RimeWithWeasel\P2cEngine.cpp(235): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(235): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(234): error C2737: 'debounce': const object must be initialized
RimeWithWeasel\P2cEngine.cpp(236): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(236): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(236): error C2737: 'beam': const object must be initialized
RimeWithWeasel\P2cEngine.cpp(245): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(246): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(246): error C3482: 'this' can only be used as a lambda capture within a non-static member function
RimeWithWeasel\P2cEngine.cpp(247): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(247): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(247): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(249): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(253): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(254): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(255): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(256): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(257): error C3482: 'this' can only be used as a lambda capture within a non-static member function
RimeWithWeasel\P2cEngine.cpp(258): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(258): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(260): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(266): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(270): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(270): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(271): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(278): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(283): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(285): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(289): error C2653: 'Impl': is not a class or namespace name
RimeWithWeasel\P2cEngine.cpp(289): error C2065: 'Stored': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(289): error C2146: syntax error: missing ';' before identifier 'result'
RimeWithWeasel\P2cEngine.cpp(289): error C2065: 'result': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(290): error C2065: 'result': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(291): error C2065: 'result': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(292): error C2065: 'result': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(294): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(294): error C2512: 'std::lock_guard<std::mutex>::lock_guard': no appropriate default constructor available
RimeWithWeasel\P2cEngine.cpp(295): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(295): error C2065: 'result': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(297): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(298): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(298): error C2065: 'm_impl': undeclared identifier
```

## 日志尾部（200 行）

```

== xmake f -a x64 ==
[0mchecking for platform ... [32;1mwindows (x64)[0m
[0m[33;1mDetecting Visual Studio environment (cached); run `xmake global --clean` first if vs updated.[0m
[0mchecking for Microsoft C/C++ Compiler (x64) ... [32;1mok[0m
[0mchecking for Microsoft Visual Studio (x64) version ... [32;1m2022[0m
[0m[1;33;1mwarning: [33;1m.\WeaselDeployer\xmake.lua:7[0m: cannot match add_files("D:\a\weasel\weasel/PerMonitorHighDPIAware.manifest") in target(WeaselDeployer)[0m
[0m[1;33;1mwarning: [33;1madd -v for getting more warnings ..[0m
== xmake (x64) ==
[0m[32;1m[  8%]:[0m [36;1m<WeaselIPC>[0m compiling.release WeaselIPC\ActionLoader.cpp[0m
[0m[32;1m[  8%]:[0m [36;1m<WeaselIPC>[0m compiling.release WeaselIPC\Committer.cpp[0m
[0m[32;1m[  8%]:[0m [36;1m<WeaselIPC>[0m compiling.release WeaselIPC\Configurator.cpp[0m
[0m[32;1m[  8%]:[0m [36;1m<WeaselIPC>[0m compiling.release WeaselIPC\ContextUpdater.cpp[0m
[0m[32;1m[  8%]:[0m [36;1m<WeaselIPC>[0m compiling.release WeaselIPC\Deserializer.cpp[0m
[0m[32;1m[  8%]:[0m [36;1m<WeaselIPC>[0m compiling.release WeaselIPC\PipeChannel.cpp[0m
[0m[32;1m[  9%]:[0m [36;1m<WeaselIPC>[0m compiling.release WeaselIPC\ResponseParser.cpp[0m
[0m[32;1m[ 10%]:[0m [36;1m<WeaselIPC>[0m compiling.release WeaselIPC\stdafx.cpp[0m
[0m[32;1m[ 11%]:[0m [36;1m<WeaselIPC>[0m compiling.release WeaselIPC\Styler.cpp[0m
[0m[32;1m[ 12%]:[0m [36;1m<WeaselIPC>[0m compiling.release WeaselIPC\WeaselClientImpl.cpp[0m
[0m[32;1m[ 13%]:[0m [36;1m<RimeWithWeasel>[0m compiling.release RimeWithWeasel\P2cEngine.cpp[0m
[0m[32;1m[ 14%]:[0m [36;1m<RimeWithWeasel>[0m compiling.release RimeWithWeasel\RimeWithWeasel.cpp[0m
[0m[32;1m[ 16%]:[0m [36;1m<RimeWithWeasel>[0m compiling.release RimeWithWeasel\stdafx.cpp[0m
[0m[32;1m[ 17%]:[0m [36;1m<RimeWithWeasel>[0m compiling.release RimeWithWeasel\WeaselUtility.cpp[0m
[0m[32;1m[ 18%]:[0m [36;1m<WeaselUI>[0m compiling.release WeaselUI\d2d.cpp[0m
[0m[1;31;1merror: [0mP2cEngine.cpp
RimeWithWeasel\P2cEngine.cpp(169): error C2039: '_Worker': is not a member of 'weasel::P2cEngine'
D:\a\weasel\weasel\include\P2cEngine.h(17): note: see declaration of 'weasel::P2cEngine'
RimeWithWeasel\P2cEngine.cpp(169): error C2065: '_Worker': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(233): error C2039: '_Worker': is not a member of 'weasel::P2cEngine'
D:\a\weasel\weasel\include\P2cEngine.h(17): note: see declaration of 'weasel::P2cEngine'
RimeWithWeasel\P2cEngine.cpp(235): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(235): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(234): error C2737: 'debounce': const object must be initialized
RimeWithWeasel\P2cEngine.cpp(236): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(236): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(236): error C2737: 'beam': const object must be initialized
RimeWithWeasel\P2cEngine.cpp(245): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(246): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(246): error C3482: 'this' can only be used as a lambda capture within a non-static member function
RimeWithWeasel\P2cEngine.cpp(247): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(247): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(247): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(249): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(253): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(254): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(255): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(256): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(257): error C3482: 'this' can only be used as a lambda capture within a non-static member function
RimeWithWeasel\P2cEngine.cpp(258): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(258): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(260): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(266): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(270): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(270): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(271): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(278): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(283): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(285): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(289): error C2653: 'Impl': is not a class or namespace name
RimeWithWeasel\P2cEngine.cpp(289): error C2065: 'Stored': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(289): error C2146: syntax error: missing ';' before identifier 'result'
RimeWithWeasel\P2cEngine.cpp(289): error C2065: 'result': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(290): error C2065: 'result': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(291): error C2065: 'result': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(292): error C2065: 'result': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(294): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(294): error C2512: 'std::lock_guard<std::mutex>::lock_guard': no appropriate default constructor available
RimeWithWeasel\P2cEngine.cpp(294): note: while trying to match the argument list '()'
RimeWithWeasel\P2cEngine.cpp(295): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(295): error C2065: 'result': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(297): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(298): error C2065: 'm_impl': undeclared identifier
RimeWithWeasel\P2cEngine.cpp(298): error C2065: 'm_impl': undeclared identifier

  [33m> in [1mRimeWithWeasel\P2cEngine.cpp[0m
[0m[1;33;1mwarning: [33;1m.\WeaselDeployer\xmake.lua:7[0m: cannot match add_files("D:\a\weasel\weasel/PerMonitorHighDPIAware.manifest") in target(WeaselDeployer)[0m
[0m[1;33;1mwarning: [33;1madd -v for getting more warnings ..[0m
!! xmake (x64) 失败
```
