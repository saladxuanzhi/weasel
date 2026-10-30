/* C ABI 冒烟测试：完全按 weasel 那侧（C/C++）的用法调用 Rust 的 cdylib。 */
#include <stdio.h>
#include <string.h>
#include "../include/p2c.h"

int main(int argc, char** argv) {
  if (argc < 3) { fprintf(stderr, "用法: %s <model.qjm> <拼音>\n", argv[0]); return 2; }

  P2cHandle* h = p2c_load(argv[1]);
  if (!h) { fprintf(stderr, "加载失败: %s\n", p2c_last_error()); return 1; }
  printf("模型已加载\n");

  P2cCandidate* cands = NULL;
  int n = p2c_convert(h, argv[2], 5, 0, &cands);
  if (n < 0) { fprintf(stderr, "转换失败: %s\n", p2c_last_error()); p2c_free(h); return 1; }

  printf("%s -> %d 条\n", argv[2], n);
  for (int i = 0; i < n; ++i)
    printf("  %d. %-24s %8.2f  (knows=%d)\n", i + 1, cands[i].text, cands[i].score,
           p2c_knows(h, cands[i].text));
  p2c_candidates_free(cands, n);

  /* 错误路径：NULL 与非 UTF-8 都要安全 */
  printf("空指针调用返回 %d（应 -1）\n", p2c_convert(NULL, argv[2], 5, 0, &cands));
  printf("最后错误: %s\n", p2c_last_error());

  p2c_free(h);
  p2c_free(NULL); /* 必须安全 */
  printf("OK\n");
  return 0;
}
