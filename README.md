# moa-verify

**MoA Sealed-Recompute Verifier** — independently recompute a mixture-of-agents
sealed-batch aggregate in a different language / runtime than the one that produced
it, and get **byte-identical** canonical JSON.

One pure C++ core is exposed three ways — native, Python (`ctypes`, wheel-bundled),
and the browser (Embind WebAssembly with a `SHA-384` inlined trust root and
fail-closed runtime verification). All three reproduce the same numbers from the
same sealed input.

## Why

A published aggregate table is only independently checkable if a third party can
re-derive it without trusting the original tooling. `moa-verify` is that second
implementation: same input → same canonical JSON, across C++, WASM and Python.
The WebAssembly path additionally makes the *delivery* tamper-evident: the loader
verifies the verifier (`.wasm` SHA-384) and the evidence (`moa_batch.csv` SHA-384)
before it recomputes anything.

## Layout

```
src/          moa_verify.{h,cc}   pure, deterministic recompute core (zero I/O, zero global state)
              moa_capi.cc         C ABI (moa_verify_csv) for FFI / ctypes
              moa_wasm.cc         Embind binding (verifyCsv)
tests/        test_moa_verify.cc  native ground-truth test (CTest)
              wasm_smoke_test.mjs node smoke (WASM == reference; tamper detected)
              browser_smoke_test.mjs playwright smoke (page verifies + recomputes)
web/          index.html          SHA-384 trust root + fail-closed loader + byte compare
python/       pyproject.toml, moa_verify/  ctypes binding, wheel-bundled shared lib
data/         moa_batch.csv       the sealed input (6 arms × F1..F7 + top verdict)
              expected_moa.txt    the reference aggregate (canonical JSON golden is transwritten from it)
```

## Build & test

Native core + shared library + CTest:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Python binding (no third-party deps; locates `build/libmoa_verify.so` or `$MOA_VERIFY_LIB`):

```sh
PYTHONPATH=python python -m pytest python/tests/test_moa_verify.py -q
PYTHONPATH=python python -m moa_verify data/moa_batch.csv   # prints canonical JSON
```

WebAssembly (needs the Emscripten SDK on `PATH`):

```sh
emcmake cmake -S . -B build-wasm -DCMAKE_BUILD_TYPE=Release
emmake cmake --build build-wasm
node tests/wasm_smoke_test.mjs build-wasm/site data/moa_batch.csv
```

## Usage

```python
from moa_verify import verify_file
print(verify_file("data/moa_batch.csv"))
# {"arms":6,"per":{"scope":{"mean":63.17,"min":45,"max":78}, ...},"batch_mean":57.10,"consensus":{"not_landable":6}}
```

C ABI: `size_t moa_verify_csv(const char* csv, char* out, size_t cap)` — two-pass
(query length, then fill); never writes past `cap`.

## Trust root (browser)

`web/index.html` carries two `<meta>` trust roots (`moa-wasm-fp`, `moa-input-fp`).
CI injects the SHA-384 of the built `moa_verify.wasm` and of `moa_batch.csv`; the
page refuses to instantiate on mismatch, when the fingerprint probe fails, or when
`crypto.subtle` is unavailable (all fail-closed). Placeholders left in place mean
"development mode": the page runs but reports unverified.

## CI

`.github/workflows/moa-verify.yml` runs three independent checks — native CTest,
Python `ctypes` tests (+ a wheel that bundles the shared library), and a WASM job
(Embind build → node smoke → fingerprint injection → playwright browser smoke) —
plus a workflow supply-chain audit.

## Sealed input

`data/moa_batch.csv` is the recorded sealed-score table of batch `1f25e48e17f0`
(six reviewer arms admitted in a single batch, each with its own rubric F1..F7 and
a top-level verdict). `moa-verify` only re-derives the aggregation.

## License

Mulan PSL v2 (see `LICENSE`). The WebAssembly engineering layout is adapted from
the Mulan PSL v2-licensed `spz_gatekeeper` wasm/CI templates; changes are marked in
`LICENSE` per section 4.
