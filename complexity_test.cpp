#include <iostream>
#include <atomic>
#include <vector>
#include <algorithm>
#include <random>
#include <cmath>
#include <iomanip>
#include "square_map.h"

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
        std::cout << operation << " - Copies: " << copy_count 
                  << ", Moves: " << move_count 
                  << ", Total Writes: " << total_writes()
                  << ", Comparisons: " << comparison_count << std::endl;
    }
};

constexpr bool kDebug = false;
constexpr double kConfidenceThreshold = 0.9;  // Threshold for strong confidence in complexity
// Don't test small sizes, as there complexity is different, but with small constants

const std::vector<int> test_sizes = {8'000, 16'000, 32'000, 64'000, 128'000};

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
                        std::string expected_complexity) {
    std::vector<double> n_values, operation_values;
    for (const auto& result : results) {
        n_values.push_back(result.n);
        if (operation == "Insert Writes") {
            operation_values.push_back(result.writes_per_insert);
        } else if (operation == "Insert Comparisons") {
            operation_values.push_back(result.comparisons_per_insert);
        } else if (operation == "Lookup Comparisons") {
            operation_values.push_back(result.lookups_per_insert);
        }
    }
    
    // Test different complexity functions
    std::vector<double> constant(n_values.size(), 1.0);
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
        std::cout << "\n=== " << operation << " Ratio Consistency Scores ===\n" << std::endl;
        std::cout << "  O(log n): " << std::fixed << std::setprecision(3) << ratio_const_log_n
                  << std::endl;
        std::cout << "     O(√n): " << std::fixed << std::setprecision(3) << ratio_const_sqrt_n
                  << std::endl;
        std::cout << "      O(n): " << std::fixed << std::setprecision(3) << ratio_const_linear_n
                  << std::endl
                  << std::endl;
    }

    // Find best fit using ratio consistency
    std::vector<std::pair<double, std::string>> ratio_results = {
        {ratio_const_log_n, "O(log n)"},
        {ratio_const_sqrt_n, "O(√n)"},
        {ratio_const_linear_n, "O(n)"}
    };
    
    std::sort(ratio_results.rbegin(), ratio_results.rend());
    if (kDebug)
        std::cout << "Best fit: " << ratio_results[0].second
                  << " (ratio consistency = " << std::fixed << std::setprecision(3)
                  << ratio_results[0].first << ")" << std::endl;

    double difference = 0.0;
    bool failure = false;
    std::string complexity = ratio_results[0].second;

    // Check the difference between best and second best
    if (ratio_results.size() > 1) {
        difference = ratio_results[0].first - ratio_results[1].first;
        if (kDebug)
            std::cout << "Margin over second best (" << ratio_results[1].second
                      << "): " << std::fixed << std::setprecision(3) << difference << std::endl;

        if (difference < 0.2) {
            failure = true;
            std::cout << "⚠️  WARNING: Small margin of " << std::fixed << std::setprecision(2)
                      << difference << " between " << ratio_results[0].second << " and "
                      << ratio_results[1].second << " suggests inconclusive results!" << std::endl;
        }
    }

    // Additional analysis for strong evidence
    failure = failure || (ratio_results[0].first < kConfidenceThreshold);
    if (failure) {
        std::cout << "❌ No clear complexity pattern detected for " << operation << std::endl;
        ++totalFailures;
    } else {
        // Add a checkmark to the next message
        std::cout << "✅ High confidence for " << complexity << " " << operation
                  << " complexity, ratio consistency " << std::fixed << std::setprecision(2)
                  << ratio_results[0].first << std::endl;
        ++totalPasses;
    }

    if (kDebug) {
        // Print detailed data for debugging
        std::cout << "\n=== Data points ===\n" << std::endl;

        std::cout << std::setw(8) << "N" << std::setw(20) << operation << std::setw(10) << "log(n)"
                  << std::setw(10) << "√n" << std::setw(10) << "n" << std::endl;
        std::cout << std::string(56, '-') << std::endl;
        for (size_t i = 0; i < n_values.size(); i++) {
            std::cout << std::setw(8) << (int)n_values[i] << std::setw(20) << std::fixed
                      << std::setprecision(2) << operation_values[i] << std::setw(10) << std::fixed
                      << std::setprecision(2) << log_n[i] << std::setw(8) << std::fixed
                      << std::setprecision(0) << sqrt_n[i] << std::setw(10) << std::fixed
                      << std::setprecision(0) << linear_n[i] << std::endl;
        }
    }
}

// Test function with multiple N values
void test_square_map_complexity_multi() {
    std::vector<TestResult> results;

    std::cout << "\n=== Square Map Complexity Test ===\n" << std::endl;

    for (int N : test_sizes) {
        std::cout << "⏳ Testing N = " << N << "...";
        std::flush(std::cout);

        TestResult result;
        result.n = N;
        
        // Test insertions
        {
            InstrumentedInt::reset_counters();
            
            using square_map_type = geert::square_map<InstrumentedInt, InstrumentedInt>;
            square_map_type smap;
            
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
        std::cout << "\r✔️\n";

        results.push_back(result);

        if (kDebug) {
            std::cout << std::fixed << std::setprecision(2);
            std::cout << "  Writes/Insert: " << result.writes_per_insert
                      << ", Comparisons/Insert: " << result.comparisons_per_insert
                      << ", Lookups/Insert: " << result.lookups_per_insert << std::endl;
        }
    }
    
    // Analyze complexities
    std::cout << "\n=== Complexity Analysis ===\n" << std::endl;
    analyze_complexity(results, "Insert Writes", "O(√n)");
    analyze_complexity(results, "Insert Comparisons", "O(log n)");
    analyze_complexity(results, "Lookup Comparisons", "O(log n)");

    // Print detailed results table
    std::cout << "\n=== Detailed Results ===\n" << std::endl;
    std::cout << std::setw(8) << "N" << std::setw(15) << "Writes/Insert" << std::setw(20)
              << "Comparisons/Insert" << std::setw(20) << "Comparisons/Lookup" << std::endl;
    std::cout << std::string(63, '-') << std::endl;

    for (const auto& result : results) {
        std::cout << std::setw(8) << result.n << std::setw(15) << std::fixed << std::setprecision(2)
                  << result.writes_per_insert << std::setw(20) << std::fixed << std::setprecision(2)
                  << result.comparisons_per_insert << std::setw(20) << std::fixed
                  << std::setprecision(2) << result.lookups_per_insert << std::endl;
    }
}

int main() {
    test_square_map_complexity_multi();
    bool failure = totalFailures > 0 || totalPasses == 0;
    std::string mark = failure ? "❌" : "✅";
    std::cout << "\n"
              << mark << " Total Passes: " << totalPasses << ", Total Failures: " << totalFailures
              << std::endl;

    return failure;
}
