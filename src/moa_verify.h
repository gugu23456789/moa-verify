// moa_verify.h — MoA 密封批聚合的独立重算核（浏览器/原生同源）
// 目的（WWW2027 §5.1 / E1）：第三方在不同语言/运行时里、从**密封输入**独立重算出
//   与论文表格**逐字相同**的聚合量 ⟹ 「access to evidence for independent scrutiny」可执行化。
// 纪律：纯函数、零 I/O、零全局状态、确定性（同输入 ⟹ 逐字节同输出）。
#pragma once

#include <string>
#include <vector>

namespace moa {

constexpr int kCriteria = 7;   // F1..F7 = scope,evidence,assumptions,recovery,references,bias,cost

struct Arm {
  int f[kCriteria];            // 每臂的密封 F1..F7 分（各臂自评 rubric）
  std::string verdict;         // 该臂的顶层裁决
};

struct CriterionStat {
  double mean = 0.0;           // 保留 2 位（四舍五入）
  int min = 0;
  int max = 0;
};

struct Aggregate {
  int arms = 0;
  CriterionStat per[kCriteria];
  double batch_mean = 0.0;     // 全批 F1..F7 均值（2 位）
  std::string top_verdict;     // 共识裁决
  int top_count = 0;           // 投该共识的臂数
};

// 从密封批 CSV 文本解析各臂（首行表头；列序 arm,F1..F7,top_verdict）。
// 单一来源：native 测试 / C ABI / WASM 绑定共用此解析，保证跨运行时输入口径一致。
std::vector<Arm> parse_csv(const std::string& csv);

// 从密封批独立重算聚合（确定性）。
Aggregate recompute(const std::vector<Arm>& arms);

// 聚合量的 canonical JSON（键序固定、2 位小数）——供第三方逐字节 diff。
std::string to_canonical_json(const Aggregate& a);

}  // namespace moa
