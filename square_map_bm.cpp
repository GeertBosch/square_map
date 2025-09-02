/**
 * Benchmarks comparing various map implementations including:
 * - std::map
 * - boost::container::flat_map
 * - square_map
 */

#include <benchmark/benchmark.h>

#include <boost/container/flat_map.hpp>
#include <cstdint>
#include <map>
#include <random>

#include "square_map.h"

namespace {

constexpr uint32_t kMinContainerSize = 11;
constexpr uint32_t kMaxContainerSize = 1'100'000;
constexpr uint32_t kMaxSlowContainerSize = kMaxContainerSize / 10;  // Smaller size for O(n) ops

constexpr size_t kPreallocSize = 16;

template <class Key, class T, class Compare = std::less<Key>>
using flat_map = boost::container::flat_map<Key, T, Compare>;

using std_map_int = std::map<uint32_t, bool>;
using flat_map_int = flat_map<uint32_t, bool>;
using square_map_int = geert::square_map<uint32_t, bool>;

// Sequential generator (1,2,3,...)
struct SequentialGenerator {
    uint32_t generateInteger() { return counter++; }
    uint32_t counter = 0;
};

// Random number generator
struct RandomGenerator {
    uint32_t generateInteger() { return dist(rng); }

    std::mt19937 rng;
    std::uniform_int_distribution<uint32_t> dist;
};

// Insert benchmark
template <typename Map, typename Generator>
static void BM_Insert(benchmark::State& state) {
    const size_t N = state.range(0);

    // Generate all keys upfront to avoid generator overhead in timing
    Generator gen;
    std::vector<typename Map::key_type> keys;
    keys.reserve(N);
    for (size_t i = 0; i < N; i++) {
        keys.push_back(gen.generateInteger());
    }

    for (auto _ : state) {
        Map m;

        for (size_t i = 0; i < N; i++) {
            benchmark::DoNotOptimize(m[keys[i]] = true);
        }
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * N);
    state.counters["time_per_item"] = benchmark::Counter(
        state.iterations() * N, benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
}

template <typename Map, typename Generator>
static void BM_Lookup(benchmark::State& state) {
    const size_t N = state.range(0);

    // Setup map with N elements
    Generator gen;
    Map m;
    std::vector<typename Map::key_type> keys;
    keys.reserve(N);
    for (size_t i = 0; i < N; i++) {
        auto key = gen.generateInteger();
        m[key] = true;
        keys.push_back(key);
    } 

    size_t idx = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(m.find(keys[idx]));
        if (++idx >= N) idx = 0;
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["time_per_item"] = benchmark::Counter(
        state.iterations(), benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
}

// Range iteration benchmark
template <typename Map, typename Generator>
static void BM_RangeIteration(benchmark::State& state) {
    const size_t N = state.range(0);

    // Setup map with N elements
    Generator gen;
    Map m;
    for (size_t i = 0; i < N; i++) {
        m[gen.generateInteger()] = true;
    }

    for (auto _ : state) {
        size_t sum = 0;
        for (const auto& [key, value] : m) {
            benchmark::DoNotOptimize(sum += value);
        }
        benchmark::DoNotOptimize(sum);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * N);
    state.counters["time_per_item"] = benchmark::Counter(
        state.iterations() * N, benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
}

// Helper to determine the appropriate max size for Insert benchmarks with RandomGenerator
template <typename Map, typename Generator>
constexpr uint32_t GetInsertMaxSize() {
    if constexpr (std::is_same_v<Generator, RandomGenerator> &&
                  (std::is_same_v<Map, flat_map_int>)) {
        return kMaxSlowContainerSize;
    } else {
        return kMaxContainerSize;
    }
}

// Helper macro to determine the correct max size for insert benchmarks
#define GET_INSERT_MAX_SIZE(MapType, GeneratorType)                                           \
    (std::is_same_v<MapType, flat_map_int> && std::is_same_v<GeneratorType, RandomGenerator>) \
        ? kMaxSlowContainerSize                                                               \
        : kMaxContainerSize

// Macro to register all benchmark types for a given Map and Generator combination
#define REGISTER_BENCHMARK_SET(MapType, GeneratorType)                           \
    BENCHMARK_TEMPLATE(BM_Insert, MapType, GeneratorType)                        \
        ->Range(kMinContainerSize, GET_INSERT_MAX_SIZE(MapType, GeneratorType)); \
    BENCHMARK_TEMPLATE(BM_Lookup, MapType, GeneratorType)                        \
        ->Range(kMinContainerSize, GET_INSERT_MAX_SIZE(MapType, GeneratorType)); \
    BENCHMARK_TEMPLATE(BM_RangeIteration, MapType, GeneratorType)                \
        ->Range(kMinContainerSize, GET_INSERT_MAX_SIZE(MapType, GeneratorType));

// Register all benchmarks with proper type names and DRY principle
void RegisterAllBenchmarks() {
    // Integer map benchmarks
    REGISTER_BENCHMARK_SET(std_map_int, SequentialGenerator);
    REGISTER_BENCHMARK_SET(std_map_int, RandomGenerator);
    REGISTER_BENCHMARK_SET(flat_map_int, SequentialGenerator);
    REGISTER_BENCHMARK_SET(flat_map_int, RandomGenerator);
    REGISTER_BENCHMARK_SET(square_map_int, SequentialGenerator);
    REGISTER_BENCHMARK_SET(square_map_int, RandomGenerator);
}

struct BenchmarkRegistrar {
    BenchmarkRegistrar() {
        RegisterAllBenchmarks();
    }
} benchmark_registrar;

}  // namespace

BENCHMARK_MAIN();
