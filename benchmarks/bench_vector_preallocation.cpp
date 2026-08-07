#include "utils.h"

#include <benchmark/benchmark.h>

#include <vector>

namespace
{
constexpr int VECTOR_SIZE = 10'000;
}

namespace reserve_variant
{
NOINLINE std::vector<int> build_vector(int size)
{
    std::vector<int> values;
    values.reserve(size);
    for (int index = 0; index < size; ++index) {
        values.push_back(index * 2);
    }
    return values;
}

void BM_reserve_push_back(benchmark::State& state)
{
    const auto size = static_cast<int>(state.range(0));
    for (auto _ : state) {
        auto values = build_vector(size);
        benchmark::DoNotOptimize(values);
    }
    state.SetItemsProcessed(state.iterations() * size);
}
BENCHMARK(BM_reserve_push_back)->Name("VectorPreallocation/reserve_push_back")->Arg(VECTOR_SIZE);
}  // namespace reserve_variant

namespace resize_variant
{
NOINLINE std::vector<int> build_vector(int size)
{
    std::vector<int> values;
    values.resize(size);
    for (int index = 0; index < size; ++index) {
        values[index] = index * 2;
    }
    return values;
}

void BM_resize_index(benchmark::State& state)
{
    const auto size = static_cast<int>(state.range(0));
    for (auto _ : state) {
        auto values = build_vector(size);
        benchmark::DoNotOptimize(values);
    }
    state.SetItemsProcessed(state.iterations() * size);
}
BENCHMARK(BM_resize_index)->Name("VectorPreallocation/resize_index")->Arg(VECTOR_SIZE);
}  // namespace resize_variant

BENCHMARK_MAIN();
