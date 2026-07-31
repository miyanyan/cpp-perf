#include "utils.h"

#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace
{
constexpr int DATA_SIZE = 65'536;

std::vector<double> make_data()
{
    std::vector<double> values(DATA_SIZE);
    for (int index = 0; index < DATA_SIZE; ++index) {
        values[index] = 1.0 + static_cast<double>(index) * 0.000001;
    }
    return values;
}

const std::vector<double> DATA = make_data();
}  // namespace

namespace single_accumulator
{
NOINLINE double sum_array(const std::vector<double>& data)
{
    double sum = 0.0;
    const std::size_t size = data.size();
    std::size_t index = 0;
    for (; index + 3 < size; index += 4) {
        sum += data[index];
        sum += data[index + 1];
        sum += data[index + 2];
        sum += data[index + 3];
    }
    for (; index < size; ++index) {
        sum += data[index];
    }
    return sum;
}

void BM_single_sum(benchmark::State& state)
{
    for (auto _ : state) {
        const auto* data = &DATA;
        benchmark::DoNotOptimize(data);
        auto result = sum_array(*data);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * DATA_SIZE);
}
BENCHMARK(BM_single_sum)->Name("LoopCarriedDependency/single_sum/65536");
}  // namespace single_accumulator

namespace split_accumulator
{
NOINLINE double sum_array(const std::vector<double>& data)
{
    double sum0 = 0.0;
    double sum1 = 0.0;
    double sum2 = 0.0;
    double sum3 = 0.0;
    const std::size_t size = data.size();
    std::size_t index = 0;
    for (; index + 3 < size; index += 4) {
        sum0 += data[index];
        sum1 += data[index + 1];
        sum2 += data[index + 2];
        sum3 += data[index + 3];
    }
    for (; index < size; ++index) {
        sum0 += data[index];
    }
    return sum0 + sum1 + sum2 + sum3;
}

void BM_split_sum(benchmark::State& state)
{
    for (auto _ : state) {
        const auto* data = &DATA;
        benchmark::DoNotOptimize(data);
        auto result = sum_array(*data);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * DATA_SIZE);
}
BENCHMARK(BM_split_sum)->Name("LoopCarriedDependency/split_sum/65536");
}  // namespace split_accumulator

BENCHMARK_MAIN();
