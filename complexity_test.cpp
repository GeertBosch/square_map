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
    static std::atomic<size_t> total_writes() { return copy_count + move_count; }

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

// Define static members
std::atomic<size_t> InstrumentedInt::copy_count{0};
std::atomic<size_t> InstrumentedInt::move_count{0};
std::atomic<size_t> InstrumentedInt::comparison_count{0};

// Structure to hold test results
struct TestResult {
    int n;
    double writes_per_element;
    double comparisons_per_element;
    double lookups_per_element;
    double failed_lookups_per_element;
};

// Function to calculate correlation coefficient for complexity analysis
double calculate_correlation(const std::vector<double>& x, const std::vector<double>& y) {
    if (x.size() != y.size() || x.empty()) return 0.0;
    
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0, sum_y2 = 0;
    int n = x.size();
    
    for (int i = 0; i < n; i++) {
        sum_x += x[i];
        sum_y += y[i];
        sum_xy += x[i] * y[i];
        sum_x2 += x[i] * x[i];
        sum_y2 += y[i] * y[i];
    }
    
    double numerator = n * sum_xy - sum_x * sum_y;
    double denominator = std::sqrt((n * sum_x2 - sum_x * sum_x) * (n * sum_y2 - sum_y * sum_y));
    
    return denominator == 0 ? 0 : numerator / denominator;
}

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
void analyze_complexity(const std::vector<TestResult>& results, const std::string& operation) {
    std::cout << "\n=== Complexity Analysis for " << operation << " ===" << std::endl;
    
    std::vector<double> n_values, operation_values;
    for (const auto& result : results) {
        n_values.push_back(result.n);
        if (operation == "Insert Writes") {
            operation_values.push_back(result.writes_per_element);
        } else if (operation == "Insert Comparisons") {
            operation_values.push_back(result.comparisons_per_element);
        } else if (operation == "Successful Lookups") {
            operation_values.push_back(result.lookups_per_element);
        } else if (operation == "Failed Lookups") {
            operation_values.push_back(result.failed_lookups_per_element);
        }
    }
    
    // Test different complexity functions
    std::vector<double> constant(n_values.size(), 1.0);
    std::vector<double> log_n, sqrt_n, linear_n, n_log_n;
    
    for (double n : n_values) {
        log_n.push_back(std::log2(n));
        sqrt_n.push_back(std::sqrt(n));
        linear_n.push_back(n);
        n_log_n.push_back(n * std::log2(n));
    }
    
    // Calculate correlations, R², and ratio consistency
    double corr_constant = std::abs(calculate_correlation(constant, operation_values));
    double corr_log_n = std::abs(calculate_correlation(log_n, operation_values));
    double corr_sqrt_n = std::abs(calculate_correlation(sqrt_n, operation_values));
    double corr_linear_n = std::abs(calculate_correlation(linear_n, operation_values));
    double corr_n_log_n = std::abs(calculate_correlation(n_log_n, operation_values));
        
    double ratio_const_log_n = calculate_ratio_consistency(log_n, operation_values);
    double ratio_const_sqrt_n = calculate_ratio_consistency(sqrt_n, operation_values);
    double ratio_const_linear_n = calculate_ratio_consistency(linear_n, operation_values);
    
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "                     Ratio Consistency" << std::endl;
    std::cout << "O(log n):            " << ratio_const_log_n << std::endl;
    std::cout << "O(√n):               " << ratio_const_sqrt_n << std::endl;
    std::cout << "O(n):                " << ratio_const_linear_n << std::endl;
    
    // Find best fit using ratio consistency
    std::vector<std::pair<double, std::string>> ratio_results = {
        {ratio_const_log_n, "O(log n)"},
        {ratio_const_sqrt_n, "O(√n)"},
        {ratio_const_linear_n, "O(n)"}
    };
    
    std::sort(ratio_results.rbegin(), ratio_results.rend());
    std::cout << "Best fit: " << ratio_results[0].second 
              << " (ratio consistency = " << ratio_results[0].first << ")" << std::endl;
    
    // Show the difference between best and second best
    if (ratio_results.size() > 1) {
        double difference = ratio_results[0].first - ratio_results[1].first;
        std::cout << "Margin over second best (" << ratio_results[1].second << "): " 
                  << difference << std::endl;
        
        if (difference < 0.2) {
            std::cout << "WARNING: Small margin suggests inconclusive results!" << std::endl;
        }
    }
    
    // Additional analysis for strong evidence
    std::cout << "\nConclusion:" << std::endl;
    if (ratio_const_log_n > 0.8) {
        std::cout << "Strong evidence for O(log n) complexity" << std::endl;
    }
    if (ratio_const_sqrt_n > 0.8) {
        std::cout << "Strong evidence for O(√n) complexity" << std::endl;
    }
    if (ratio_const_linear_n > 0.8) {
        std::cout << "Strong evidence for O(n) complexity" << std::endl;
    }
    if (ratio_const_log_n < 0.8 && ratio_const_sqrt_n < 0.8 && ratio_const_linear_n < 0.8) {
        std::cout << "No clear complexity pattern detected" << std::endl;
    }
    
    // Print detailed data for debugging
    std::cout << "Data points:" << std::endl;
    std::cout << "N\t" << operation << "\tlog(n)\t√n\tn" << std::endl;
    for (size_t i = 0; i < n_values.size(); i++) {
        std::cout << (int)n_values[i] << "\t" 
                  << std::fixed << std::setprecision(2) << operation_values[i] << "\t\t"
                  << std::fixed << std::setprecision(2) << log_n[i] << "\t"
                  << std::fixed << std::setprecision(0) << sqrt_n[i] << "\t"
                  << std::fixed << std::setprecision(0) << linear_n[i] << std::endl;
    }
}

// Test function with multiple N values
void test_square_map_complexity_multi() {
    std::vector<int> test_sizes = {4000, 8000, 16000, 32000, 64000, 128000};
    std::vector<TestResult> results;
    
    std::cout << "=== Square Map Multi-Size Complexity Test ===" << std::endl;
    std::cout << "Testing with sizes: ";
    for (int n : test_sizes) {
        std::cout << n << " ";
    }
    std::cout << std::endl << std::endl;
    
    for (int N : test_sizes) {
        std::cout << "Testing N = " << N << "..." << std::endl;
        
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
            std::random_device rd;
            std::mt19937 gen(rd());
            std::shuffle(insert_order.begin(), insert_order.end(), gen);
            
            // Insert N elements in shuffled order
            for (int i : insert_order) {
                smap.insert({InstrumentedInt(i), InstrumentedInt(i * 2)});
            }
            
            result.writes_per_element = (double)InstrumentedInt::total_writes() / N;
            result.comparisons_per_element = (double)InstrumentedInt::comparison_count / N;
            
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
            
            result.lookups_per_element = (double)InstrumentedInt::comparison_count / N;
        }
        
        // Test failed lookups
        {
            InstrumentedInt::reset_counters();
            
            using square_map_type = geert::square_map<InstrumentedInt, InstrumentedInt>;
            square_map_type smap;
            
            // Populate with even numbers
            for (int i = 0; i < N; i += 2) {
                smap.insert({InstrumentedInt(i), InstrumentedInt(i * 2)});
            }
            
            InstrumentedInt::reset_counters();
            
            // Create shuffled order for failed lookups (odd numbers)
            std::vector<int> failed_lookup_order;
            for (int i = 1; i < N; i += 2) {
                failed_lookup_order.push_back(i);
            }
            std::random_device rd;
            std::mt19937 gen(rd());
            std::shuffle(failed_lookup_order.begin(), failed_lookup_order.end(), gen);
            
            // Search for odd numbers in shuffled order (should fail)
            int not_found_count = 0;
            for (int i : failed_lookup_order) {
                auto it = smap.find(InstrumentedInt(i));
                if (it == smap.end()) {
                    not_found_count++;
                }
            }
            
            result.failed_lookups_per_element = (double)InstrumentedInt::comparison_count / (N/2.0);
        }
        
        results.push_back(result);
        
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "  Writes/elem: " << result.writes_per_element 
                  << ", Comparisons/elem: " << result.comparisons_per_element
                  << ", Lookups/elem: " << result.lookups_per_element
                  << ", Failed lookups/elem: " << result.failed_lookups_per_element << std::endl;
    }
    
    // Analyze complexities
    analyze_complexity(results, "Insert Writes");
    analyze_complexity(results, "Insert Comparisons");
    analyze_complexity(results, "Successful Lookups");
    analyze_complexity(results, "Failed Lookups");
    
    // Print detailed results table
    std::cout << "\n=== Detailed Results ===" << std::endl;
    std::cout << std::setw(8) << "N" 
              << std::setw(12) << "Writes/elem"
              << std::setw(12) << "Comp/elem" 
              << std::setw(12) << "Lookup/elem"
              << std::setw(15) << "FailLookup/elem" << std::endl;
    std::cout << std::string(59, '-') << std::endl;
    
    for (const auto& result : results) {
        std::cout << std::setw(8) << result.n
                  << std::setw(12) << std::fixed << std::setprecision(2) << result.writes_per_element
                  << std::setw(12) << std::fixed << std::setprecision(2) << result.comparisons_per_element
                  << std::setw(12) << std::fixed << std::setprecision(2) << result.lookups_per_element
                  << std::setw(15) << std::fixed << std::setprecision(2) << result.failed_lookups_per_element << std::endl;
    }
}

// Original single-size test function (kept for compatibility)
void test_square_map_complexity() {
    constexpr int N = 100'000;
    
    std::cout << "=== Square Map Complexity Test ===" << std::endl;
    std::cout << "Testing with " << N << " insertions" << std::endl << std::endl;
    
    // Test with square_map
    {
        InstrumentedInt::reset_counters();
        
        using square_map_type = geert::square_map<InstrumentedInt, InstrumentedInt>;
        square_map_type smap;
        
        // Create shuffled order for insertion
        std::vector<int> insert_order;
        for (int i = 0; i < N; ++i) {
            insert_order.push_back(i);
        }
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(insert_order.begin(), insert_order.end(), gen);
        
        // Insert N elements in shuffled order
        for (int i : insert_order) {
            smap.insert({InstrumentedInt(i), InstrumentedInt(i * 2)});
        }
        
        InstrumentedInt::print_stats("Square Map Shuffled Insert");
        
        size_t total_writes = InstrumentedInt::total_writes();
        size_t comparisons = InstrumentedInt::comparison_count;
        
        std::cout << "Analysis:" << std::endl;
        std::cout << "  Writes per element: " << (double)total_writes / N << std::endl;
        std::cout << "  Comparisons per element: " << (double)comparisons / N << std::endl;
        
        // Square map insert complexity analysis
        double sqrt_n = std::sqrt(N);
        std::cout << "  Expected O(√n): " << sqrt_n << " (√" << N << ")" << std::endl;
        std::cout << "  Writes vs O(√n): " << (double)total_writes / N / sqrt_n << "x" << std::endl;
        std::cout << "  Comparisons vs O(√n): " << (double)comparisons / N / sqrt_n << "x" << std::endl;
        std::cout << std::endl;
    }
    
    // Test lookup complexity
    {
        InstrumentedInt::reset_counters();
        
        using square_map_type = geert::square_map<InstrumentedInt, InstrumentedInt>;
        square_map_type smap;
        
        // First, populate the map (reset counters after)
        for (int i = 0; i < N; ++i) {
            smap.insert({InstrumentedInt(i), InstrumentedInt(i * 2)});
        }
        
        InstrumentedInt::reset_counters();
        
        // Create shuffled order for lookups
        std::vector<int> lookup_order;
        for (int i = 0; i < N; ++i) {
            lookup_order.push_back(i);
        }
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(lookup_order.begin(), lookup_order.end(), gen);
        
        // Now test lookups in shuffled order
        int found_count = 0;
        for (int i : lookup_order) {
            auto it = smap.find(InstrumentedInt(i));
            if (it != smap.end()) {
                found_count++;
            }
        }
        
        InstrumentedInt::print_stats("Square Map Successful Lookup");
        
        size_t total_writes = InstrumentedInt::total_writes();
        size_t comparisons = InstrumentedInt::comparison_count;
        
        std::cout << "Found " << found_count << " out of " << N << " elements" << std::endl;
        std::cout << "Analysis:" << std::endl;
        std::cout << "  Writes per lookup: " << (double)total_writes / N << std::endl;
        std::cout << "  Comparisons per lookup: " << (double)comparisons / N << std::endl;
        
        double log_n = std::log2(N);
        std::cout << "  Expected O(log n): " << log_n << " (log₂(" << N << "))" << std::endl;
        std::cout << "  Actual vs O(log n): " << (double)comparisons / N / log_n << "x" << std::endl;
        std::cout << std::endl;
    }
    
    // Test failed lookup complexity
    {
        InstrumentedInt::reset_counters();
        
        using square_map_type = geert::square_map<InstrumentedInt, InstrumentedInt>;
        square_map_type smap;
        
        // Populate with even numbers
        for (int i = 0; i < N; i += 2) {
            smap.insert({InstrumentedInt(i), InstrumentedInt(i * 2)});
        }
        
        InstrumentedInt::reset_counters();
        
        // Create shuffled order for failed lookups (odd numbers)
        std::vector<int> failed_lookup_order;
        for (int i = 1; i < N; i += 2) {
            failed_lookup_order.push_back(i);
        }
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(failed_lookup_order.begin(), failed_lookup_order.end(), gen);
        
        // Search for odd numbers in shuffled order (should fail)
        int not_found_count = 0;
        for (int i : failed_lookup_order) {
            auto it = smap.find(InstrumentedInt(i));
            if (it == smap.end()) {
                not_found_count++;
            }
        }
        
        InstrumentedInt::print_stats("Square Map Failed Lookup");
        
        size_t total_writes = InstrumentedInt::total_writes();
        size_t comparisons = InstrumentedInt::comparison_count;
        int searches = N / 2;
        
        std::cout << "Failed to find " << not_found_count << " out of " << searches << " searches" << std::endl;
        std::cout << "Analysis:" << std::endl;
        std::cout << "  Writes per failed lookup: " << (double)total_writes / searches << std::endl;
        std::cout << "  Comparisons per failed lookup: " << (double)comparisons / searches << std::endl;
        
        double log_n = std::log2(N/2);  // Size of actual map
        std::cout << "  Expected O(log n): " << log_n << " (log₂(" << N/2 << "))" << std::endl;
        std::cout << "  Actual vs O(log n): " << (double)comparisons / searches / log_n << "x" << std::endl;
        std::cout << std::endl;
    }
}

int main() {
    test_square_map_complexity_multi();
    return 0;
}
