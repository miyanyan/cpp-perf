#include "utils.h"

#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace
{
// char* is an alias-eligible type: the compiler must assume writes through `p`
// may modify *probe, so probe[0] is reloaded every iteration.
NOINLINE std::uint64_t run_char(char* p, std::uint32_t* probe, std::size_t n)
{
    std::uint64_t sum = 0;
    for (std::size_t i = 0; i < n; ++i) {
        p[i] = static_cast<char>(p[i] + 1);
        sum += probe[0];
    }
    return sum;
}

// char8_t* is a distinct type: the compiler may assume it does not alias
// uint32_t*, so probe[0] can be hoisted out of the loop.
NOINLINE std::uint64_t run_char8(char8_t* p, std::uint32_t* probe, std::size_t n)
{
    std::uint64_t sum = 0;
    for (std::size_t i = 0; i < n; ++i) {
        p[i] = static_cast<char8_t>(p[i] + char8_t{1});
        sum += probe[0];
    }
    return sum;
}
}  // namespace

static void BM_AliasingChar(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    std::vector<char> buf(n, 'a');
    std::vector<std::uint32_t> probe(1, 0x12345678u);
    for (auto _ : state) {
        auto v = run_char(buf.data(), probe.data(), n);
        benchmark::DoNotOptimize(v);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(n));
}
BENCHMARK(BM_AliasingChar)->Range(1 << 6, 1 << 16)->Name("AliasingModel/char*");

static void BM_AliasingChar8(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    std::vector<char8_t> buf(n, char8_t{'a'});
    std::vector<std::uint32_t> probe(1, 0x12345678u);
    for (auto _ : state) {
        auto v = run_char8(buf.data(), probe.data(), n);
        benchmark::DoNotOptimize(v);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(n));
}
BENCHMARK(BM_AliasingChar8)->Range(1 << 6, 1 << 16)->Name("AliasingModel/char8_t*");

BENCHMARK_MAIN();
