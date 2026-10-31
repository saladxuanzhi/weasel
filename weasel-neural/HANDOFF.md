# 交接文档：把含章·通变整句模型搬进 weasel

> 历史记录。当前功能和安装方式以根目录 README.md 为准。DLL 已纳入安装包，下文手动下载 DLL 的步骤不再适用。

> 写于额度耗尽前。下面每一条都是已经验证过的，照着做即可接上。

## 一句话现状

Rust 侧的模型封装（`weasel-neural`）**已完成并验证**；C++ 侧集成（阶段 1）**已在 CI 上把 xmake + msbuild 两个变体全部编过（run 41053582477, commit 87aa5f5）**，剩下的只有实机行为验证。

---

## 一、项目背景

- `/mnt/d/Rime/weasel` —— Rime 的 Windows 前端（C++，MSVC + xmake/msbuild 双构建）
- `/mnt/d/Rime/qingjian` —— 青简输入法（Rust），其「含章·通变」是字级 Transformer，做 P2C（拼音→整句）
- `/mnt/d/Rime/weasel-neural` —— 我为移植建的独立 Rust 原型工程（**已同步进 weasel 仓库的 `weasel-neural/` 子目录**）

用户诉求：把青简的整句模型移植到 weasel，改善长句输入。选定路线：
**先做注入验证原型 → 两个模型分两期（通变生成 = 阶段 1，知意重排 = 阶段 2）→ 自己用，不对外分发。**

---

## 二、已完成

### 2.1 weasel 追上上游（独立于本次功能）

- `master` 已 merge `rime/master`（54 commits），保留本地 `mark_bar_weight` 功能
- 合并 commit：`0f22959`，已把 `mark_bar_weight` 移植进上游新抽出的 `StandardLayout::_CalcMarkMetrics()`
- 备份分支：`backup/pre-catchup-20261030-1806`
- **重要**：本仓库 blob 存的是 CRLF，**不要设 `core.autocrlf`**，改文件时别破坏行尾（用 python 读写要按二进制处理）

### 2.2 weasel-neural 原型（阶段 0）—— 完成

位置：`weasel/weasel-neural/`，分支 `neural-p2c`

- vendor 了 `qingjian-neural/src/hanzhang/`（去掉了接 `qingjian-core` 的 `core_scorer.rs`）与 `.qjm` 容器读取
- 暴露 C ABI：`p2c_load / p2c_convert / p2c_free / p2c_candidates_free / p2c_knows / p2c_last_error`，头文件 `weasel-neural/include/p2c.h`
- 两个工具：`p2c-check`（单次转换+延迟）、`p2c-typing`（逐键延迟曲线）
- CI：`weasel-neural` 在 windows-latest 编 x64+x86、查导出表、用 MSVC 编纯 C 调用程序、跑真实模型

**实测数据（Windows / GitHub Actions）**

| 项目 | 数值 |
|---|---|
| cdylib 体积 | x64 2.38 MB / x86 2.15 MB |
| 模型加载 | 194 ms |
| 逐键延迟 | 平均 62.9 ms，最慢 110 ms |
| 1 字母 | 20.5 ms |
| 19 字母 | 108–110 ms |
| 31 字母 | 232 ms |

**结论：绝对不能在 `RimeProcessKey` 里同步调用，必须防抖 + 后台线程。**

模型文件（45 MB，不在仓库里）：

```
https://github.com/qingjian-team/qingjian/releases/download/data-v9/qingjian-data.tar.gz
  └─ data/models/hanzhang-tongbian/hanzhang-tongbian-small.qjm
sha256(qingjian-data.tar.gz) = bc8dd7f4e283c6b7fa0f38e493ea89e5d874ccf45ff22308ca52f4078df6694b
```

本地已缓存：`/mnt/d/Rime/.cache/qj/`

### 2.3 阶段 1：接进 weasel —— 代码完成，编译待最终确认

**架构关键发现**（这条决定了整个方案）：候选窗口画在 **TSF 进程**（`CCandidateList` 持有 `weasel::UI`），server 侧 `_UpdateUI` 传的是空 Context，所以 **server 没法主动推送给候选窗**。IPC 是纯请求-响应模型。

于是方案是：server 在响应里带 `p2c.pending=1` → TSF 起 60 ms 线程定时器 → 定时器发新命令 `WEASEL_IPC_FETCH_CONTEXT` 重新拉 context。

**改动的文件**

| 文件 | 内容 |
|---|---|
| `include/P2cEngine.h`（新） | 引擎封装接口 |
| `RimeWithWeasel/P2cEngine.cpp`（新） | 动态加载 dll、后台线程、防抖、结果缓存 |
| `RimeWithWeasel/RimeWithWeasel.cpp` | `_LoadP2cSettings/_P2cSchedule/_P2cClear/_P2cInvalidate/_P2cInject/_P2cSelectionKey/_P2cWaiting/_P2cTakeCommit/_P2cCurrentKeys/RefreshUI/FetchContext/OnP2cReady`，`ProcessKeyEvent` 拦截，`_Respond` 加 `p2c.pending` 与模型上屏 |
| `include/RimeWithWeasel.h` | 上述声明 + `P2cSession` 结构 |
| `include/WeaselIPC.h` | 新命令 `WEASEL_IPC_FETCH_CONTEXT`；`RequestHandler::FetchContext`、`SetUIRefreshCallback`；`Client::FetchContext` |
| `WeaselIPC/WeaselClientImpl.{h,cpp}` | `FetchContext()` |
| `WeaselIPCServer/WeaselServerImpl.{h,cpp}` | `OnFetchContext`、`WM_WEASEL_SERVICE_NOTIFY` 按 wParam==1 分流到 UI 刷新 |
| `WeaselServer/WeaselServerApp.cpp` | 接线两个回调 |
| `include/ResponseParser.h`、`WeaselIPC/ResponseParser.cpp` | 解析 `p2c.pending` |
| `WeaselTSF/EditSession.cpp` | 按 `p2c_pending` 起/停定时器 |
| `WeaselTSF/CandidateList.cpp` | `_ScheduleP2cPoll/_P2cPoll/_StopP2cPoll/_P2cTimerProc` |
| `WeaselTSF/WeaselTSF.{h,cpp}` | 声明 + Deactivate 里 `_StopP2cPoll()` |
| `output/data/weasel.yaml` | `p2c:` 配置段（enabled/model/debounce_ms/beam/min_keys） |
| `RimeWithWeasel/RimeWithWeasel.vcxproj{,.filters}` | **刚补上 `P2cEngine.cpp` 条目，未验证** |

**行为设计**

- 只有 ≥ `min_keys`(12) 个纯小写字母的输入才问模型；短输入照旧只按词库
- 模型给的**首选整句**插到候选列表最前面**一条**，不替换词候选；只在第 1 页且用户没动过高亮时插
- 选中模型那句时**直接上屏**（它多半不在 rime 候选表里），并 `clear_composition`
- 空格/回车/数字键会先按视觉顺序映射回 librime 序号；其他键一律先把注入撤掉再交给 rime，**保证不会选错**

---

## 三、当前阻塞 & 下一步

### 3.1 立刻要做的

**① ~~验证 vcxproj 修复~~ —— 已通过**

CI run `41053582477`（commit `87aa5f5`）结论 **success**，msbuild 与 xmake 两个变体都编过。

上一次 CI 失败在 msbuild 变体的链接阶段：

```
error LNK2001: unresolved external symbol "public: __cdecl weasel::P2cEngine::P2cEngine(void)"
  ... 10 个 P2cEngine 符号，来自 RimeWithWeasel.obj
```

原因：xmake 用 `add_files("./*.cpp")` 自动收录 `P2cEngine.cpp`，但 msbuild 的 `.vcxproj` 要手写。已补：

- `RimeWithWeasel/RimeWithWeasel.vcxproj`：`<ClCompile Include="P2cEngine.cpp" />`
- `RimeWithWeasel/RimeWithWeasel.vcxproj.filters`：对应条目 + `..\include\P2cEngine.h`

**这两个改动还没 commit，也没跑 CI。** 下一步：

```bash
cd /mnt/d/Rime/weasel
git add -A && git commit -m "fix(build): add P2cEngine.cpp to the msbuild project"
git push gh neural-p2c
```

然后盯 `https://github.com/saladxuanzhi/weasel/actions` 的 **CI**（不是 C++ check）。
CI 的 `Build Weasel` 要过，尤其 msbuild 变体。

**② 实机验证（用户自己做，不要代劳）**

> 用户明确要求：不要在 Windows 上代装、不要代换输入法。

产物下载地址（两个预发布 tag，产物可匿名下载）：

```
安装包  https://github.com/saladxuanzhi/weasel/releases/tag/p2c-test
原生库  https://github.com/saladxuanzhi/weasel/releases/tag/p2c-dll
```

步骤：

1. 装 `p2c-test` 里的 `weasel-*.exe`，覆盖当前 0.17.4
2. 确认安装目录（本机当前是 `C:\Program Files\Rime\weasel-0.17.4`）
3. 下载 `p2c-dll` 里的 `weasel_p2c_x64.dll`，**改名 `weasel_p2c.dll`**，放进安装目录（和 `WeaselServer.exe` 同级）
4. 取模型，同样放安装目录：

   ```
   https://github.com/qingjian-team/qingjian/releases/download/data-v9/qingjian-data.tar.gz
     └─ data/models/hanzhang-tongbian/hanzhang-tongbian-small.qjm   (46,141,208 B)
   sha256(qingjian-data.tar.gz) = bc8dd7f4e283c6b7fa0f38e493ea89e5d874ccf45ff22308ca52f4078df6694b
   ```

5. 编辑 `%APPDATA%\Rime\weasel.yaml`，把文件末尾的 `p2c:` 整段取消注释，设 `enabled: true`
6. 托盘右键 → 重新部署；或 `stop_service.bat` / `start_service.bat`
7. 验证：打 `jintiantianqihenhao` 停住 → 候选第一项应变成「今天天气很好」，按空格上屏
8. 回归：短输入、数字键选词、翻页、方向键移动高亮都不能错

日志：`%TEMP%\weasel.*.log`。引擎加载失败会走 `LOG(WARNING)` 分支，dll 或模型缺失时自动静默禁用，不会崩。

### 3.2 已知风险点（编译过了也要盯）

- **32 位**下 `GetProcAddress` 返回 `FARPROC`（`__stdcall`），而 Rust `extern "C"` 是 `__cdecl`，`reinterpret_cast` 能编但 32 位调用会栈不平衡。**x86 的 WeaselServer 大概率会崩**。修法：在 `P2cEngine.cpp` 里给函数指针显式加 `__cdecl`，或用 `__pragma` 包一层。x64 没有这个问题（统一调用约定）。
- **`SetTimer(NULL, (UINT_PTR)this, ...)`** 用指针当定时器 ID，实例销毁前必须 `KillTimer`（已在 `Deactivate` 加了 `_StopP2cPoll`），但若 TSF 对象被异常销毁仍有悬垂风险。
- **`_P2cInvalidate` 在每次非选择键调用**，会把 `active` 置 false；如果用户按方向键后 keys 没变，`_P2cSchedule` 里 `session.keys == keys` 会直接 return，模型候选不会再出现（这是有意为之，但值得确认体验）。
- **`rime_api->get_input`** 假定下载的 rime dev 版有该 API（xmake 编译已证明有）。

### 3.3 阶段 2（知意重排）

- 知意是 BERT 类 encoder + 选词头，Windows 上走 **int8 ONNX + ONNX Runtime**（70 MB），纯 C++ 即可，不需要 Rust
- 延迟只 ~62 ms（重排），比重排路径更适合做成 librime `Filter`
- 效果数据（青简自测）：词图 42.3% → 重排 42.3%（不涨），但和生成并集首选 58.9%
- **许可**：模型由 zhiyi_ime 作者授权，再分发要先跟作者确认。自己用无碍

### 3.4 可选优化

- **前缀 KV 缓存**：`P2c::convert` 每次重算整段拼音的 prefix，连续输入时可复用（青简的 `CharLm::prefix_cache` 就是干这个的）
- 模型加载 194 ms 移到启动时的后台线程（现在在 `Initialize()` 里同步做）

---

## 四、常用命令

```bash
# 本地
cd /mnt/d/Rime/weasel-neural && export PATH="$HOME/.cargo/bin:$PATH"
cargo build --release
./target/release/p2c-check  <model.qjm> jintiantianqihenhao
./target/release/p2c-typing <model.qjm> jintiantianqihenhao 5

# 推送（SSH 已配好，用户 saladxuanzhi）
cd /mnt/d/Rime/weasel && git push gh neural-p2c

# 看 CI 状态（匿名可查，日志需要 admin）
curl -s "https://api.github.com/repos/saladxuanzhi/weasel/actions/runs?per_page=3" \
  | python3 -c "import json,sys;[print(r['id'],r['name'],r['head_sha'][:7],r['status'],r['conclusion']) for r in json.load(sys.stdin)['workflow_runs']]"

# 拿编译日志（CI 会写回分支）
git fetch gh neural-p2c && git show gh/neural-p2c:weasel-neural/RESULTS-cpp.md
```

## 五、CI 说明

| workflow | 作用 | 触发 |
|---|---|---|
| `neural.yml` | 编 Rust、跑真实模型推理，结果写进 `weasel-neural/RESULTS-windows.md` | 改 `weasel-neural/**` |
| `cpp-check.yml` | 只编 x64+x86，日志写进 `weasel-neural/RESULTS-cpp.md` | 改 C++ 目录 |
| `ci.yml` | 上游的完整 CI（arm64/arm/x64/x86 + installer），会跑 msbuild + xmake 两个变体 | push 任意不含 `/` 的分支 |

> 分支名 **不能带 `/`**：`ci.yml` 的 `branches: ['*']` 不匹配 `feat/xxx`。
> 这就是为什么分支叫 `neural-p2c` 而不是 `feat/neural-p2c`。

> Actions 日志需要仓库管理员权限才能读，所以两个自制 workflow 都把结果 commit 回分支。
> GITHUB_TOKEN 的 push 不会递归触发 workflow，安全。

## 六、Git 状态快照

```
分支: neural-p2c（远程 gh = git@github.com:saladxuanzhi/weasel.git）
master: 0f22959（干净，未受本次功能影响）
未提交: RimeWithWeasel/RimeWithWeasel.vcxproj{,.filters} 的 P2cEngine 条目
```

## 七、许可

- weasel / qingjian 都是 GPL-3.0-or-later，代码兼容
- `weasel-neural/src/hanzhang/`、`src/format/` vendor 自 qingjian，GPL-3.0-or-later
- **模型权重许可未单独声明**，自用无碍，对外分发前必须找青简作者确认
- 知意模型需另找 zhiyi_ime 作者确认
