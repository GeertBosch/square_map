#include "merge_with_binary_search.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <vector>
#include <string>
#include <random>
#include <functional>

class MergeWithBinarySearchTest : public ::testing::Test {
protected:
    template<typename T>
    void test_merge_equivalence(std::vector<T> data1, std::vector<T> data2) {
        // Sort both ranges
        std::sort(data1.begin(), data1.end());
        std::sort(data2.begin(), data2.end());
        
        // Create combined vector for std::inplace_merge
        std::vector<T> std_result = data1;
        std_result.insert(std_result.end(), data2.begin(), data2.end());
        auto std_middle = std_result.begin() + data1.size();
        std::inplace_merge(std_result.begin(), std_middle, std_result.end());
        
        // Create combined vector for our implementation
        std::vector<T> our_result = data1;
        our_result.insert(our_result.end(), data2.begin(), data2.end());
        auto our_middle = our_result.begin() + data1.size();
        merge_with_binary_search(our_result.begin(), our_middle, our_result.end());
        
        EXPECT_EQ(std_result, our_result) << "Merge results should be identical";
    }
    
    template<typename T, typename Compare>
    void test_merge_equivalence_with_comp(std::vector<T> data1, std::vector<T> data2, Compare comp) {
        // Sort both ranges
        std::sort(data1.begin(), data1.end(), comp);
        std::sort(data2.begin(), data2.end(), comp);
        
        // Create combined vector for std::inplace_merge
        std::vector<T> std_result = data1;
        std_result.insert(std_result.end(), data2.begin(), data2.end());
        auto std_middle = std_result.begin() + data1.size();
        std::inplace_merge(std_result.begin(), std_middle, std_result.end(), comp);
        
        // Create combined vector for our implementation
        std::vector<T> our_result = data1;
        our_result.insert(our_result.end(), data2.begin(), data2.end());
        auto our_middle = our_result.begin() + data1.size();
        merge_with_binary_search(our_result.begin(), our_middle, our_result.end(), comp);
        
        EXPECT_EQ(std_result, our_result) << "Merge results should be identical";
    }
};

TEST_F(MergeWithBinarySearchTest, EmptyRanges) {
    test_merge_equivalence<int>({}, {});
    test_merge_equivalence<int>({1, 2, 3}, {});
    test_merge_equivalence<int>({}, {4, 5, 6});
}

TEST_F(MergeWithBinarySearchTest, SingleElements) {
    test_merge_equivalence<int>({1}, {2});
    test_merge_equivalence<int>({2}, {1});
    test_merge_equivalence<int>({1}, {1});
}

TEST_F(MergeWithBinarySearchTest, SmallRanges) {
    test_merge_equivalence<int>({1, 3, 5}, {2, 4, 6});
    test_merge_equivalence<int>({1, 2, 3}, {4, 5, 6});
    test_merge_equivalence<int>({4, 5, 6}, {1, 2, 3});
}

TEST_F(MergeWithBinarySearchTest, LargeFirstRange) {
    // Test case where first range is much larger than second (F >> L)
    std::vector<int> large_range;
    for (int i = 0; i < 1000; i += 2) {
        large_range.push_back(i);
    }
    std::vector<int> small_range = {1, 3, 5, 7, 9};
    
    test_merge_equivalence(large_range, small_range);
}

TEST_F(MergeWithBinarySearchTest, DuplicateElements) {
    test_merge_equivalence<int>({1, 1, 2, 2}, {1, 2, 3, 3});
    test_merge_equivalence<int>({1, 1, 1}, {1, 1, 1});
}

TEST_F(MergeWithBinarySearchTest, StringMerge) {
    test_merge_equivalence<std::string>({"apple", "cherry", "grape"}, {"banana", "date", "fig"});
}

TEST_F(MergeWithBinarySearchTest, CustomComparator) {
    // Test with greater<int> comparator
    test_merge_equivalence_with_comp<int>({5, 3, 1}, {6, 4, 2}, std::greater<int>());
}

TEST_F(MergeWithBinarySearchTest, RandomData) {
    std::mt19937 gen(42); // Fixed seed for reproducibility
    std::uniform_int_distribution<int> dis(1, 1000);
    
    for (int test = 0; test < 10; ++test) {
        std::vector<int> range1, range2;
        
        // Generate random sizes, with first range typically larger
        int size1 = dis(gen) % 500 + 100;  // 100-599
        int size2 = dis(gen) % 50 + 1;     // 1-50
        
        for (int i = 0; i < size1; ++i) {
            range1.push_back(dis(gen));
        }
        for (int i = 0; i < size2; ++i) {
            range2.push_back(dis(gen));
        }
        
        test_merge_equivalence(range1, range2);
    }
}

TEST_F(MergeWithBinarySearchTest, AllElementsInSecondRange) {
    // Test where all elements in second range are larger than first
    test_merge_equivalence<int>({1, 2, 3}, {10, 11, 12});
}

TEST_F(MergeWithBinarySearchTest, AllElementsInFirstRange) {
    // Test where all elements in first range are larger than second
    test_merge_equivalence<int>({10, 11, 12}, {1, 2, 3});
}

TEST_F(MergeWithBinarySearchTest, InterleavedElements) {
    // Test where elements are perfectly interleaved
    test_merge_equivalence<int>({1, 3, 5, 7, 9}, {2, 4, 6, 8, 10});
}
