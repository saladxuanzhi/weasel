/*
 * p2c.h — 含章·通变整句模型（P2C）的 C 接口。
 *
 * 由 weasel-p2c.dll 导出（Rust 侧见 src/lib.rs）。用法：
 *
 *     P2cHandle* h = p2c_load("C:\\path\\hanzhang-tongbian-small.qjm");
 *     if (!h) { log(p2c_last_error()); return; }
 *
 *     P2cCandidate* cands = NULL;
 *     int n = p2c_convert(h, "jintiantianqihenhao", 5, 0, &cands);
 *     for (int i = 0; i < n; ++i)
 *         wprintf(L"%hs  %.2f\n", cands[i].text, cands[i].score);
 *     p2c_candidates_free(cands, n);
 *
 *     p2c_free(h);
 *
 * 线程：一个 P2cHandle 可以被多线程同时用于 p2c_convert / p2c_knows（只读权重）；
 *       但 p2c_last_error 是每线程的，跨线程读不到对方的消息。
 * 编码：所有字符串都是 UTF-8。Windows 上路径请先用 WideCharToMultiByte(CP_UTF8) 转，
 *       不要直接传 CP_ACP 的窄字符串。
 * 生命周期：p2c_convert 返回的数组必须用 p2c_candidates_free(数组, 条数) 释放；
 *       条数是返回值，不是数组长度，别自己猜。
 */
#ifndef WEASEL_P2C_H_
#define WEASEL_P2C_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct P2cHandle P2cHandle;

typedef struct P2cCandidate {
  /* UTF-8 的整句文本，归候选数组所有 */
  char* text;
  /* 这段汉字的 log 概率之和，越接近 0 越好；只在同一次调用的候选之间可比 */
  double score;
} P2cCandidate;

/* 最近一次失败的原因（UTF-8，空串表示没有错误）。指针在下次出错前有效。 */
const char* p2c_last_error(void);

/*
 * 加载模型。`model_path` 是 .qjm 单文件、三件套目录，或装着其中之一的目录。
 * 35~45 MB 权重 + 建图，实测 0.3~3 秒，不要放在按键路径上。
 * 失败返回 NULL。
 */
P2cHandle* p2c_load(const char* model_path);

/* 释放句柄。传 NULL 安全。 */
void p2c_free(P2cHandle* handle);

/*
 * 拼音 → 整句候选。
 *   keys       不带分隔的小写全拼，如 "jintiantianqihenhao"
 *   beam       束宽，建议 5
 *   max_chars  生成长度上限，给 0 表示按按键长度自动估
 *   out        成功时写入候选数组
 * 返回候选条数（>= 0）；失败返回 -1，`out` 不动。
 * 平均 80~200 ms（视 CPU 与句长），最慢可达 2 秒以上——务必放在后台线程。
 */
int p2c_convert(P2cHandle* handle,
                const char* keys,
                int beam,
                int max_chars,
                P2cCandidate** out);

/* 释放 p2c_convert 写出的数组（连同每条 text）。 */
void p2c_candidates_free(P2cCandidate* candidates, int count);

/* 字表里有没有这些字。0 = 没有（或出错），1 = 有。字表外的字模型算不准。 */
int p2c_knows(P2cHandle* handle, const char* text);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* WEASEL_P2C_H_ */
