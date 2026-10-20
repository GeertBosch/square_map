#include <algorithm>
#include <atomic>
#include <boost/container/flat_map.hpp>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "square_map.h"

constexpr bool kDebug = false;
constexpr double kConfidenceThreshold = 0.80;  // Threshold for strong confidence in complexity
constexpr double kSeparationThreshold = 0.20;  // Minimum difference between best and second best

// Don't test small sizes, as there complexity is different, but with small constants
const std::vector<int> test_sizes = {8'000, 16'000, 32'000, 64'000, 128'000};

// Instrumented type to count operations
class InstrumentedInt {
public:
    static std::atomic<size_t> copy_count;
    static std::atomic<size_t> move_count;
    static std::atomic<size_t> comparison_count;
    static size_t total_writes() { return copy_count.load() + move_count.load(); }

    int value;

    // Default constructor
    InstrumentedInt() : value(0) {}

    // Value constructor
    explicit InstrumentedInt(int v) : value(v) {}

    // Copy constructor
    InstrumentedInt(const InstrumentedInt& other) : value(other.value) {
        copy_count++;
    }

    // Move constructor
    InstrumentedInt(InstrumentedInt&& other) noexcept : value(other.value) {
        move_count++;
    }

    // Copy assignment
    InstrumentedInt& operator=(const InstrumentedInt& other) {
        if (this != &other) {
            value = other.value;
            copy_count++;
        }
        return *this;
    }

    // Move assignment
    InstrumentedInt& operator=(InstrumentedInt&& other) noexcept {
        if (this != &other) {
            value = other.value;
            move_count++;
        }
        return *this;
    }

    // Comparison operators
    bool operator<(const InstrumentedInt& other) const {
        comparison_count++;
        return value < other.value;
    }

    bool operator==(const InstrumentedInt& other) const {
        comparison_count++;
        return value == other.value;
    }

    bool operator!=(const InstrumentedInt& other) const {
        comparison_count++;
        return value != other.value;
    }

    bool operator<=(const InstrumentedInt& other) const {
        comparison_count++;
        return value <= other.value;
    }

    bool operator>(const InstrumentedInt& other) const {
        comparison_count++;
        return value > other.value;
    }

    bool operator>=(const InstrumentedInt& other) const {
        comparison_count++;
        return value >= other.value;
    }

    // Reset counters
    static void reset_counters() {
        copy_count = 0;
        move_count = 0;
        comparison_count = 0;
    }

    // Print current counts
    static void print_stats(const std::string& operation) {
        std::cout << operation << " - Copies: " << copy_count << ", Moves: " << move_count
                  << ", Total Writes: " << total_writes() << ", Comparisons: " << comparison_count
                  << std::endl;
    }
};

int totalFailures = 0;
int totalPasses = 0;

// Define static members
std::atomic<size_t> InstrumentedInt::copy_count{0};
std::atomic<size_t> InstrumentedInt::move_count{0};
std::atomic<size_t> InstrumentedInt::comparison_count{0};

// Structure to hold test results
struct TestResult {
    int n;
    double writes_per_insert;
    double comparisons_per_insert;
    double lookups_per_insert;
    double writes_per_erase;
    double writes_per_ends_erase;
};

// Check if ratio y/x is approximately constant (indicating true proportionality)
double calculate_ratio_consistency(const std::vector<double>& x, const std::vector<double>& y) {
    if (x.size() != y.size() || x.size() < 2) return 0.0;

    std::vector<double> ratios;
    for (size_t i = 0; i < x.size(); i++) {
        if (x[i] != 0) {
            ratios.push_back(y[i] / x[i]);
        }
    }

    if (ratios.empty()) return 0.0;

    // Calculate coefficient of variation (std dev / mean) for ratios
    double mean = 0;
    for (double ratio : ratios) {
        mean += ratio;
    }
    mean /= ratios.size();

    double variance = 0;
    for (double ratio : ratios) {
        variance += (ratio - mean) * (ratio - mean);
    }
    variance /= ratios.size();

    double std_dev = std::sqrt(variance);
    double cv = mean == 0 ? 1.0 : std_dev / std::abs(mean);

    // Return 1 - CV (so perfect consistency = 1.0, poor consistency = 0.0)
    return std::max(0.0, 1.0 - cv);
}

// Function to analyze complexity by testing correlation with different functions
void analyze_complexity(const std::vector<TestResult>& results, std::string operation,
                        std::string expected_complexity, bool header) {
    std::vector<double> n_values, operation_values;
    for (const auto& result : results) {
        n_values.push_back(result.n);
        if (operation == "Insert Writes") {
            operation_values.push_back(result.writes_per_insert);
        } else if (operation == "Insert Comps") {
            operation_values.push_back(result.comparisons_per_insert);
        } else if (operation == "Lookup Comps") {
            operation_values.push_back(result.lookups_per_insert);
        } else if (operation == "Erase Writes") {
            operation_values.push_back(result.writes_per_erase);
        } else if (operation == "Erase Ends Writes") {
            operation_values.push_back(result.writes_per_ends_erase);
        }
    }

    // Test different complexity functions
    std::vector<double> log_n, sqrt_n, linear_n;

    for (double n : n_values) {
        log_n.push_back(std::log2(n));
        sqrt_n.push_back(std::sqrt(n));
        linear_n.push_back(n);
    }

    // Calculate ratio consistency
    double ratio_const_log_n = calculate_ratio_consistency(log_n, operation_values);
    double ratio_const_sqrt_n = calculate_ratio_consistency(sqrt_n, operation_values);
    double ratio_const_linear_n = calculate_ratio_consistency(linear_n, operation_values);

    if (kDebug) {
        std::cout << "\n### "
                  << " Ratio Consistency Scores for " << operation << "\n"
                  << std::endl;
        std::cout << "| Complexity | Ratio Consistency |\n";
        std::cout << "|:----------:|:-----------------:|\n";
        std::cout << "| O(log n)   | " << std::fixed << std::setprecision(3) << std::setw(17)
                  << ratio_const_log_n << " |\n";
        std::cout << "|  O(√n)     | " << std::fixed << std::setprecision(3) << std::setw(17)
                  << ratio_const_sqrt_n << " |\n";
        std::cout << "|  O(n)      | " << std::fixed << std::setprecision(3) << std::setw(17)
                  << ratio_const_linear_n << " |\n";
        std::cout << std::endl;
    }

    // Find best fit using ratio consistency
    std::vector<std::pair<double, std::string>> ratio_results = {
        {ratio_const_log_n, "O(log n)"},
        {ratio_const_sqrt_n, "O(√n)"},
        {ratio_const_linear_n, "O(n)"}
    };

    std::stable_sort(ratio_results.rbegin(), ratio_results.rend());
    if (kDebug)
        std::cout << "**Best fit**: " << ratio_results[0].second
                  << ", ratio consistency = " << std::fixed << std::setprecision(3)
                  << ratio_results[0].first << "\n"
                  << std::endl;

    double margin = 0.0;
    bool failure = false;
    std::string complexity = ratio_results[0].second;

    // Print table header
    if (header) {
        std::cout << "| Pass | Operation      |  Actual  | Confidence | Margin | Expected |\n";
        std::cout << "|------|----------------|:--------:|:----------:|:------:|:--------:|\n";
    }

    // Prepare result table row
    std::string pass_mark, margin_str, confidence_str, actual_complexity;
    if (ratio_results.size() > 1) {
        margin = ratio_results[0].first - ratio_results[1].first;
        std::ostringstream margin_stream;
        if (margin < kSeparationThreshold)
            margin_stream << "⚠️ " << std::fixed << std::setprecision(2) << margin;
        else
            margin_stream << std::fixed << std::setprecision(2) << margin;
        margin_str = margin_stream.str();
    } else {
        margin_str = "-";
    }

    std::ostringstream confidence_stream;
    if (ratio_results[0].first < kConfidenceThreshold)
        confidence_stream << "⚠️ " << std::fixed << std::setprecision(2) << ratio_results[0].first;
    else
        confidence_stream << std::fixed << std::setprecision(2) << ratio_results[0].first;
    confidence_str = confidence_stream.str();

    actual_complexity =
        (ratio_results[0].first < kConfidenceThreshold) ? "Unclear" : ratio_results[0].second;

    bool pass = (actual_complexity == expected_complexity) &&
                (ratio_results[0].first >= kConfidenceThreshold) &&
                (margin >= kSeparationThreshold);

    pass_mark = pass ? "✅" : "❌";

    // Print result table row
    // if actual complexity includes the sqrt symbol, adjust widths accordingly
    int actual_complexity_width = 8 + 2 * (actual_complexity.find("√") != std::string::npos);
    int expected_complexity_width = 8 + 2 * (expected_complexity.find("√") != std::string::npos);
    size_t operation_width = 14;
    while (operation.length() < operation_width) operation += " ";

    std::cout << "|  " << std::setw(4) << pass_mark << " | " << operation << " | "
              << std::setw(actual_complexity_width) << actual_complexity << " | " << std::setw(10)
              << confidence_str << " | " << std::setw(6) << margin_str << " | "
              << std::setw(expected_complexity_width) << expected_complexity << " |" << std::endl;

    // Update global counters
    if (pass) {
        ++totalPasses;
    } else {
        ++totalFailures;
    }

    if (kDebug) {
        // Print detailed data for debugging
        std::cout << "\n### Data points\n" << std::endl;

        std::cout << std::setw(10) << "N" << std::setw(17) << operation << std::setw(8) << "log(n)"
                  << std::setw(12) << "√n" << std::setw(10) << "n" << std::endl;
        std::cout << "    " << std::string(52, '-') << std::endl;
        for (size_t i = 0; i < n_values.size(); i++) {
            std::cout << std::setw(10) << (int)n_values[i] << std::setw(15) << std::fixed
                      << std::setprecision(2) << operation_values[i] << std::setw(10) << std::fixed
                      << std::setprecision(2) << log_n[i] << std::setw(10) << std::fixed
                      << std::setprecision(2) << sqrt_n[i] << std::setw(10) << std::fixed
                      << std::setprecision(0) << linear_n[i] << std::endl;
        }
    }
}

// Returns writes per erase for erasing keys in the given order from a map built in ascending order,
// which for square_map results in a single unsplit range.
template <class Map>
double erase_writes(int n, const std::vector<int>& erase_order) {
    Map map;
    for (int i = 0; i < n; ++i) map.insert({InstrumentedInt(i), InstrumentedInt(i * 2)});

    InstrumentedInt::reset_counters();
    for (int i : erase_order) map.erase(map.find(InstrumentedInt(i)));
    return (double)InstrumentedInt::total_writes() / erase_order.size();
}

// Test function with multiple N values
template <template <typename Key, typename T> class MapType>
void test_map_complexity(std::string mapName, std::vector<std::string> expected_complexities) {
    std::vector<TestResult> results;

    std::cerr << "\n##  Complexity Test for " << mapName << "\n" << std::endl;

    for (int N : test_sizes) {
        bool reduce_complexity =
            std::find(expected_complexities.begin(), expected_complexities.end(), "O(n)") !=
            expected_complexities.end();
        N = reduce_complexity ? N / 10 : N;
        std::cerr << "⏳ Testing N = " << N << "...";
        if (reduce_complexity) std::cerr << " (reduced due to O(n) expected insert complexity)";
        std::flush(std::cerr);

        TestResult result;
        result.n = N;

        // Test insertions
        {
            InstrumentedInt::reset_counters();

            using map_type = MapType<InstrumentedInt, InstrumentedInt>;
            map_type smap;

            // Create shuffled order for insertion
            std::vector<int> insert_order;
            for (int i = 0; i < N; ++i) {
                insert_order.push_back(i);
            }
            std::mt19937 gen;
            std::shuffle(insert_order.begin(), insert_order.end(), gen);

            // Insert N elements in shuffled order
            for (int i : insert_order) {
                smap.insert({InstrumentedInt(i), InstrumentedInt(i * 2)});
            }

            result.writes_per_insert = (double)InstrumentedInt::total_writes() / N;
            result.comparisons_per_insert = (double)InstrumentedInt::comparison_count / N;

            // Test successful lookups on the same map
            InstrumentedInt::reset_counters();

            // Create shuffled order for lookups
            std::vector<int> lookup_order;
            for (int i = 0; i < N; ++i) {
                lookup_order.push_back(i);
            }
            std::shuffle(lookup_order.begin(), lookup_order.end(), gen);

            // Test lookups in shuffled order
            int found_count = 0;
            for (int i : lookup_order) {
                auto it = smap.find(InstrumentedInt(i));
                if (it != smap.end()) {
                    found_count++;
                }
            }

            result.lookups_per_insert = (double)InstrumentedInt::comparison_count / N;
        }

        // Test erasing half the elements in random order, and alternately from front and back.
        {
            using map_type = MapType<InstrumentedInt, InstrumentedInt>;
            std::vector<int> erase_order(N);
            std::iota(erase_order.begin(), erase_order.end(), 0);
            std::shuffle(erase_order.begin(), erase_order.end(), std::mt19937());
            erase_order.resize(N / 2);
            result.writes_per_erase = erase_writes<map_type>(N, erase_order);

            for (int i = 0; i < N / 2; ++i) erase_order[i] = i % 2 ? N - 1 - i / 2 : i / 2;
            result.writes_per_ends_erase = erase_writes<map_type>(N, erase_order);
        }
        std::cerr << "\r✔️\n";

        results.push_back(result);
    }

    // Analyze complexities
    std::cout << "\n### Complexity Analysis for " << mapName << "\n" << std::endl;
    // Maps may omit trailing expectations, e.g. std::map erase does no writes to classify.
    const std::vector<std::string> operations = {"Insert Writes", "Insert Comps", "Lookup Comps",
                                                 "Erase Writes", "Erase Ends Writes"};
    for (size_t i = 0; i < expected_complexities.size(); ++i)
        analyze_complexity(results, operations[i], expected_complexities[i], i == 0 || kDebug);

    // Print detailed results table
    std::cout << "\n### Detailed Results\n" << std::endl;
    std::cout << std::setw(12) << "N" << std::setw(15) << "Writes/Insert" << std::setw(20)
              << "Comparisons/Insert" << std::setw(20) << "Comparisons/Lookup" << std::setw(15)
              << "Writes/Erase" << std::setw(18) << "Writes/EraseEnds" << std::endl;
    std::cout << "    " << std::string(96, '-') << std::endl;

    for (const auto& result : results) {
        std::cout << std::setw(12) << result.n << std::setw(15) << std::fixed
                  << std::setprecision(2) << result.writes_per_insert << std::setw(20) << std::fixed
                  << std::setprecision(2) << result.comparisons_per_insert << std::setw(20)
                  << std::fixed << std::setprecision(2) << result.lookups_per_insert
                  << std::setw(15) << std::fixed << std::setprecision(2) << result.writes_per_erase
                  << std::setw(18) << result.writes_per_ends_erase << std::endl;
    }
}

int main() {
    test_map_complexity<geert::square_map>("square_map",
                                           {"O(√n)", "O(log n)", "O(log n)", "O(√n)", "O(√n)"});
    test_map_complexity<std::map>("std::map", {"O(log n)", "O(log n)", "O(log n)"});
    test_map_complexity<boost::container::flat_map>("flat_map",
                                                    {"O(n)", "O(log n)", "O(log n)", "O(n)", "O(n)"});

    bool failure = totalFailures > 0 || totalPasses == 0;
    std::string mark = failure ? "❌" : "✅";
    std::cout << "\n## Overall Summary\n" << std::endl;
    std::cout << "\n " << mark << " Total Passes: " << totalPasses
              << ", Total Failures: " << totalFailures << std::endl;

    return failure;
}
