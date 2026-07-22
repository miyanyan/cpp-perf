#include "utils.h"

#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <random>
#include <utility>
#include <vector>

namespace
{
constexpr std::uint32_t DATA_COUNT = 1U << 20;
constexpr std::size_t SITE_COUNT = 4096;
constexpr std::size_t CHUNK_SIZE = 256;

static_assert((DATA_COUNT & (DATA_COUNT - 1)) == 0, "DATA_COUNT must be a power of two");
static_assert(SITE_COUNT % CHUNK_SIZE == 0, "SITE_COUNT must be divisible by CHUNK_SIZE");

std::vector<std::uint32_t> make_raw_data()
{
    std::mt19937 rng(12345);
    rng.discard(700);
    std::uniform_int_distribution<std::uint32_t> dist(0, UINT32_MAX);
    std::vector<std::uint32_t> data(DATA_COUNT);
    for (auto& value : data) {
        value = dist(rng);
    }
    return data;
}

const std::vector<std::uint32_t> RAW_DATA = make_raw_data();

bool is_failure(std::uint32_t value, std::size_t salt)
{
    constexpr std::uint64_t MIX = 0x9E3779B97F4A7C15ULL;
    return ((static_cast<std::uint64_t>(value) ^ (salt * MIX)) & 0x3FFULL) == 0;
}

std::uint64_t handle_failure(std::uint64_t value)
{
    std::uint64_t result = value * 6364136223846793005ULL + 1ULL;
    result ^= result >> 33;
    result *= 0xFF51AFD7ED558CCDULL;
    return result;
}
}  // namespace

namespace has_failure_api
{
struct Result
{
    bool hasFailure;
    std::uint32_t value;
};

std::vector<Result> make_data()
{
    std::vector<Result> data;
    data.reserve(RAW_DATA.size());
    std::size_t salt = 1;
    for (const std::uint32_t value : RAW_DATA) {
        data.push_back(Result{is_failure(value, salt), value});
        ++salt;
    }
    return data;
}

const std::vector<Result> DATA = make_data();

template<std::size_t Site>
NOINLINE std::uint64_t check_value(const Result& result, std::uint64_t accumulator)
{
    if (result.hasFailure) {
        accumulator += handle_failure(result.value ^ Site);
    }
    else {
        accumulator += result.value;
    }
    return accumulator;
}

template<std::size_t Base, std::size_t... I>
std::uint64_t process_chunk(std::index_sequence<I...>, std::size_t& data_index, std::uint64_t accumulator)
{
    ((accumulator = check_value<Base + I>(DATA[data_index], accumulator),
      data_index = (data_index + 1) & (DATA_COUNT - 1)),
     ...);
    return accumulator;
}

template<std::size_t... Chunks>
std::uint64_t process_all(std::index_sequence<Chunks...>, std::size_t& data_index, std::uint64_t accumulator)
{
    ((accumulator =
          process_chunk<Chunks * CHUNK_SIZE>(std::make_index_sequence<CHUNK_SIZE>{}, data_index, accumulator)),
     ...);
    return accumulator;
}

void BM_has_failure(benchmark::State& state)
{
    std::size_t index = 0;
    std::uint64_t accumulator = 0;
    for (auto _ : state) {
        std::size_t data_index = index;
        accumulator = process_all(std::make_index_sequence<SITE_COUNT / CHUNK_SIZE>{}, data_index, accumulator);
        index = data_index;
        benchmark::DoNotOptimize(accumulator);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(SITE_COUNT));
}
BENCHMARK(BM_has_failure)->Name("ApiShape/has_failure/4096");
}  // namespace has_failure_api

namespace has_value_api
{
struct Result
{
    bool hasValue;
    std::uint32_t value;
};

std::vector<Result> make_data()
{
    std::vector<Result> data;
    data.reserve(RAW_DATA.size());
    std::size_t salt = 1;
    for (const std::uint32_t value : RAW_DATA) {
        data.push_back(Result{!is_failure(value, salt), value});
        ++salt;
    }
    return data;
}

const std::vector<Result> DATA = make_data();

template<std::size_t Site>
NOINLINE std::uint64_t check_value(const Result& result, std::uint64_t accumulator)
{
    if (!result.hasValue) {
        accumulator += handle_failure(result.value ^ Site);
    }
    else {
        accumulator += result.value;
    }
    return accumulator;
}

template<std::size_t Base, std::size_t... I>
std::uint64_t process_chunk(std::index_sequence<I...>, std::size_t& data_index, std::uint64_t accumulator)
{
    ((accumulator = check_value<Base + I>(DATA[data_index], accumulator),
      data_index = (data_index + 1) & (DATA_COUNT - 1)),
     ...);
    return accumulator;
}

template<std::size_t... Chunks>
std::uint64_t process_all(std::index_sequence<Chunks...>, std::size_t& data_index, std::uint64_t accumulator)
{
    ((accumulator =
          process_chunk<Chunks * CHUNK_SIZE>(std::make_index_sequence<CHUNK_SIZE>{}, data_index, accumulator)),
     ...);
    return accumulator;
}

void BM_has_value(benchmark::State& state)
{
    std::size_t index = 0;
    std::uint64_t accumulator = 0;
    for (auto _ : state) {
        std::size_t data_index = index;
        accumulator = process_all(std::make_index_sequence<SITE_COUNT / CHUNK_SIZE>{}, data_index, accumulator);
        index = data_index;
        benchmark::DoNotOptimize(accumulator);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(SITE_COUNT));
}
BENCHMARK(BM_has_value)->Name("ApiShape/has_value/4096");
}  // namespace has_value_api

BENCHMARK_MAIN();
