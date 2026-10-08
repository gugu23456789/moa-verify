// moa_wasm.cc — WASM (Embind) 绑定：浏览器端独立重算密封批聚合。
//
// 与 native / Python 使用**同一** C++ 纯核；跨运行时得逐字节相同的 canonical JSON。
#include "moa_verify.h"

#include <string>

#include <emscripten/bind.h>

namespace {

// JS 传入密封批 CSV 文本 → 返回 canonical JSON 字符串（供逐字节比对）。
std::string verifyCsv(const std::string& csv) {
  return moa::to_canonical_json(moa::recompute(moa::parse_csv(csv)));
}

}  // namespace

EMSCRIPTEN_BINDINGS(moa_verify_module) {
  emscripten::function("verifyCsv", &verifyCsv);
}
