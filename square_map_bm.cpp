/**
 * Benchmarks comparing various map implementations including:
 * - std::map
 * - boost::container::flat_map
 * - square_map
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <boost/container/flat_map.hpp>
#include <cstdint>
#include <iostream>
#include <map>
#include <random>
#include <vector>

#ifndef DEBUG
#define DEBUG 0
#endif

constexpr bool kDebug = 0;

#include "square_map.h"

namespace {

constexpr uint32_t kMinContainerSize = 8;
constexpr uint32_t kMaxContainerSize = kDebug ? 100'000 : 1'000'000;
constexpr uint32_t kMaxSlowContainerSize = kMaxContainerSize / 10;  // Smaller size for O(n) ops

template <class Key, class T, class Compare = std::less<Key>>
using flat_map = boost::container::flat_map<Key, T, Compare>;

using std_map_int = std::map<uint32_t, bool>;
using flat_map_int = flat_map<uint32_t, bool>;
using square_map_int = geert::square_map<uint32_t, bool>;

// Key order type for benchmarks
enum class KeyOrder { Sequential, Random };

// Helper function to generate keys - sequential numbers, optionally shuffled
template <typename KeyType>
std::vector<KeyType> generateKeys(size_t N, KeyOrder order) {
    std::vector<KeyType> keys;
    keys.reserve(N);

    // Generate sequential numbers starting from 1
    for (size_t i = 0; i < N; i++) {
        keys.push_back(static_cast<KeyType>(i + 1));
    }

    // Shuffle if random order is requested
    if (order == KeyOrder::Random) {
        static std::mt19937 rng;
        std::shuffle(keys.begin(), keys.end(), rng);
    }

    return keys;
}

// A map built by BM_Insert along with the keys used to build it, so that the
// Lookup and RangeIteration benchmarks can reuse it instead of rebuilding.
template <typename Map>
struct BuiltMap {
    std::vector<typename Map::key_type> keys;
    Map map;
};

// Cache of built maps by size, one per Map/Order combination. The insert benchmarks run
// first for all sizes, so entries stay around until the later benchmarks consume them.
template <typename Map, KeyOrder Order>
BuiltMap<Map>& builtMapCache(size_t N) {
    static std::map<size_t, BuiltMap<Map>> cache;
    return cache[N];
}

// Returns the cached map for size N, building it if BM_Insert did not run (e.g. filtered out)
template <typename Map, KeyOrder Order>
const BuiltMap<Map>& getBuiltMap(size_t N) {
    auto& built = builtMapCache<Map, Order>(N);
    if (!built.keys.empty()) return built;

    built.keys = generateKeys<typename Map::key_type>(N, Order);
    for (const auto& key : built.keys) built.map[key] = true;
    return built;
}

// Insert benchmark
template <typename Map, KeyOrder Order>
static void BM_Insert(benchmark::State& state) {
    const size_t N = state.range(0);

    // Generate all keys upfront to avoid generator overhead in timing
    auto keys = generateKeys<typename Map::key_type>(N, Order);
    Map m;

    for (auto _ : state) {
        m = Map();

        for (size_t i = 0; i < N; i++) {
            benchmark::DoNotOptimize(m[keys[i]] = true);
        }
        benchmark::ClobberMemory();
    }

    // Keep the last map built for the Lookup and RangeIteration benchmarks
    auto& built = builtMapCache<Map, Order>(N);
    built.keys = std::move(keys);
    built.map = std::move(m);

    state.SetItemsProcessed(state.iterations() * N);
    state.counters["time_per_item"] = benchmark::Counter(
        state.iterations() * N, benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
}

template <typename Map, KeyOrder Order>
static void BM_Lookup(benchmark::State& state) {
    const size_t N = state.range(0);

    const auto& [keys, m] = getBuiltMap<Map, Order>(N);

    size_t idx = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(m.at(keys[idx]));
        if (++idx >= N) idx = 0;
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["time_per_item"] = benchmark::Counter(
        state.iterations(), benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
}

// Range iteration benchmark
template <typename Map, KeyOrder Order>
static void BM_RangeIteration(benchmark::State& state) {
    const size_t N = state.range(0);

    const auto& m = getBuiltMap<Map, Order>(N).map;

    for (auto _ : state) {
        size_t sum = 0;
        for (auto&& [key, value] : m) {
            benchmark::DoNotOptimize(sum += value);
        }
        benchmark::DoNotOptimize(sum);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * N);
    state.counters["time_per_item"] = benchmark::Counter(
        state.iterations() * N, benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
}

// Helper to determine the appropriate max size for Insert benchmarks with Random order
template <typename Map, KeyOrder Order>
constexpr uint32_t GetInsertMaxSize() {
    if constexpr (Order == KeyOrder::Random && (std::is_same_v<Map, flat_map_int>)) {
        return kMaxSlowContainerSize;
    } else {
        return kMaxContainerSize;
    }
}

// Helper macro to determine the correct max size for insert benchmarks
#define GET_INSERT_MAX_SIZE(MapType, OrderType)                              \
    (std::is_same_v<MapType, flat_map_int> && OrderType == KeyOrder::Random) \
        ? kMaxSlowContainerSize                                              \
        : kMaxContainerSize

// Macro to register all benchmark types for a given Map and KeyOrder combination
#define REGISTER_BENCHMARK_SET(MapType, OrderType)                           \
    BENCHMARK_TEMPLATE(BM_Insert, MapType, OrderType)                        \
        ->Range(kMinContainerSize, GET_INSERT_MAX_SIZE(MapType, OrderType)); \
    BENCHMARK_TEMPLATE(BM_Lookup, MapType, OrderType)                        \
        ->Range(kMinContainerSize, GET_INSERT_MAX_SIZE(MapType, OrderType)); \
    BENCHMARK_TEMPLATE(BM_RangeIteration, MapType, OrderType)                \
        ->Range(kMinContainerSize, GET_INSERT_MAX_SIZE(MapType, OrderType));

// Register all benchmarks with proper type names and DRY principle
void RegisterAllBenchmarks() {
    // Integer map benchmarks
    REGISTER_BENCHMARK_SET(std_map_int, KeyOrder::Sequential);
    REGISTER_BENCHMARK_SET(std_map_int, KeyOrder::Random);
    REGISTER_BENCHMARK_SET(flat_map_int, KeyOrder::Sequential);
    REGISTER_BENCHMARK_SET(flat_map_int, KeyOrder::Random);
    REGISTER_BENCHMARK_SET(square_map_int, KeyOrder::Sequential);
    REGISTER_BENCHMARK_SET(square_map_int, KeyOrder::Random);
}

struct BenchmarkRegistrar {
    BenchmarkRegistrar() {
        RegisterAllBenchmarks();
    }
} benchmark_registrar;

}  // namespace

int main(int argc, char** argv) {
    if (kDebug) {
        std::cerr << "WARNING: Benchmark compiled in DEBUG mode! Results may be unreliable.\n"
                  << "For accurate performance measurements, compile with optimization flags.\n"
                  << "----------------------------------------\n\n";
    }

    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
