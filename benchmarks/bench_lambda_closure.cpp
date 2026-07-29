#include "utils.h"

#include <benchmark/benchmark.h>

#include <cstdint>

namespace
{
constexpr int ITERATION_COUNT = 50'000;

template<typename Callable>
NOINLINE std::int64_t compute_with_function(Callable&& function)
{
    std::int64_t sum = 0;
    for (int index = 0; index < ITERATION_COUNT; ++index) {
        sum += function();
    }
    return sum;
}
}  // namespace

namespace ref_capture_variant
{
void BM_ref_capture(benchmark::State& state)
{
    for (auto _ : state) {
        int value = 42;
        benchmark::DoNotOptimize(value);
        auto lambda = [&value] { return value * value + value; };
        auto result = compute_with_function(lambda);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ref_capture)->Name("LambdaClosure/ref_capture/50000");
}  // namespace ref_capture_variant

namespace value_capture_variant
{
void BM_value_capture(benchmark::State& state)
{
    for (auto _ : state) {
        int value = 42;
        benchmark::DoNotOptimize(value);
        auto lambda = [value] { return value * value + value; };
        auto result = compute_with_function(lambda);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_value_capture)->Name("LambdaClosure/value_capture/50000");
}  // namespace value_capture_variant

BENCHMARK_MAIN();
