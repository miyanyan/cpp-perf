#include <benchmark/benchmark.h>

#include <cstdint>

namespace
{
std::uint64_t global_counter = 0;

struct Data
{
    std::uint64_t x = global_counter++;
};

inline Data* get_local_static_data()
{
    static Data data{};
    return &data;
}

Data GLOBAL_DATA{};

inline Data* get_global_data()
{
    return &GLOBAL_DATA;
}
}  // namespace

void BM_local_static(benchmark::State& state)
{
    auto* initialized_data = get_local_static_data();
    benchmark::DoNotOptimize(initialized_data);
    for (auto _ : state) {
        auto* data = get_local_static_data();
        benchmark::DoNotOptimize(data->x);
    }
}
BENCHMARK(BM_local_static)->Name("AccessPattern/local_static");

void BM_global_access(benchmark::State& state)
{
    for (auto _ : state) {
        auto* data = get_global_data();
        benchmark::DoNotOptimize(data->x);
    }
}
BENCHMARK(BM_global_access)->Name("AccessPattern/global_access");

BENCHMARK_MAIN();
