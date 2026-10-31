# 前词搭配学习

适用于当前 rime_ice 全拼方案。修改 `rime_ice.custom.yaml` 后，在 Windows 托盘执行“重新部署”：

```yaml
patch:
  contextual_pair/enabled: true
  contextual_pair/timeout_seconds: 60
  contextual_pair/candidate_limit: 24
  engine/filters/@before 4: lua_filter@*contextual_pair
```

将 enabled 改为 false 并重新部署即可关闭，关闭后不读取或记录搭配。

例如先上屏“部署”，再输入 mo xing 并选择“模型”。下一次上屏“部署”之后，同样输入 mo xing，在前 24 个候选里存在“模型”时，它会优先于没有学过的同范围候选。不同前文分别统计，次数相同保留原顺序；显式置顶规则仍然优先。

这里的“前词”是上一次完整上屏的中文片段，最长 32 字；不是自动中文分词。超时、上屏标点或英文后清除前文。前文在当前 Lua 引擎实例中保留，不读取应用中的旧文档，也不能感知应用内光标移动。需要更严格的上下文边界时可缩短 timeout_seconds。

搭配保存在用户目录 `contextual_pair.tsv`，每次学习追加一行。停止输入法后删除这个文件，可清除已学搭配。只调整已有、覆盖相同输入范围的中文候选，不创造新词，也不会把短词提到整句前面。

这是个人搭配学习，不是神经语义推理。新安装没有搭配记录时保持原排序。当前通变模型忽略前文，不能据此宣称已支持从未学习过的上下文搭配。Weasel 的本次 C++ 修改会尊重 Lua 提升后的首选，避免生成候选再次抢到其前面；必须使用重新编译的 Weasel 才有这项协作行为。普通短词的 Lua 排序不依赖新的 DLL。

万象 gram 已移除，源方案不再引用 grammar，contextual_suggestions 已关闭；现存 build 中相关引用也已清除。新增 Lua 的部署产物应由 Windows 重新部署生成。

验证：

```sh
test_dir=$(mktemp -d)
lua Rime/lua/tests/contextual_pair_test.lua "$test_dir"
rm -r "$test_dir"
python3 weasel/test/TestP2cEngine/run_portable.py
```

Lua 测试覆盖排序、开关、持久化、超时、特殊候选、输入覆盖范围和候选保留。C++ 测试使用生产 P2cEngine 的线程代码和模拟的 DLL 接口，覆盖取消、同串新请求、旧请求晚返回、空结果与错误结果；不验证真实 Windows DLL、TSF 或微信窗口位置。
