#include "utils.h"

#include <benchmark/benchmark.h>

#include <array>
#include <cstdint>
#include <vector>

namespace
{
// Compile-time lookup table: built once, lives in static storage (.rodata).
constexpr std::array<uint8_t, 256> makeTable()
{
    std::array<uint8_t, 256> t{};
    for (int i = 0; i < 256; ++i) {
        t[i] = static_cast<uint8_t>((i * 73 + 19) & 0xFF);
    }
    return t;
}

// Static version: one indirect load from a 256-byte table in static storage.
// NOINLINE ensures the call site cannot see through to hoist the lookup.
NOINLINE uint8_t transform_static(uint8_t c)
{
    static constexpr auto TABLE = makeTable();
    return TABLE[c];
}

// Stack version: rebuilds the 256-byte table on the stack every call, then
// loads one entry. NOINLINE prevents the compiler from hoisting the rebuild
// out of the query loop (otherwise both variants collapse to plain lookups).
NOINLINE uint8_t transform_stack(uint8_t c)
{
    std::array<uint8_t, 256> table;
    for (int i = 0; i < 256; ++i) {
        table[i] = static_cast<uint8_t>((i * 73 + 19) & 0xFF);
    }
    return table[c];
}

// Static vector: built once at first call (via thread-safe static-local
// init), then each subsequent call pays (a) a guard-variable check and
// (b) an indirection through the heap-allocated buffer.
NOINLINE uint8_t transform_static_vector(uint8_t c)
{
    static const std::vector<uint8_t> TABLE = [] {
        std::vector<uint8_t> t(256);
        for (int i = 0; i < 256; ++i) {
            t[i] = static_cast<uint8_t>((i * 73 + 19) & 0xFF);
        }
        return t;
    }();
    return TABLE[c];
}
}  // namespace

static void BM_LookupTableStatic(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    std::vector<uint8_t> queries(n);
    for (std::size_t i = 0; i < n; ++i) {
        queries[i] = static_cast<uint8_t>(i);
    }
    for (auto _ : state) {
        uint32_t sum = 0;
        for (uint8_t q : queries) {
            sum += transform_static(q);
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}
BENCHMARK(BM_LookupTableStatic)->Range(1 << 4, 1 << 14)->Name("LookupTable/static");

static void BM_LookupTableStack(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    std::vector<uint8_t> queries(n);
    for (std::size_t i = 0; i < n; ++i) {
        queries[i] = static_cast<uint8_t>(i);
    }
    for (auto _ : state) {
        uint32_t sum = 0;
        for (uint8_t q : queries) {
            sum += transform_stack(q);
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}
BENCHMARK(BM_LookupTableStack)->Range(1 << 4, 1 << 14)->Name("LookupTable/stack");

static void BM_LookupTableStaticVector(benchmark::State& state)
{
    const auto n = static_cast<std::size_t>(state.range(0));
    std::vector<uint8_t> queries(n);
    for (std::size_t i = 0; i < n; ++i) {
        queries[i] = static_cast<uint8_t>(i);
    }
    for (auto _ : state) {
        uint32_t sum = 0;
        for (uint8_t q : queries) {
            sum += transform_static_vector(q);
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}
BENCHMARK(BM_LookupTableStaticVector)->Range(1 << 4, 1 << 14)->Name("LookupTable/static_vector");

BENCHMARK_MAIN();
