from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from build_source_manifest import benchmark_case_name, build_manifest


class BuildSourceManifestTest(unittest.TestCase):
    def test_benchmark_case_name_matches_dashboard_grouping(self) -> None:
        self.assertEqual(
            benchmark_case_name("ApiShape/has_value/4096_mean"), "ApiShape"
        )
        self.assertEqual(benchmark_case_name("Single_mean"), "Single")
        self.assertEqual(benchmark_case_name("Single"), "Single")

    def test_build_manifest_maps_results_to_their_source_file(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            results = root / "results"
            benchmarks = root / "benchmarks"
            results.mkdir()
            benchmarks.mkdir()
            source = "#include <benchmark/benchmark.h>\n"
            (benchmarks / "bench_api_shape.cpp").write_text(source, encoding="utf-8")
            (results / "bench_api_shape.json").write_text(
                json.dumps(
                    {
                        "benchmarks": [
                            {"name": "ApiShape/has_failure/4096_mean"},
                            {"name": "ApiShape/has_value/4096_mean"},
                        ]
                    }
                ),
                encoding="utf-8",
            )

            manifest = build_manifest(results, root, "abc123")

            self.assertEqual(manifest["commit"], "abc123")
            self.assertEqual(
                manifest["cases"], {"ApiShape": "benchmarks/bench_api_shape.cpp"}
            )
            self.assertEqual(
                manifest["files"]["benchmarks/bench_api_shape.cpp"], source
            )


if __name__ == "__main__":
    unittest.main()
