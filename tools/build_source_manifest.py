#!/usr/bin/env python3
"""Build the benchmark source manifest consumed by the dashboard.

Usage:
    python tools/build_source_manifest.py <input_dir> <repo_root> <output_file> <commit>

Each ``bench_*.json`` file is matched with ``benchmarks/<stem>.cpp``. Benchmark
suite names are taken from the JSON output, so the dashboard does not need to
guess source filenames from display names.
"""

from __future__ import annotations

import json
import sys
from pathlib import Path


AGGREGATES = ("mean", "median", "stddev", "cv")


def benchmark_case_name(name: str) -> str:
    parts = [part for part in name.split("/") if part]
    if not parts:
        raise ValueError("benchmark name must not be empty")
    if len(parts) > 1:
        return parts[0]

    case_name = parts[0]
    for aggregate in AGGREGATES:
        suffix = f"_{aggregate}"
        if case_name.endswith(suffix):
            return case_name[: -len(suffix)]
    return case_name


def build_manifest(input_dir: Path, repo_root: Path, commit: str) -> dict:
    cases: dict[str, str] = {}
    files: dict[str, str] = {}
    result_files = sorted(input_dir.glob("bench_*.json"))
    if not result_files:
        raise ValueError(f"no bench_*.json files found in {input_dir}")

    for result_file in result_files:
        source_file = repo_root / "benchmarks" / f"{result_file.stem}.cpp"
        if not source_file.is_file():
            raise ValueError(
                f"source file not found for {result_file.name}: {source_file}"
            )

        source_path = source_file.relative_to(repo_root).as_posix()
        files[source_path] = source_file.read_text(encoding="utf-8")

        with result_file.open(encoding="utf-8") as file:
            result = json.load(file)
        for benchmark in result.get("benchmarks", []):
            case_name = benchmark_case_name(str(benchmark.get("name", "")))
            previous_path = cases.get(case_name)
            if previous_path is not None and previous_path != source_path:
                raise ValueError(
                    f"benchmark case {case_name!r} maps to both "
                    f"{previous_path} and {source_path}"
                )
            cases[case_name] = source_path

    return {
        "version": 1,
        "commit": commit,
        "cases": dict(sorted(cases.items())),
        "files": dict(sorted(files.items())),
    }


def main() -> int:
    if len(sys.argv) != 5:
        print(
            f"Usage: {sys.argv[0]} <input_dir> <repo_root> <output_file> <commit>",
            file=sys.stderr,
        )
        return 1

    input_dir = Path(sys.argv[1])
    repo_root = Path(sys.argv[2])
    output_file = Path(sys.argv[3])

    try:
        manifest = build_manifest(input_dir, repo_root, sys.argv[4])
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1

    output_file.write_text(
        "window.BENCHMARK_SOURCES = " + json.dumps(manifest, indent=2) + ";\n",
        encoding="utf-8",
    )
    print(
        f"Wrote {len(manifest['cases'])} benchmark source mapping(s) to {output_file}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
