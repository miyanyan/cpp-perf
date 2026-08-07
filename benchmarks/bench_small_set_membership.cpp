#include "utils.h"

#include <benchmark/benchmark.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <random>
#include <set>
#include <unordered_set>
#include <vector>

namespace
{
constexpr int QUERY_COUNT = 65'536;
constexpr std::array<int, 14> SET_SIZES = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128};

std::vector<int> make_elements(int size)
{
    std::vector<int> elements;
    elements.reserve(size);
    for (int index = 0; index < size; ++index) {
        elements.push_back(index * 3);
    }
    return elements;
}

std::vector<int> make_queries(int count, int set_size)
{
    std::vector<int> queries;
    queries.reserve(count);
    for (int index = 0; index < count / 2; ++index) {
        queries.push_back((index % set_size) * 3);
        queries.push_back((set_size + index % set_size) * 3);
    }
    std::mt19937 generator(0x5E7B'1234U);
    std::shuffle(queries.begin(), queries.end(), generator);
    return queries;
}

void apply_set_sizes(benchmark::Benchmark* benchmark)
{
    for (const int size : SET_SIZES) {
        benchmark->Arg(size);
    }
}
}  // namespace

namespace tree_set_variant
{
NOINLINE std::int64_t count_matches(const std::set<int>& values, const std::vector<int>& queries)
{
    std::int64_t found = 0;
    for (const int query : queries) {
        if (values.count(query) != 0) {
            ++found;
        }
    }
    return found;
}

void BM_tree_set(benchmark::State& state)
{
    const int size = static_cast<int>(state.range(0));
    const auto elements = make_elements(size);
    const auto queries = make_queries(QUERY_COUNT, size);
    const std::set<int> values(elements.begin(), elements.end());
    for (auto _ : state) {
        const auto* values_ptr = &values;
        const auto* queries_ptr = &queries;
        benchmark::DoNotOptimize(values_ptr);
        benchmark::DoNotOptimize(queries_ptr);
        auto found = count_matches(*values_ptr, *queries_ptr);
        benchmark::DoNotOptimize(found);
    }
    state.SetItemsProcessed(state.iterations() * QUERY_COUNT);
}
BENCHMARK(BM_tree_set)->Name("SmallSetMembership/tree_set")->Apply(apply_set_sizes);
}  // namespace tree_set_variant

namespace hash_set_variant
{
NOINLINE std::int64_t count_matches(const std::unordered_set<int>& values, const std::vector<int>& queries)
{
    std::int64_t found = 0;
    for (const int query : queries) {
        if (values.count(query) != 0) {
            ++found;
        }
    }
    return found;
}

void BM_hash_set(benchmark::State& state)
{
    const int size = static_cast<int>(state.range(0));
    const auto elements = make_elements(size);
    const auto queries = make_queries(QUERY_COUNT, size);
    const std::unordered_set<int> values(elements.begin(), elements.end());
    for (auto _ : state) {
        const auto* values_ptr = &values;
        const auto* queries_ptr = &queries;
        benchmark::DoNotOptimize(values_ptr);
        benchmark::DoNotOptimize(queries_ptr);
        auto found = count_matches(*values_ptr, *queries_ptr);
        benchmark::DoNotOptimize(found);
    }
    state.SetItemsProcessed(state.iterations() * QUERY_COUNT);
}
BENCHMARK(BM_hash_set)->Name("SmallSetMembership/hash_set")->Apply(apply_set_sizes);
}  // namespace hash_set_variant

BENCHMARK_MAIN();
