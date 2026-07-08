#!/usr/bin/env python3
"""Merge multiple Google Benchmark JSON output files into a single file.

Usage:
    python tools/merge_benchmarks.py <input_dir> <output_file>

All files matching ``bench_*.json`` in ``<input_dir>`` are read, their
``benchmarks`` arrays concatenated, and the combined result written to
``<output_file>`` as ``{"benchmarks": [...]}``.
"""

from __future__ import annotations

import json
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input_dir> <output_file>", file=sys.stderr)
        return 1

    input_dir = Path(sys.argv[1])
    output_file = Path(sys.argv[2])

    if not input_dir.is_dir():
        print(f"error: {input_dir} is not a directory", file=sys.stderr)
        return 1

    all_benchmarks: list[dict] = []
    files = sorted(input_dir.glob("bench_*.json"))
    if not files:
        print(f"error: no bench_*.json files found in {input_dir}", file=sys.stderr)
        return 1

    for f in files:
        with f.open() as fh:
            data = json.load(fh)
        all_benchmarks.extend(data["benchmarks"])

    output_file.write_text(json.dumps({"benchmarks": all_benchmarks}))
    print(
        f"Merged {len(all_benchmarks)} benchmark entries from {len(files)} file(s) -> {output_file}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
