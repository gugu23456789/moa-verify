"""moa-verify — independent recompute of the MoA sealed-batch aggregate.

Thin, zero-dependency Python binding over the C ABI (``moa_verify_csv``) of the
same C++ core that ships to WebAssembly.  Same input -> byte-identical canonical
JSON across C++, WASM and Python (cross-runner reproducibility).

The compiled shared library is located in this order:
  1. ``MOA_VERIFY_LIB`` environment variable (explicit path),
  2. the wheel-bundled ``moa_verify/_lib/`` directory,
  3. the CMake build tree next to the source package (``../build``).
"""
from __future__ import annotations

import ctypes
import os
import pathlib
import sys

__all__ = ["verify_csv", "verify_file", "find_library", "canonical_json"]

_LIB_NAMES = {"win32": "moa_verify.dll", "darwin": "libmoa_verify.dylib"}
_DEFAULT_SO = "libmoa_verify.so"


def _lib_basename() -> str:
    return _LIB_NAMES.get(sys.platform, _DEFAULT_SO)


def find_library() -> pathlib.Path:
    """Return the path to the compiled ``moa_verify`` shared library."""
    env = os.environ.get("MOA_VERIFY_LIB")
    if env:
        p = pathlib.Path(env)
        if not p.is_file():
            raise FileNotFoundError(f"MOA_VERIFY_LIB points to a missing file: {p}")
        return p

    here = pathlib.Path(__file__).resolve()
    name = _lib_basename()
    candidates = [
        here.parent / "_lib" / name,               # wheel-bundled
        here.parents[2] / "build" / name,          # CMake build tree
        here.parents[2] / "build" / "Release" / name,
    ]
    for c in candidates:
        if c.is_file():
            return c
    raise FileNotFoundError(
        "moa_verify shared library not found; build it (cmake --build build) or set "
        f"MOA_VERIFY_LIB. Looked in: {', '.join(str(c) for c in candidates)}"
    )


_lib: ctypes.CDLL | None = None


def _load() -> ctypes.CDLL:
    lib = ctypes.CDLL(str(find_library()))
    lib.moa_verify_csv.restype = ctypes.c_size_t
    lib.moa_verify_csv.argtypes = [
        ctypes.c_char_p,   # csv (utf-8, NUL-terminated)
        ctypes.c_char_p,   # out buffer (or NULL to query length)
        ctypes.c_size_t,   # capacity (including room for NUL)
    ]
    return lib


def _get_lib() -> ctypes.CDLL:
    global _lib
    if _lib is None:
        _lib = _load()
    return _lib


def verify_csv(csv_text: str) -> str:
    """Recompute the aggregate for a sealed-batch CSV and return canonical JSON."""
    lib = _get_lib()
    data = csv_text.encode("utf-8")
    need = lib.moa_verify_csv(data, None, 0)          # 1st pass: ask for length
    buf = ctypes.create_string_buffer(need + 1)        # +1 for NUL
    got = lib.moa_verify_csv(data, buf, need + 1)      # 2nd pass: fill
    if got != need:
        raise RuntimeError(f"moa_verify_csv length changed between passes: {got} != {need}")
    return buf.value.decode("utf-8")


def verify_file(path: os.PathLike[str] | str) -> str:
    """Recompute the aggregate for a sealed-batch CSV file."""
    return verify_csv(pathlib.Path(path).read_text(encoding="utf-8"))


# Alias: the payload is the canonical (byte-checkable) JSON of the aggregate.
canonical_json = verify_csv
