# weasel-neural

把青简（[qingjian](https://github.com/qingjian-team/qingjian)）的**含章·通变**整句模型嵌进
weasel 的 Rust 模型组件。现已接入服务端，DLL 随安装包提供。

当前功能、安装和配置见[仓库首页](../README.md)。下文保留原型评测与 C ABI 说明。

## 这是什么

含章·通变是一个 GPT-2 风格的**字级 Transformer**，做的是 P2C（pinyin-to-chinese）：
给一段连续拼音，直接生成整句汉字，不经过词图。序列是
`[带噪拼音] <sep> [汉字] <eos>`，解码就是喂进拼音逐字生成到 `<eos>`，束宽可调。

它和 weasel 现在用的 `script_translator`（词图 + 词频）是**互补**的，不是替代关系 ——
青简的配对实验里，两边都对 35.4%，只有生成对 16.5%，只有重排对 6.9%。

青简自己的评测（2081 句，行话密集的技术文本，看相对增益）：

| 做法 | 整句首选 | 字准确率 |
|---|---|---|
| 词图（≈ weasel 现状） | 42.3% | 78.5% |
| P2C 自由生成 | **51.9%** | **83.0%** |

## 阶段 0 的结论

| 项目 | 结果 |
|---|---|
| 能否脱离 `qingjian-core` 独立编译 | ✅ 只需 vendor `hanzhang/` 9 个文件（~1600 行）+ `qingjian-format` |
| 真实模型能否跑通 | ✅ `jintiantianqihenhao` → 今天天气很好（首选） |
| cdylib 体积 | ✅ **2.38 MB**（x64）/ 2.15 MB（x86）—— 不是担心的几十 MB |
| C ABI 从 C 调用 | ✅ 见 `proto/c-abi-smoke.c`，含错误路径与 NULL 安全 |
| 模型加载 | 194 ms（Windows）/ 330 ms（Linux），后台加载不进按键路径 |
| 推理延迟 | 见下表 |
| 跨平台 | ✅ Windows x64 **与** x86 都在 CI 里编过、跑过真实推理，见 [RESULTS-windows.md](RESULTS-windows.md) |
| CRT 静态链接 | ✅ `-C target-feature=+crt-static`，与 weasel 的 `/MT` 一致 |

### 延迟

Windows 实测（GitHub Actions `windows-latest`，AMD EPYC 7763，见 [RESULTS-windows.md](RESULTS-windows.md)）：

| 输入 | 字母数 | Windows | Linux/WSL |
|---|---|---|---|
| `j` | 1 | 20.5 ms | 55 ms |
| `shurukuang` | 10 | 51.0 ms | 76 ms |
| `jintiantianqihenhao` | 19 | 108–110 ms | 123 ms |
| `womenjintianyiqiqugongyuanwanyiba` | 31 | 232 ms | 222 ms |

逐键平均 **62.9 ms** / 最慢 110 ms（Linux 上是 82.5 / 123.6）。
模型加载 194 ms（Linux 330 ms）。
束宽影响很小（beam 1 → 8 只从 117 到 136 ms），
瓶颈是**每层十几个小算子的调度开销**，不是矩阵乘 —— 这也是青简自己文档里的结论。

**结论：`RimeProcessKey` 里绝对不能调。** 必须走「停键防抖 → 后台线程 → 结果回来再刷候选」。

## 怎么用

### Linux（本机验证）

```bash
cargo build --release

# 一次转换
./target/release/p2c-check <model.qjm> jintiantianqihenhao

# 逐键延迟
./target/release/p2c-typing <model.qjm> jintiantianqihenhao 5

# C ABI 冒烟
gcc -O2 -o proto/c-abi-smoke proto/c-abi-smoke.c \
    -Ltarget/release -lweasel_p2c -Wl,-rpath,'$ORIGIN/../target/release'
./proto/c-abi-smoke <model.qjm> jintiantianqihenhao
```

### Windows

普通用户直接安装新版 Weasel，不用单独下载 DLL。
安装包由 `ci.yml` 构建，并包含匹配服务端架构的运行库。
`neural.yml` 只负责 C ABI、真实模型与延迟验证。

要在本机编：

```bat
rustup target add x86_64-pc-windows-msvc i686-pc-windows-msvc
cargo build --release --locked --target x86_64-pc-windows-msvc
:: 产物：target\x86_64-pc-windows-msvc\release\weasel_p2c.dll
```

`.cargo/config.toml` 里已经配好 `+crt-static`，与 weasel 的 `/MT` 对齐。
`onig_sys`（由 `candle-core` → `tokenizers` 引入）需要 MSVC 编一段 C，
在 WSL 里交叉编译编不出来，所以这条路走 CI。

### 模型从哪来

模型不在本仓库。青简的 `data-v9` 发布包里有：

```
https://github.com/qingjian-team/qingjian/releases/download/data-v9/qingjian-data.tar.gz
  └─ data/models/hanzhang-tongbian/hanzhang-tongbian-small.qjm   (45 MB, SHA-256 见 tools/release/data.lock)
```

## C ABI

见 `include/p2c.h`，六个函数：

```c
P2cHandle*  p2c_load(const char* model_path);            // 0.3~3 s，别放按键路径
void        p2c_free(P2cHandle*);
int         p2c_convert(handle, keys, beam, max_chars, &cands);  // 80~200 ms
void        p2c_candidates_free(cands, n);
int         p2c_knows(handle, text);                     // 字表外的字模型算不准
const char* p2c_last_error(void);
```

## 目录

```
src/lib.rs              C ABI + 安全的 Rust 封装
src/hanzhang/           vendor 自 qingjian-neural/src/hanzhang/（去掉 core_scorer.rs）
src/format/             vendor 自 qingjian-format（只用来读 .qjm 容器）
src/bin/p2c-check.rs    单次转换 + 延迟
src/bin/p2c-typing.rs   逐键延迟曲线
proto/c-abi-smoke.c     C 侧调用验证
include/p2c.h           给 weasel 的头文件
```

vendor 的源码除了改 `qingjian_format::` → `crate::format::` 与删掉 `core_scorer.rs`，
其余原样保留，便于日后与上游对账。

## 下一步（阶段 1 / 2）

**阶段 1 — 接到 weasel（生成）**

1. `WeaselServer` 侧起后台线程：停键 ~80 ms 后用 `RimeGetContext` 拿到的原始输入串调
   `p2c_convert`，结果进缓存
2. 结果回来 `_UpdateUI`，把整句候选插到候选列表**前面**（不是替换，词候选要保留）
3. **索引映射**：视觉顺序 ≠ librime 顺序。数字键 / 空格目前是透传 `process_key`，
   要改成显式 `SelectCandidateOnCurrentPage(real_index)`，否则会选错候选
4. 长输入才调用（青简的经验：6 音节起）；短输入交给词图
5. 翻页或动过高亮就不刷（青简同策略）

**阶段 2 — 知意（重排）**

知意是 BERT 类 encoder + 选词头，Windows 上走 **int8 ONNX + ONNX Runtime**（70 MB），
纯 C++ 就能接，不需要 Rust。它读光标前文给候选重排，延迟只 ~62 ms，比重排路径更适合
做成 librime `Filter` 插件。许可证上要跟作者确认才能再分发。

**性能优化（阶段 1 顺带做）**

- **前缀 KV 缓存**：`P2c::convert` 每次都重算整段拼音的 prefix。连续输入时前一个键的
  prefix 是后一个的前缀，可以复用 —— 青简的 `CharLm::prefix_cache` 就是干这个的
- 模型加载（330 ms）挪到启动时的后台线程

## 许可

- 本工程代码：GPL-3.0-or-later（与 weasel、qingjian 一致）
- `src/hanzhang/`、`src/format/`：来自 qingjian，GPL-3.0-or-later
- 模型权重（`.qjm`）：青简自训，**权重许可未单独声明**。自用无碍；要随 weasel 分发
  必须先跟青简作者确认
