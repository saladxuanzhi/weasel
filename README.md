# 小狼毫 · 本地长句生成版

[![Build](https://github.com/saladxuanzhi/weasel/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/saladxuanzhi/weasel/actions/workflows/ci.yml)

基于 [Rime Weasel](https://github.com/rime/weasel) 的个人分支。
接入青简的「含章·通变」模型，补充本地长句候选。
保留 Rime 的词库、方案和用户词频。

[下载安装包](https://github.com/saladxuanzhi/weasel/releases/tag/p2c-test) · [构建记录](https://github.com/saladxuanzhi/weasel/actions) · [反馈问题](https://github.com/saladxuanzhi/weasel/issues)

## 新增功能

- **本地长句生成**：连续全拼可直接生成汉字句子。推理无需联网。
- **后台推理**：停键后执行。防抖和过期请求取消，避免阻塞按键处理。
- **词库与生成并存**：默认保留词库首选。生成句放在第二项，也可设为第一项。
- **保护手动选择**：已选前缀不再交给模型。只生成剩余部分，提交时保留前缀。
- **生成项星标**：默认显示 `✦`。可改符号、放到注释中，或关闭。
- **来源诊断**：区分 Rime、通变生成，以及双方同文的候选。
- **前词搭配学习**：按上一段上屏文字，提升你选过的下一词。可独立关闭。
- **统一选词映射**：适配注入后的空格、数字、上下键、鼠标点击和高亮。
- **候选窗更新修复**：协调异步候选与 TSF 定位。补充定时器和失焦清理。
- **可调高亮条宽度**：通过 `mark_bar_weight` 调整。保留本分支图标。
- **DLL 随安装包提供**：自动安装匹配服务端架构的运行库，无需单独下载。

模型和词库各有优势。当前不按模型分数自动抢占首选。
模型分数与 Rime 权重不在同一尺度。
本仓库尚未接入知意神经重排。

## 安装

1. 从上方下载页获取最新安装包，并运行安装。
2. 获取同页的 `p2c-rime-config.zip`，用于合并可选配置。
3. 准备 `hanzhang-tongbian-small.qjm` 模型文件。
4. 将模型放入 Rime 用户目录。托盘菜单可打开该目录。
5. 按下面的教程合并配置，再重新部署并重启服务。

默认用户目录是 `%APPDATA%\Rime`。
自定义过路径时，以托盘打开的目录为准。
配置合并到已有文件，不要覆盖个人词库。

### 模型来源

本仓库不托管、不分发模型权重，也不提供权重下载直链。
请前往 [qingjian / 青简项目](https://github.com/qingjian-team/qingjian)，查阅模型说明与获取方式。
准备好兼容的 `hanzhang-tongbian-small.qjm` 后，按本页配置使用。

安装包包含 `weasel_p2c.dll`，不包含模型权重。
DLL 会装到 `WeaselServer.exe` 同目录。
安装器按服务端选择 x64 或 x86 版本。
ARM64 系统沿用现有安装器的服务端选择策略。

模型默认先从安装目录查找，再从用户目录查找。
找不到 DLL 或模型时，回退到普通 Rime 候选。
建议将模型放在用户目录，升级时可保留。

## 配置长句生成

编辑用户目录的 `weasel.custom.yaml`。
已有 `patch:` 时，只合并配置项，不要再写一个 `patch:`。

```yaml
patch:
  p2c/enabled: true
  p2c/debounce_ms: 80
  p2c/beam: 4
  p2c/min_keys: 12
  p2c/prefer_generated: false
  p2c/marker: "✦"
  p2c/marker_position: prefix
  p2c/show_source: false
```

| 配置 | 作用 |
|---|---|
| `enabled` | 开启生成。设为 `false` 可关闭 |
| `debounce_ms` | 停键多久后开始推理，单位为毫秒 |
| `beam` | 搜索宽度。界面只补充一条生成句 |
| `min_keys` | 剩余拼音的最少字母数 |
| `prefer_generated` | `false` 放第二项；`true` 优先放第一项 |
| `marker` | 星标内容。可用 `✦`、`✧`、`★` 或自定义文字 |
| `marker_position` | `prefix` 放前面；`comment` 放注释；`none` 关闭 |
| `show_source` | 显示详细来源，排查时可开启 |
| `model` | 可选的模型绝对路径 |

表中的配置名都位于 `p2c/` 下。
省略 `model` 即使用自动查找。
指定其他位置时，可添加：

```yaml
  p2c/model: 'D:\Models\hanzhang-tongbian-small.qjm'
```

星标仅用于显示，不会随候选文字上屏。
注释模式需要主题允许显示注释。
当前没有单独为生成项染色的配置。

开启 `show_source` 后：

| 标记 | 来源 |
|---|---|
| `〔Rime〕` | Rime 候选流，包含 Lua 调整 |
| `〔通变生成〕` | 额外插入的生成句 |
| `〔Rime〕〔通变同文〕` | 模型首选与该词库候选相同 |

同文结果会去重，并保留原排名。
所以即使开启生成优先，也不会强行提升已有的同文候选。

### 输入边界

当前按连续小写全拼处理，不支持将双拼按键直接交给模型。
输入与预编辑无法对齐时，不调用生成。
光标在中间时，也保留普通候选。

手动选定前半句后，只计算剩余拼音。
例如只剩 `cha` 时，不满足默认的 12 字母门槛。
这时正常显示字词候选，不生成整句。

## 配置前词搭配学习

此功能用于 rime_ice 方案，与 P2C 开关独立。
它学习个人搭配，不是神经语义推理。

1. 从配置包复制 `lua/contextual_pair.lua`。
2. 放入用户目录的 `lua/` 文件夹。
3. 新建或合并用户目录的 `rime_ice.custom.yaml`：

```yaml
patch:
  contextual_pair/enabled: true
  contextual_pair/timeout_seconds: 60
  contextual_pair/candidate_limit: 24
  engine/filters/@before 4: lua_filter@*contextual_pair
```

必须先安装 Lua 文件，再加入过滤器配置。
`@before 4` 对应本仓库测试的 rime_ice 过滤器顺序。
若你修改过该列表，请将它放在 `pin_cand_filter` 之前。

例如先上屏“部署”，再输入 `moxing` 并选择“模型”。
下次在“部署”后输入相同拼音，“模型”会获得优先级。
它必须已在前 24 个候选中，且覆盖相同输入范围。
显式置顶规则仍优先。

这里的“前词”是上次完整上屏的中文片段，最多 32 字。
超过 60 秒，或上屏标点、英文后，会清除当前前文。
它不读取应用内旧文档，也不感知应用中的光标移动。
新安装没有搭配记录时，保留原顺序。

设 `contextual_pair/enabled: false` 可停止读取和学习。
记录保存在用户目录的 `contextual_pair.tsv`。
停止服务后删除该文件，可清除已学搭配。

## 配置高亮条

同样写入 `weasel.custom.yaml` 的 `patch:` 下：

```yaml
  style/layout/mark_bar_weight: 4
```

数值用于调整候选高亮条的宽度。

## 应用配置与排错

修改后，在托盘菜单执行“重新部署”，再重启小狼毫服务。
不要直接编辑 `build/` 中的生成文件。
安装新版后，应确认旧服务已退出。

如果放入 custom 文件后只剩英文字母，先检查 Lua 文件是否齐全。
可暂时把 `rime_ice.custom.yaml` 改名为 `.yaml.bak`，再重新部署。
排查日志时，查看 `%TEMP%` 中的 `weasel` 和 `rime.weasel` 文件。

生成句不一定更准确。可同时保留词库和生成结果。
本功能无需万象 gram，也不会自动删除你原有的其他模型。

## 构建与发布

主分支为 `master`。推送后由 GitHub Actions 自动构建。
只有主分支更新下载页，其他分支提供 Actions 产物。
同一提交先生成 x64、x86 P2C DLL，再构建 msbuild 和 xmake 安装包。
打包前检查 DLL 架构、源提交和 SHA-256。
缺少运行库时，安装包构建会失败。

安装包和配置包发布到 [p2c-test](https://github.com/saladxuanzhi/weasel/releases/tag/p2c-test)。
也可从 Actions 下载产物。
DLL 不再单独发布。

本地构建沿用 [INSTALL.md](INSTALL.md)。
打安装包时还需要 Rust、Cargo 和 MSVC C++ 工具链。
`build.bat arm64 installer` 或 `xbuild.bat arm64 installer` 会自动编译 P2C DLL。
无需先运行独立的神经模型 workflow。
`weasel-neural/` 是源码中的 Rust 模块，无需单独安装。

本地可运行的检查：

```sh
python3 test/TestP2cEngine/run_portable.py
# Packaging checks require PowerShell (pwsh).
python3 test/TestP2cPackage/run_staging_checks.py
```

该测试检查后台任务和输入边界。
不能替代 Windows 编译、真实 DLL 加载和应用内测试。

## 致谢与许可

- 感谢 [Rime / Weasel](https://github.com/rime/weasel) 的全部作者、方案维护者、数据提供者、设计者及上游依赖。原有致谢汇总见 [上游 README](https://github.com/rime/weasel#readme)；高亮条宽度方案参考 [ccyybn 的贡献](https://github.com/rime/weasel/pull/1156)。
- 感谢 [qingjian / 青简](https://github.com/qingjian-team/qingjian)。含章·通变推理代码与模型接入参考自该项目。

本仓库代码遵循 [GPLv3](LICENSE.txt)。
原有第三方许可文件保持不变。模型权重许可请查阅其来源项目。
