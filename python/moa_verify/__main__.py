"""CLI: ``python -m moa_verify <sealed-batch.csv>`` prints the canonical JSON."""
from __future__ import annotations

import sys

from . import verify_file


def main(argv: list[str] | None = None) -> int:
    args = list(sys.argv[1:] if argv is None else argv)
    if len(args) != 1:
        print("usage: python -m moa_verify <sealed-batch.csv>", file=sys.stderr)
        return 2
    print(verify_file(args[0]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
