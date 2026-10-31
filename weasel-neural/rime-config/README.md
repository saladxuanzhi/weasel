# 前词搭配学习配置包

适用于当前使用的 rime_ice 全拼方案，不包含个人词库或历史记录。

1. 将 `lua/contextual_pair.lua` 放进 Rime 用户目录的 `lua/`。
   必须与 custom 配置一起使用；只复制 YAML 会引用尚未安装的 Lua 组件。旧版脚本有候选迭代接口错误，需要替换为本目录最新版。
2. 将 `rime_ice.custom.yaml` 中的 patch 合并进自己的同名文件，已有设置需保留。
3. 将 `weasel.custom.example.yaml` 中的 patch 合并进自己的 `weasel.custom.yaml`。
4. 如果原先有万象 gram，删除源方案中的 grammar 段，将 translator/contextual_suggestions 设为 false，再移除不再使用的 gram 文件。
5. Windows 托盘右键“重新部署”，再重启小狼毫服务。

模型和原生 DLL 仍需单独放置。P2C 默认查找安装目录或用户目录中的 hanzhang-tongbian-small.qjm；weasel_p2c.dll 放在 WeaselServer.exe 同目录。本轮没有修改 Rust ABI，可沿用已有匹配服务端架构的 DLL。

新版 Weasel 默认保留词库第一项，将生成句放在第二项，避免停键后改变默认提交的词句。`weasel.custom.yaml` 的 patch 下可设置 `p2c/prefer_generated: false`；改为 true 则恢复生成优先。这个开关需要安装支持它的新构建，修改后重启服务。

排查候选质量时，可设置 `p2c/show_source: true` 并重启服务：`〔Rime〕` 表示 librime 候选流（含 Lua 排序），`〔通变生成〕` 表示额外插入的模型生成项；`〔Rime〕〔通变同文〕` 表示模型首选与该 Rime 候选相同，已去重并保留原排名。仅凭第一、第二的位置不能判断来源。若要复现旧版“生成优先”策略，可同时设 `p2c/prefer_generated: true`；检查后自行选择策略，来源标记可关闭。

手动选定前半句后，模型只处理剩余的未选拼音，选生成项时保留已确认前缀。`min_keys` 按剩余字母数判断，例如前半句已选、只剩 cha 时不调用整句模型。光标在中间，或预编辑格式无法与原始拼音对齐时，保留普通候选；不会退回猜测完整原始输入。

开关和限制见 CONTEXTUAL-PAIR.md。关闭 contextual_pair/enabled 后恢复原排序。此功能学习已选择的中文搭配，不提供未学习过的神经语义判断。

`lua/tests/` 仅供开发验证，日常使用无需复制。
