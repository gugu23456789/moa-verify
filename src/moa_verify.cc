// moa_verify.cc — MoA 密封批聚合的独立重算核（纯函数实现）
// 与 repro/aggregate_moa.py 得出**逐字相同**的聚合量（跨语言交叉检验）。
#include "moa_verify.h"

#include <cmath>
#include <cstdio>
#include <map>
#include <sstream>
#include <string>

namespace moa {

static double round2(double x) { return std::round(x * 100.0) / 100.0; }

static const char* kNames[kCriteria] = {
    "scope", "evidence", "assumptions", "recovery", "references", "bias", "cost"};

std::vector<Arm> parse_csv(const std::string& csv) {
  std::vector<Arm> out;
  const int need = 1 + kCriteria + 1;  // arm + F1..F7 + top_verdict
  std::istringstream in(csv);
  std::string line;
  bool header = true;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (header) { header = false; continue; }  // 跳过表头
    if (line.empty()) continue;
    std::istringstream ls(line);
    std::vector<std::string> col;
    std::string cell;
    while (std::getline(ls, cell, ',')) col.push_back(cell);
    if (static_cast<int>(col.size()) < need) continue;
    Arm a{};
    for (int i = 0; i < kCriteria; ++i) a.f[i] = std::stoi(col[1 + i]);
    a.verdict = col[1 + kCriteria];
    out.push_back(a);
  }
  return out;
}

Aggregate recompute(const std::vector<Arm>& arms) {
  Aggregate a{};  // 值初始化：空批路径下 per[] 亦为确定的 0（避免未初始化内存）
  a.arms = static_cast<int>(arms.size());
  if (arms.empty()) return a;

  long long total = 0;
  for (int c = 0; c < kCriteria; ++c) {
    long long sum = 0;
    int mn = arms[0].f[c];
    int mx = arms[0].f[c];
    for (const Arm& arm : arms) {
      const int v = arm.f[c];
      sum += v;
      if (v < mn) mn = v;
      if (v > mx) mx = v;
    }
    a.per[c].mean = round2(static_cast<double>(sum) / static_cast<double>(arms.size()));
    a.per[c].min = mn;
    a.per[c].max = mx;
    total += sum;
  }
  a.batch_mean = round2(static_cast<double>(total) /
                        (static_cast<double>(arms.size()) * kCriteria));

  // top-verdict consensus（确定性：最高票；平票取批内先出现者）
  std::map<std::string, int> tally;
  for (const Arm& arm : arms) tally[arm.verdict] += 1;
  int best = -1;
  for (const Arm& arm : arms) {
    const int c = tally[arm.verdict];
    if (c > best) {
      best = c;
      a.top_verdict = arm.verdict;
      a.top_count = c;
    }
  }
  return a;
}

std::string to_canonical_json(const Aggregate& a) {
  char buf[256];
  std::string out = "{\"arms\":";
  out += std::to_string(a.arms);
  out += ",\"per\":{";
  for (int c = 0; c < kCriteria; ++c) {
    if (c) out += ",";
    std::snprintf(buf, sizeof(buf), "\"%s\":{\"mean\":%.2f,\"min\":%d,\"max\":%d}",
                  kNames[c], a.per[c].mean, a.per[c].min, a.per[c].max);
    out += buf;
  }
  out += "},";
  std::snprintf(buf, sizeof(buf), "\"batch_mean\":%.2f", a.batch_mean);
  out += buf;
  out += ",\"consensus\":{\"";
  out += a.top_verdict;
  out += "\":";
  out += std::to_string(a.top_count);
  out += "}}";
  return out;
}

}  // namespace moa
