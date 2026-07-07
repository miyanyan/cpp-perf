#include <benchmark/benchmark.h>

#include <string>
#include <string_view>

static void BM_StringCreation(benchmark::State& state)
{
    for (auto _ : state) {
        std::string s(static_cast<size_t>(state.range(0)), 'x');
        benchmark::DoNotOptimize(s);
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(state.range(0)));
}
BENCHMARK(BM_StringCreation)->Range(8, 8 << 10);

static void BM_StringCopy(benchmark::State& state)
{
    std::string src(static_cast<size_t>(state.range(0)), 'x');
    for (auto _ : state) {
        std::string dst = src;
        benchmark::DoNotOptimize(dst);
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(state.range(0)));
}
BENCHMARK(BM_StringCopy)->Range(8, 8 << 10);

static void BM_StringViewSubstr(benchmark::State& state)
{
    std::string s(static_cast<size_t>(state.range(0)), 'x');
    std::string_view sv(s);
    for (auto _ : state) {
        auto sub = sv.substr(0, sv.size() / 2);
        benchmark::DoNotOptimize(sub);
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(state.range(0)));
}
BENCHMARK(BM_StringViewSubstr)->Range(8, 8 << 10);

BENCHMARK_MAIN();
