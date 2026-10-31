# 长句模型移植审查与改造方案

日期：2026-10-31。依据：本地源码、个人 Rime 配置、截图及本地真实模型推理。本次只分析与制定方案，不修改输入法行为、不部署 Windows。

## 1. 结论与验证边界

- 当前移植的是含章·通变 P2C **整句生成**，不是知意神经重排。Rust C ABI 明确只提供拼音生成；C++ 将生成的第一条句子插入 librime 候选之前。
- 个人方案另有 `wanxiang-lts-zh-hans.gram`，通过 `grammar`、`translator/contextual_suggestions` 参与传统组句；不能将它等同于知意重排。
- 截图第一条极可能就是生成结果。本地使用缓存的真实模型、beam=5，对 `danshichangduihuayinweimeiyouhuancunyouhui` 推理，首选恰为“但市场对话因为没有缓存优惠”，与截图一致。第二条来源仍需 Windows 日志确认。
- 不能仅凭截图证明 Windows 正在运行哪个二进制或模型。当前 Linux 可验证模型输出与静态代码问题，无法复现微信中的窗口移动或 TSF 崩溃。
- 两份 handoff 有版本差异，仓库内版本较新，但自身仍残留“已编过”和“未提交、待验证”的矛盾。编译记录不能视为完整行为验证。

## 2. 当前项目分层

| 位置 | 职责 | 当前状态 |
|---|---|---|
| `qingjian/` | 原始输入法与参考实现：Core 解析/词图/候选/学习，neural 提供生成和打分，Windows Server 管异步与候选布局 | 参考来源，未整体移植 |
| 顶层 `weasel-neural/` | Rust 独立原型，vendor 含章与容器读取，输出 C ABI 动态库、验证工具 | 源码与仓库内副本一致，文档和结果文件不同，有本地 target |
| `weasel/weasel-neural/` | weasel 仓库内的 Rust 模型组件，CI 使用此路径 | 建议作为唯一维护源 |
| `weasel/RimeWithWeasel/` | 调 librime；P2cEngine 动态加载模型 DLL、后台推理；handler 合并候选、拦截提交 | 原型集成，状态与索引存在缺陷 |
| `weasel/WeaselIPC*` | 前端与 Server 请求响应协议，新增 FetchContext 和 p2c.pending | 已接线，缺结果版本与完整状态 |
| `weasel/WeaselTSF/` | 应用内 TSF 组合文本、候选 UI、拉取异步候选 | 已接线，定时器与刷新时序有缺陷 |
| `weasel/WeaselUI/` | 绘制、布局、鼠标事件 | 仍按原候选序号发事件，未完整适配外部候选 |
| `Rime/` | 用户方案、词典、Lua、gram、样式；`build/` 是部署产物 | 当前记录选择 rime_ice；横排、行内预编辑 |

现有数据流：

```text
应用按键 → TSF → IPC → WeaselServer / RimeWithWeasel → librime
                                                    ↓ 原始输入
                                             P2cEngine 后台线程
                                                    ↓ C ABI
                                       Rust 含章·通变 → 整句列表
                                                    ↓ 缓存
TSF 定时 FetchContext ← IPC 响应注入第一条整句 ← handler
          ↓
      WeaselUI 候选窗
```

这条渐进移植路线可保留：不必先把整个青简 Core 搬进 weasel，也不必立刻换成 librime 插件。但生成候选必须成为有明确身份、生命周期的候选，不能仅在输出数组里临时插一条。

## 3. 已确认的实现缺陷

### P0：定时器 ID 被错误地当成对象地址

`WeaselTSF/CandidateList.cpp:468` 开始：`SetTimer(NULL, (UINT_PTR)this, ...)` 的返回值只判真假；回调却把 `id_event` 强转为 `WeaselTSF*`，停止时也用 this 调 KillTimer。

微软文档规定：hWnd=NULL 且指定 ID 不对应现有定时器时，传入 ID 被忽略，系统生成新 ID。因此这里存在非法指针访问、错误取消定时器的确定性 API 使用缺陷。不能只理解为对象销毁后的偶发悬垂。

参考：https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-settimer

建议：使用 TSF 线程拥有的消息窗口，通过窗口 userdata 定位实例，使用固定 timer ID；或保存实际返回 ID，并建立线程内 ID→实例映射。焦点切换、组句结束、Deactivate、析构均需注销并校验回调生命周期。

### P0：显示高亮与空格提交不一致

`_P2cInject` 把模型插在最前，随后 `cinfo.highlighted += 1`，保留原词库首选的视觉高亮；但 `session.highlight = 0`。空格/回车按 session.highlight 提交模型候选。

结果：画面蓝色标记在第二项，空格却可能提交第一项。截图蓝色标记的位置正符合这段代码。

建议确定统一规则：未交互时模型成为新首选，则显示与服务端均高亮模型；已开始导航则冻结当次候选视图，不再插入或改序。

### P0：只有部分键盘选词做了序号映射

`_P2cSelectionKey` 只处理空格、回车、1–9。`SelectCandidateOnCurrentPage` 和 `HighlightCandidateOnCurrentPage` 仍将视觉 index 原样交给 librime。

因此点击第一条模型候选会选中 librime 第一条；点击后续项整体错一位。方向导航、滚轮、翻页、自定义选字键、数字 0 也没有共享映射规则。不能把每个入口都简单减一，需要按当前视图确定候选身份。

建议引入候选视图映射：每项记录 source、候选 ID、librime 页内索引或生成文本。键盘、鼠标、滚轮、TSF 集成候选选择统一走一个分发入口；翻页语义仍由 librime 管理，当前页生成项作为附加项处理。

### P0：新输入先响应、后登记，可能发送旧结果

`ProcessKeyEvent` 顺序为 invalidate → librime process_key → `_Respond` → `_UpdateUI` → `_P2cSchedule`。invalidate 只改 active，不清 keys。`_P2cInject` 使用 session 旧 keys 查缓存，没有与当前 librime 原始输入再核对。

旧结果已存在时，下一次输入的响应可能重新注入旧句；随后 schedule 才改成新 keys 并清 active。这样屏幕、服务端选择映射和实际输入可能分属不同版本。

建议：先基于新 librime 状态更新输入版本和请求状态，再构造响应。注入与提交均要求 session、composition generation、raw input、caret、schema 和候选视图版本匹配；构造响应不修改会话状态。

### P1：pending 不是实际任务状态

`_P2cWaiting` 只要输入够长且尚未 active 就返回 true。没有结果、推理失败、与词库候选重复、已导航而不允许注入，都可能一直 pending。轮询每 60 ms 重画，计数耗尽之前持续产生无变化刷新。

建议状态为 Idle / Debouncing / Running / Ready / Empty / Failed / Cancelled；Ready 包括被去重的有效结果。pending 只表示实际在排队或执行；候选内容和版本未变时不重画；超时按当前请求的截止时间计算。

### P1：取消不完整，缺少组句生命周期版本

P2cEngine::Clear/ClearAll 只清 results，不取消待执行或正在执行请求。工作线程可在 clear 后重新写回。RemoveSession、FocusOut、ClearComposition 等入口未统一清理 P2C 会话。仅靠 keys 无法区分“相同拼音的下一次组句”。

建议取消时增加 generation；完成结果写入缓存和发送通知前，在同一锁内核对版本。会话移除时擦除状态；切换方案、焦点、组句结束时作废对应请求。单 worker 可保留，但全局 latest-request 策略需要明确只服务当前聚焦会话，避免其他会话被覆盖后永久等待。

### P1：异步刷新绕过定位流程，旧帧可能覆盖新帧

正常 DoEditSession 将候选放入 pendingUI，等 `_SetCompositionPosition` 获得位置后刷新。`_P2cPoll` 却直接 `_UpdateUI`，没有位置或版本屏障。旧 pendingUI 以后仍可被 flush，覆盖更新的模型候选。

这是候选闪回、尺寸/位置变化的代码风险；窗口具体移动还需要微信实机验证。横排中额外插入一条整句也会明显改变窗口宽度，触发屏幕边界避让。

建议按键和模型结果共用一个带版本的 UI 发布入口，先确定组合文本锚点再发布最新候选；相同组句内内容更新可沿用有效锚点，位置无效时请求新的 TSF extent。结束组句必须清 pendingUI，过期 extent 回调不能刷新新组句。

### P1：输入适用范围过宽

`_P2cCurrentKeys` 从整个 get_input 中只抽取 a–z，其他字符全部丢弃。未核对方案、光标位置、已经选定的前缀或当前未转换段。双拼也可能作为全拼送入模型；符号/英文可能被悄悄改成另一串输入。

建议第一版显式只支持 rime_ice 全拼、光标在末尾、未分段确认的完整组句。只允许明确的拼音分隔符归一化，其他字符按策略拒绝；后续再接双拼还原与分段前缀。12 字母只适合粗筛，不能等同于青简的完整音节数和末尾完整性判断。

## 4. 尚未移植的青简能力与模型效果限制

青简不仅有一个模型调用。`engine/query/generating.rs` 和 p2c_placement 测试还定义：

- 根据解析结果判断完整音节、末尾残缺、纠错、双拼与注音。
- 够长且完整的拼音可生成优先，短句或残缺输入优先保留词图。
- 生成与词图同文时复用词图候选并保留其读音与排序身份。
- 候选具有 Generated 等类型；生成不会被误认为普通词库序号。
- Windows rescore 层记录 navigated，用户交互后禁止异步改序；具备后台加载、超时与上下文路径。

当前 weasel 只取生成结果的第一条、丢弃分数，未移植上述融合、上下文打分和学习联动。直接 clear_composition 提交生成句也绕过正常候选选择学习路径，需要明确后续如何学习，避免伪装成已完成。

真实模型检查：

```text
输入：danshichangduihuayinweimeiyouhuancunyouhui
1. 但市场对话因为没有缓存优惠
2. 但市场对华因为没有缓存优惠
3. 但是常对话因为没有缓存优惠
4. 但市场对话因为没有缓存又会
5. 但市场对话因为没有缓存游会
```

此轮稳态推理约 624 ms，模型加载约 170 ms；只是当前 Linux 主机单样本，不代表 Windows 性能。旧二进制运行时有 GLIBC 弱版本警告，因此此结果用于确认文本复现，不作为正式性能基线。

“但是长对话”不在这五条中。修好选择框不能保证纠正语义；单独重排这五条也不可能恢复未生成的目标句。需要保留词图互补，随后用个人典型句集评估更宽候选、上下文打分或其他生成约束，而不是直接假定加知意就能解决。

## 5. 个人配置与工程整理

- `Rime/user.yaml` 当前方案是 rime_ice；源配置及 build 配置均启用了万象 gram 与 contextual_suggestions。
- `Rime/weasel.yaml`、custom 和 build 中未见 p2c 段。但 C++ enabled 缺省值是 true，并自动搜索安装目录和用户数据目录模型，所以没有配置不等于没启用。应增加明确开关、状态日志、模型与 DLL 版本/路径及加载失败原因。
- `hover_type: semi_hilitece` 拼写无效；代码只识别 none / semi_hilite / hilite。建议诊断时显式设 none，修复后再选需要的悬停行为；此拼写不能独立解释模型索引错误。
- 横排长句过宽，建议提供候选缩略或长句竖排样式供验证；不靠主题改动掩盖索引与定位缺陷。个人配置修改写 custom patch，不直接编辑 build 产物。
- handoff 中“FARPROC 是 stdcall，因此 cast 后 x86 必然栈不平衡”的推论不成立：调用约定由目标函数指针类型决定。当前 xmake 显式 /Gd，普通函数指针默认 cdecl；仍可显式 __cdecl 固定 ABI，并做 x86 动态加载测试。
- 统一维护 `weasel/weasel-neural/`，顶层原型待确认无独有源码后归档；本次已比较，源码未发现差异，handoff 与结果文件有差异。不要未经整理删除独有记录。
- 顶层 target 实测约 822 MB，可删除，重新编译会生成；缓存模型在 `.cache/qj/`，不在 target。需使用验证工具时可先单独保留可执行文件。此次未删除 target。
- Windows C++ 构建与 Rust DLL 发布目前分开，需要手动拼装；后续把 exe、匹配架构 DLL、模型元数据和诊断配置组合成可复现测试包。部署与更换输入法仍由用户在 Windows 上进行。

## 6. 推荐实施顺序

### 第一阶段：正确性修复，不新增模型

1. 修复 timer ownership/ID 与回调生命周期。
2. 引入 composition generation、request ID、view revision，调整“更新输入 → 更新请求状态 → 构造响应”的顺序。
3. 在 Server 候选协调层维护统一候选视图和 source 映射，统一键盘/鼠标/滚轮/翻页的选词与高亮。
4. 明确高亮规则：未交互时生成可为默认首选；已导航则冻结本轮视图，不异步插入。
5. 共用 TSF UI 发布与定位流程，丢弃旧响应和旧位置回调；仅在内容或位置改变时刷新。
6. 完整处理 cancelled / failed / empty / deduplicated 状态及焦点、方案、组句清理。

### 第二阶段：补齐生成融合策略

1. 先限制为全拼、末尾光标、整段未确认输入；增加完整性判断。
2. 将至多 2–3 条不同生成句与词图整句合并，去重并保留来源；数量可配置，横排优先限制展示宽度。
3. 保留分数用于诊断，但不直接与 librime quality 相加：两者没有共同标尺。
4. 建立个人句集，分别测词图、生成、融合的首选正确率、目标句覆盖率与延迟；包含本截图案例。
5. 根据实测选择生成优先门槛。生成错误时必须能稳定、准确地选回词图候选。

### 第三阶段：再决定重排与性能优化

1. 只有评测证明收益才引入知意或其他上下文打分；若需同时评价生成与词图候选，应作用于合并后的候选集。仅在 librime filter 内重排看不到 Server 外加的生成项，需要明确边界。
2. 后台加载、复用跨击键前缀缓存；先测单 worker 的实际延迟与内存，再决定优化。
3. 完善模型包、ABI 版本、DLL/模型标识与测试包；统一 README/handoff，注明源码版本和未做的 Windows 行为验证。

## 7. 验收条件

逻辑层用可控延迟的假模型验证：旧请求晚返回、同输入下一轮组句、Clear 后返回、空结果/失败/去重、用户导航后返回、切方案/焦点/会话销毁。断言“可见高亮候选 = 空格提交文本”，每种选择入口得到同一候选；pending 必须终结。

Windows 验证 x64/x86、msbuild/xmake；DLL 动态加载与 timer 回调都要运行验证，不能只编译。

微信与记事本实机分别检查：截图原句、普通整句、快速输入与回删、结果到达同时按空格、数字与鼠标点击、滚轮/方向键/翻页、行内预编辑换行、屏幕边缘、多显示器 DPI、切焦点、Esc/部署后返回。日志关联 request/view revision 与候选窗矩形，区分高亮跳动、候选重排和窗口坐标移动。

完成标准：选择与画面一致、旧结果不能覆盖新输入、用户交互后候选稳定、失败会停止轮询、同一组句的异步刷新不使用旧锚点。模型语义效果另按句集报告，不将 UI 修复等同于识别率提升。
