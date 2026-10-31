本版本基于 Rime Weasel，加入青简「含章·通变」本地整句生成。

- 后台推理与停键防抖。
- 生成句默认放在第二项，与词库候选并存。
- 保留手动选定的前缀，只生成剩余拼音。
- 支持 `✦` 星标、自定义符号和来源标记。
- 支持可关闭的前词搭配学习。
- 修复注入候选的选择映射和异步刷新问题。
- 保留可调高亮条宽度。
- 安装器自动安装匹配架构的 P2C DLL。

下载 `weasel-*-installer.exe` 安装。
可选配置与 Lua 文件位于 `p2c-rime-config.zip`。
校验值见 `SHA256SUMS.txt`。

升级后，请重新部署并重启服务。
配置说明见[仓库 README](https://github.com/saladxuanzhi/weasel#readme)。

本仓库不提供青简模型权重，也不提供权重下载直链。
模型说明与获取方式请查阅 [qingjian / 青简项目](https://github.com/qingjian-team/qingjian)。
