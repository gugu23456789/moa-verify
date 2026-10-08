cmake_minimum_required(VERSION 3.16)
project(moa_wasm CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── 独立重算核（纯函数；浏览器/原生同源）──────────────────────────────
add_library(moa_verify STATIC src/moa_verify.cc)
target_include_directories(moa_verify PUBLIC src)

# ── 共享库（C ABI）——供 Python ctypes 等 FFI 零依赖绑定 ────────────────
if(NOT EMSCRIPTEN)
  add_library(moa_verify_shared SHARED src/moa_verify.cc src/moa_capi.cc)
  target_include_directories(moa_verify_shared PUBLIC src)
  set_target_properties(moa_verify_shared PROPERTIES OUTPUT_NAME "moa_verify")
endif()

# ── 原生地面真值测试（TDD 红-绿的可证面）──────────────────────────────
# 输入 = 论文 repro 的密封批 CSV（跨语言交叉检验）。
add_executable(test_moa_verify tests/test_moa_verify.cc)
target_link_libraries(test_moa_verify PRIVATE moa_verify)
target_compile_definitions(test_moa_verify PRIVATE
  BATCH_CSV="${CMAKE_CURRENT_SOURCE_DIR}/data/moa_batch.csv")

enable_testing()
add_test(NAME moa_verify COMMAND test_moa_verify)

# ── WASM 打包（仅 Emscripten；"C" 步落地）────────────────────────────
# 复用门卫/spz2glb 的 WASM 管线：Embind + MODULARIZE + 站点目录。
# 原生构建不受影响；CI 侧用 emcmake 走此分支。
if(EMSCRIPTEN)
  set(MOA_WASM_SITE_DIR "${CMAKE_CURRENT_BINARY_DIR}/site")

  add_executable(moa_verify_wasm src/moa_verify.cc src/moa_wasm.cc)
  target_include_directories(moa_verify_wasm PRIVATE src)
  target_compile_options(moa_verify_wasm PRIVATE "-O3")

  # ENVIRONMENT 含 node：CI 里用 node 直接加载 glue 做交叉检验；浏览器端走 web。
  target_link_options(moa_verify_wasm PRIVATE
    "--bind"
    "--no-entry"
    "-sMODULARIZE=1"
    "-sEXPORT_NAME=createMoaVerifyModule"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sMAXIMUM_MEMORY=512MB"
    "-sENVIRONMENT=web,node"
    "-sFILESYSTEM=0"
    "-O3"
  )
  set_target_properties(moa_verify_wasm PROPERTIES
    OUTPUT_NAME "moa_verify"
    RUNTIME_OUTPUT_DIRECTORY "${MOA_WASM_SITE_DIR}"
  )

  # 站点入口 + 密封输入（web/ 为单一来源；占位符 __WASM_FP__/__INPUT_FP__/__FP__ 由 CI 注入）
  set(MOA_SEALED_CSV "${CMAKE_CURRENT_SOURCE_DIR}/data/moa_batch.csv")
  add_custom_command(
    OUTPUT "${MOA_WASM_SITE_DIR}/index.html" "${MOA_WASM_SITE_DIR}/moa_batch.csv"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${MOA_WASM_SITE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/web/index.html"
            "${MOA_WASM_SITE_DIR}/index.html"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MOA_SEALED_CSV}"
            "${MOA_WASM_SITE_DIR}/moa_batch.csv"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/web/index.html" "${MOA_SEALED_CSV}"
    COMMENT "Copy web entrypoint + sealed batch into WASM site directory"
    VERBATIM
  )
  add_custom_target(moa_verify_wasm_site ALL
    DEPENDS moa_verify_wasm
            "${MOA_WASM_SITE_DIR}/index.html"
            "${MOA_WASM_SITE_DIR}/moa_batch.csv"
  )
endif()
