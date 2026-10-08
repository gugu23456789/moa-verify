// moa_capi.cc — C ABI（供 Python ctypes 等 FFI 零依赖绑定）。
//
// 契约：moa_verify_csv 把 canonical JSON 写入 out（容量 cap，含结尾 '\0'），
// 返回写入所需总长度（不含 '\0'）。out==nullptr 或 cap 不足时仅返回所需长度，
// 绝不越界写——调用方按"两段式"分配（先问长度、再取内容）。
#include "moa_verify.h"

#include <cstddef>
#include <cstring>
#include <string>

extern "C" {

std::size_t moa_verify_csv(const char* csv, char* out, std::size_t cap) {
  const std::string input = (csv != nullptr) ? std::string(csv) : std::string();
  const moa::Aggregate agg = moa::recompute(moa::parse_csv(input));
  const std::string json = moa::to_canonical_json(agg);
  const std::size_t need = json.size();
  if (out != nullptr && cap > need) {  // cap 必须留出结尾 '\0' 位置
    std::memcpy(out, json.data(), need);
    out[need] = '\0';
  }
  return need;
}

}  // extern "C"
