## 环境

```
rustc 1.99.1 (58cf8f9c3 2026-10-13)
cargo 1.99.1 (5f94df478 2026-08-27)
Microsoft Windows Server 2025 Datacenter / Intel(R) Xeon(R) 6973P-C
```
info: component rust-std is up to date
info: component rust-std for target i686-pc-windows-msvc is up to date

## 产物

```
target\x86_64-pc-windows-msvc\release\weasel_p2c.dll    2,384,384 B
target\i686-pc-windows-msvc\release\weasel_p2c.dll    2,154,496 B
```

## 导出符号（x64）

```
164   A3 00028150 p2c_candidates_free = p2c_candidates_free
165   A4 000281D0 p2c_convert = p2c_convert
166   A5 00028590 p2c_free = p2c_free
167   A6 000285D0 p2c_knows = p2c_knows
168   A7 00028630 p2c_last_error = p2c_last_error
169   A8 000286A0 p2c_load = p2c_load
```

## 模型

```
sha256(qingjian-data.tar.gz) = bc8dd7f4e283c6b7fa0f38e493ea89e5d874ccf45ff22308ca52f4078df6694b
hanzhang-tongbian-small.qjm  46,141,208 B
```

## C ABI 冒烟（纯 C 调用 dll）

```
模型已加载
jintiantianqihenhao -> 5 条
  1. 今天天气很好          -0.01  (knows=1)
  2. 今天天起很好          -6.32  (knows=1)
  3. 近天天气很好          -6.98  (knows=1)
  4. 今天天气很高          -7.45  (knows=1)
  5. 今天天气很好啊       -7.78  (knows=1)
空指针调用返回 -1（应 -1）
最后错误: p2c_convert: 空指针
OK
```

## 整句生成

```
模型: 含章·通变（Hanzhang Tongbian）  加载 203.8802ms

jintiantianqihenhao  (87.3111ms, 5 条)
  1. 今天天气很好                        -0.01
  2. 今天天起很好                        -6.32
  3. 近天天气很好                        -6.98
  4. 今天天气很高                        -7.45
  5. 今天天气很好啊                       -7.78

shurukuang  (40.6646ms, 5 条)
  1. 输入框                           -0.30
  2. 输入矿                           -2.15
  3. 输入筐                           -4.18
  4. 输入狂                           -4.41
  5. 输入况                           -4.44

houxuanshengcheng  (61.0335ms, 5 条)
  1. 候选声称                          -1.94
  2. 候选生成                          -2.22
  3. 后宣声称                          -2.93
  4. 后宣生成                          -3.47
  5. 候选省城                          -3.78

nihaoshijie  (65.389ms, 5 条)
  1. 你好世界                          -1.34
  2. 你好师姐                          -1.84
  3. 你好时节                          -2.01
  4. 你好是饥饿                         -3.35
  5. 你好是戒                          -3.45

womenjintianyiqiqugongyuanwanyiba  (204.2913ms, 5 条)
  1. 我们今天一起去公园玩一把                  -0.28
  2. 我们今天一起去公园玩一吧                  -3.17
  3. 我们今天一起去公园玩意吧                  -4.31
  4. 我们今天一起去公元晚一把                  -4.32
  5. 我们今天一起去公园万一吧                  -4.45

womenmingtianqukanchuangyi  (125.1002ms, 5 条)
  1. 我们明天去看创意                      -0.16
  2. 我们明天去看床衣                      -3.14
  3. 我们明天去看床医                      -3.78
  4. 我们明天去看床以                      -4.60
  5. 我们明天去看床艺                      -4.67

稳态延迟: 平均 97.298283ms / 最慢 204.2913ms（6 次）
```

## 逐键延迟

```
加载 202.867ms  beam=5

  键 已输入的拼音                       条数         延迟
  1 j                             5     15.2 ms  → 将
  2 ji                            5     14.6 ms  → 吉
  3 jin                           5     16.1 ms  → 金
  4 jint                          5     23.7 ms  → 今天
  5 jinti                         5     25.9 ms  → 今体
  6 jintia                        5     35.3 ms  → 今天
  7 jintian                       5     28.6 ms  → 今天
  8 jintiant                      5     40.7 ms  → 今天T
  9 jintianti                     5     40.9 ms  → 今天题
 10 jintiantia                    5     51.2 ms  → 今天提啊
 11 jintiantian                   5     53.7 ms  → 今天天
 12 jintiantianq                  5     54.9 ms  → 今天天气
 13 jintiantianqi                 5     60.6 ms  → 今天天气
 14 jintiantianqih                5     69.1 ms  → 今天天气好
 15 jintiantianqihe               5     69.8 ms  → 今天天气和
 16 jintiantianqihen              5     71.6 ms  → 今天天气很
 17 jintiantianqihenh             5     84.1 ms  → 今天天气很好
 18 jintiantianqihenha            5     85.1 ms  → 今天天气很好
 19 jintiantianqihenhao           5     88.0 ms  → 今天天气很好

每键平均 48.9 ms / 最慢 88.0 ms（19 键）
```
