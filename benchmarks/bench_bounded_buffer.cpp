#include <benchmark/benchmark.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#if __has_include(<inplace_vector>)
#    include <inplace_vector>
#endif

namespace
{
constexpr std::size_t CAPACITY = 32;

#if defined(__cpp_lib_inplace_vector) && __cpp_lib_inplace_vector >= 202406L
template<typename T, std::size_t N>
using InplaceVector = std::inplace_vector<T, N>;
#else
template<typename T, std::size_t N>
class InplaceVector
{
public:
    void push_back(const T& value)
    {
        data_[size_] = value;
        ++size_;
    }

    const T& operator[](std::size_t index) const { return data_[index]; }

    std::size_t size() const { return size_; }

    T* begin() { return data_.data(); }

    T* end() { return data_.data() + size_; }

private:
    std::array<T, N> data_;
    std::size_t size_ = 0;
};
#endif
}  // namespace

namespace inplace_vector_variant
{
InplaceVector<std::uint32_t, CAPACITY> get_array(std::uint32_t start)
{
    InplaceVector<std::uint32_t, CAPACITY> values;
    for (std::size_t index = 0; index < CAPACITY; ++index) {
        values.push_back(static_cast<std::uint32_t>(index) * start + start);
    }
    return values;
}

void BM_inplace_vector(benchmark::State& state)
{
    std::uint32_t start = 0;
    for (auto _ : state) {
        auto values = get_array(++start);
        benchmark::DoNotOptimize(values);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(CAPACITY));
}
BENCHMARK(BM_inplace_vector)->Name("BoundedBuffer/inplace_vector/32");
}  // namespace inplace_vector_variant

namespace std_vector_variant
{
std::vector<std::uint32_t> get_array(std::uint32_t start)
{
    std::vector<std::uint32_t> values;
    values.reserve(CAPACITY);
    for (std::size_t index = 0; index < CAPACITY; ++index) {
        values.push_back(static_cast<std::uint32_t>(index) * start + start);
    }
    return values;
}

void BM_std_vector(benchmark::State& state)
{
    std::uint32_t start = 0;
    for (auto _ : state) {
        auto values = get_array(++start);
        benchmark::DoNotOptimize(values);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(CAPACITY));
}
BENCHMARK(BM_std_vector)->Name("BoundedBuffer/std_vector/32");
}  // namespace std_vector_variant

BENCHMARK_MAIN();
