#include <benchmark/benchmark.h>

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace
{
constexpr int MAP_SIZE = 512;

std::vector<std::string> make_keys(std::string_view prefix)
{
    std::vector<std::string> keys;
    keys.reserve(MAP_SIZE);
    for (int index = 0; index < MAP_SIZE; ++index) {
        keys.emplace_back(prefix);
        keys.back() += std::to_string(index);
    }
    return keys;
}

// 12-14 bytes: within the common 15-byte std::string SSO capacity.
const std::vector<std::string> SSO_KEYS = make_keys("lookup-key-");

// Longer than the SSO capacity used by libstdc++, libc++, and MSVC STL.
const std::vector<std::string> HEAP_KEYS = make_keys("lookup-key-with-padding-that-exceeds-small-string-capacity-");
}  // namespace

namespace transparent_variant
{
using Map = std::map<std::string, int, std::less<>>;

Map make_map(const std::vector<std::string>& keys)
{
    Map map;
    for (int index = 0; index < MAP_SIZE; ++index) {
        map.emplace(keys[index], index);
    }
    return map;
}

int lookup(const Map& map, std::string_view key)
{
    const auto iterator = map.find(key);
    return iterator != map.end() ? iterator->second : -1;
}

const Map SSO_MAP = make_map(SSO_KEYS);
const Map HEAP_MAP = make_map(HEAP_KEYS);

void run_benchmark(benchmark::State& state, const Map& map, const std::vector<std::string>& keys)
{
    for (auto _ : state) {
        std::int64_t total = 0;
        for (const auto& key : keys) {
            total += lookup(map, std::string_view{key});
        }
        benchmark::DoNotOptimize(total);
    }
    state.SetItemsProcessed(state.iterations() * MAP_SIZE);
}

void BM_transparent_sso(benchmark::State& state)
{
    run_benchmark(state, SSO_MAP, SSO_KEYS);
}
BENCHMARK(BM_transparent_sso)->Name("MapStringLookup/transparent/sso");

void BM_transparent_heap(benchmark::State& state)
{
    run_benchmark(state, HEAP_MAP, HEAP_KEYS);
}
BENCHMARK(BM_transparent_heap)->Name("MapStringLookup/transparent/heap");
}  // namespace transparent_variant

namespace opaque_variant
{
using Map = std::map<std::string, int>;

Map make_map(const std::vector<std::string>& keys)
{
    Map map;
    for (int index = 0; index < MAP_SIZE; ++index) {
        map.emplace(keys[index], index);
    }
    return map;
}

int lookup(const Map& map, std::string_view key)
{
    const auto iterator = map.find(std::string{key});
    return iterator != map.end() ? iterator->second : -1;
}

const Map SSO_MAP = make_map(SSO_KEYS);
const Map HEAP_MAP = make_map(HEAP_KEYS);

void run_benchmark(benchmark::State& state, const Map& map, const std::vector<std::string>& keys)
{
    for (auto _ : state) {
        std::int64_t total = 0;
        for (const auto& key : keys) {
            total += lookup(map, std::string_view{key});
        }
        benchmark::DoNotOptimize(total);
    }
    state.SetItemsProcessed(state.iterations() * MAP_SIZE);
}

void BM_opaque_sso(benchmark::State& state)
{
    run_benchmark(state, SSO_MAP, SSO_KEYS);
}
BENCHMARK(BM_opaque_sso)->Name("MapStringLookup/opaque/sso");

void BM_opaque_heap(benchmark::State& state)
{
    run_benchmark(state, HEAP_MAP, HEAP_KEYS);
}
BENCHMARK(BM_opaque_heap)->Name("MapStringLookup/opaque/heap");
}  // namespace opaque_variant

BENCHMARK_MAIN();
