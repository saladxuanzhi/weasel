# weasel-neural

Weasel 的 Rust 本地推理组件。
推理代码来自 [qingjian / 青简](https://github.com/qingjian-team/qingjian)。
本目录不是需要单独安装的产品。

功能与配置见[仓库首页](../README.md)。
离线配置示例见 [rime-config](rime-config/README.md)。

## 职责

- 加载含章·通变模型。
- 将连续全拼转换为整句候选。
- 通过 C ABI 提供动态库接口。
- 提供单次转换、逐键延迟和 C ABI 验证工具。

防抖、后台调度、分段边界和候选合并由 C++ 服务端负责。
显示、定位和选词由 Weasel 前端处理。
本组件不包含知意重排，也不使用前文做语义排序。

## 构建与打包

安装包构建会自动调用根目录的 `build-p2c.ps1`。
它构建 x64、x86 DLL，并验证架构和校验信息。
普通用户无需单独下载或编译 DLL。

仅开发本组件时，可在本目录运行：

```sh
cargo build --release --locked
cargo test --locked
```

Windows 目标为 `x86_64-pc-windows-msvc` 和 `i686-pc-windows-msvc`。
静态 CRT 配置位于 `.cargo/config.toml`。

## 本地验证

自备兼容模型后，可运行：

```sh
./target/release/p2c-check /path/to/model.qjm jintiantianqihenhao
./target/release/p2c-typing /path/to/model.qjm jintiantianqihenhao 5
```

本仓库不托管或分发权重，也不提供下载直链。
模型说明及获取方式请查阅[青简项目](https://github.com/qingjian-team/qingjian)。

## C ABI

接口与内存所有权见 [include/p2c.h](include/p2c.h)。

| 接口 | 用途 |
|---|---|
| `p2c_load` | 加载模型，返回句柄 |
| `p2c_free` | 释放句柄 |
| `p2c_convert` | 拼音生成候选 |
| `p2c_candidates_free` | 释放候选数组及文本 |
| `p2c_knows` | 检查模型字表覆盖 |
| `p2c_last_error` | 获取当前线程的错误说明 |

候选文本使用 UTF-8。
数组必须由 `p2c_candidates_free` 释放。
加载和推理均应在按键处理线程之外执行。

## CI

- `ci.yml`：构建 DLL 和完整安装包。
- `neural.yml`：验证 Rust 编译、导出和 C ABI 测试程序。
- `cpp-check.yml`：快速检查 x64、x86 C++ 编译。

日志见 Actions 摘要和附件，不再提交回分支。
真实模型测试默认关闭。维护者可手动启用。
测试来源与 SHA-256 通过 Actions Secrets 配置。
测试权重不进入发布产物。

## 目录

| 路径 | 内容 |
|---|---|
| `src/lib.rs` | Rust 封装与 C ABI |
| `src/hanzhang/` | 含章模型推理 |
| `src/format/` | 模型容器读取 |
| `src/bin/` | 转换与计时工具 |
| `include/` | C 头文件 |
| `proto/` | C ABI 验证程序 |
| `rime-config/` | 用户配置示例与 Lua 组件 |

代码许可见根目录 [LICENSE.txt](../LICENSE.txt)。
权重许可及使用方式以青简项目说明为准。
