// test_moa_verify.cc — 独立重算的地面真值测试
// 真值来源：repro/expected_moa.txt（由 repro/aggregate_moa.py 生成）。
// 这是**跨语言交叉检验**：C++ 核须与 Python repro 得出逐字相同的聚合量。
// 输入：sealed batch = repro/moa_batch.csv（路径经 -DBATCH_CSV=... 注入）。
#include "moa_verify.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

static bool approx2(double x, double want) { return std::fabs(x - want) < 0.005; }

static std::vector<moa::Arm> load_batch(const char* path) {
  std::ifstream in(path, std::ios::binary);
  std::stringstream ss;
  ss << in.rdbuf();
  return moa::parse_csv(ss.str());
}

int main() {
  const char* csv = BATCH_CSV;
  std::vector<moa::Arm> arms = load_batch(csv);
  if (arms.size() != 6) { std::printf("FAIL: arms=%zu (want 6)\n", arms.size()); return 1; }

  moa::Aggregate a = moa::recompute(arms);

  // 期望值（逐字取自 repro/expected_moa.txt）
  struct Row { int idx; double mean; int mn; int mx; } exp[] = {
      {0, 63.17, 45, 78}, {1, 69.00, 62, 78}, {2, 45.33, 25, 62},
      {3, 39.83, 28, 58}, {4, 68.33, 55, 82}, {5, 62.17, 45, 82},
      {6, 51.83, 40, 74},
  };
  for (const auto& e : exp) {
    if (!approx2(a.per[e.idx].mean, e.mean)) {
      std::printf("FAIL: per[%d].mean=%.4f want %.2f\n", e.idx, a.per[e.idx].mean, e.mean);
      return 1;
    }
    if (a.per[e.idx].min != e.mn || a.per[e.idx].max != e.mx) {
      std::printf("FAIL: per[%d] min/max=(%d,%d) want (%d,%d)\n", e.idx,
                  a.per[e.idx].min, a.per[e.idx].max, e.mn, e.mx);
      return 1;
    }
  }
  if (!approx2(a.batch_mean, 57.10)) {
    std::printf("FAIL: batch_mean=%.4f want 57.10\n", a.batch_mean);
    return 1;
  }
  if (a.top_verdict != "not_landable" || a.top_count != 6) {
    std::printf("FAIL: consensus=%s %d want not_landable 6\n",
                a.top_verdict.c_str(), a.top_count);
    return 1;
  }

  // canonical JSON：键序固定、2 位小数 —— 第三方逐字节 diff 的契约。
  // golden 由 repro/expected_moa.txt（Python 产出）转写，须逐字节相同。
  const char* want_json =
      "{\"arms\":6,\"per\":{"
      "\"scope\":{\"mean\":63.17,\"min\":45,\"max\":78},"
      "\"evidence\":{\"mean\":69.00,\"min\":62,\"max\":78},"
      "\"assumptions\":{\"mean\":45.33,\"min\":25,\"max\":62},"
      "\"recovery\":{\"mean\":39.83,\"min\":28,\"max\":58},"
      "\"references\":{\"mean\":68.33,\"min\":55,\"max\":82},"
      "\"bias\":{\"mean\":62.17,\"min\":45,\"max\":82},"
      "\"cost\":{\"mean\":51.83,\"min\":40,\"max\":74}},"
      "\"batch_mean\":57.10,\"consensus\":{\"not_landable\":6}}";
  std::string got_json = moa::to_canonical_json(a);
  if (got_json != want_json) {
    std::printf("FAIL: canonical json mismatch\n  got =%s\n  want=%s\n",
                got_json.c_str(), want_json);
    return 1;
  }

  // 空批确定性回归守卫：曾因 CriterionStat 未初始化 → 同一空输入每次产出不同 JSON。
  const std::string e1 = moa::to_canonical_json(moa::recompute({}));
  const std::string e2 = moa::to_canonical_json(moa::recompute({}));
  if (e1 != e2) {
    std::printf("FAIL: empty-batch canonical JSON is non-deterministic\n  e1=%s\n  e2=%s\n",
                e1.c_str(), e2.c_str());
    return 1;
  }
  if (e1.find("\"arms\":0") == std::string::npos ||
      e1.find("\"batch_mean\":0.00") == std::string::npos) {
    std::printf("FAIL: empty-batch JSON malformed: %s\n", e1.c_str());
    return 1;
  }

  std::printf("OK: sealed batch re-derived independently (arms=%d, batch_mean=%.2f, consensus=%s %d/%d)\n",
              a.arms, a.batch_mean, a.top_verdict.c_str(), a.top_count, a.arms);
  return 0;
}
