#include "algorithms.h"
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
        geert::merge_with_binary_search(our_result.begin(), our_middle, our_result.end());

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
        geert::merge_with_binary_search(our_result.begin(), our_middle, our_result.end(), comp);

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

TEST_F(MergeWithBinarySearchTest, StabilityViolationReproducer) {
    // Minimal test case that exposes the stability violation
    // This test should FAIL with current implementation

    std::vector<std::pair<int, int>> data = {
        {4, 40},  // Left range: (4,40)
        {4, 0},   // Right range: (4,0)
    };

    auto middle = data.begin() + 1;  // Split after first element

    // Use a custom comparator that only compares the first element (key)
    // This makes (4,40) and (4,0) truly equal for comparison purposes
    auto key_compare = [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
        return a.first < b.first;
    };

    geert::merge_with_binary_search(data.begin(), middle, data.end(), key_compare);

    // Check if (4,40) from left still comes before (4,0) from right
    // This should FAIL because the current implementation violates stability
    EXPECT_EQ(data[0].second, 40)
        << "STABILITY VIOLATION: First element should be (4,40) from left range, but got ("
        << data[0].first << "," << data[0].second << ")";
    EXPECT_EQ(data[1].second, 0)
        << "STABILITY VIOLATION: Second element should be (4,0) from right range, but got ("
        << data[1].first << "," << data[1].second << ")";
}

TEST_F(MergeWithBinarySearchTest, StdInplaceMergeStabilityVerification) {
    // Verify that std::inplace_merge passes the same stability test
    // This should PASS, confirming our test is correct

    std::vector<std::pair<int, int>> data = {
        {4, 40},  // Left range: (4,40)
        {4, 0},   // Right range: (4,0)
    };

    auto middle = data.begin() + 1;  // Split after first element

    // Use a custom comparator that only compares the first element (key)
    // This makes (4,40) and (4,0) truly equal for comparison purposes
    auto key_compare = [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
        return a.first < b.first;
    };

    std::inplace_merge(data.begin(), middle, data.end(), key_compare);

    // Check if (4,40) from left still comes before (4,0) from right
    // This should PASS because std::inplace_merge is guaranteed to be stable
    EXPECT_EQ(data[0].second, 40) << "std::inplace_merge stability test failed: First element "
                                     "should be (4,40) from left range, but got ("
                                  << data[0].first << "," << data[0].second << ")";
    EXPECT_EQ(data[1].second, 0) << "std::inplace_merge stability test failed: Second element "
                                    "should be (4,0) from right range, but got ("
                                 << data[1].first << "," << data[1].second << ")";
}

TEST_F(MergeWithBinarySearchTest, StableMerge) {
    // Test that merge is stable: elements that compare equal maintain their relative order

    // Create a struct where we can distinguish between equal elements
    struct TrackedValue {
        int value;
        int original_position;

        bool operator<(const TrackedValue& other) const { return value < other.value; }

        bool operator==(const TrackedValue& other) const {
            return value == other.value && original_position == other.original_position;
        }
    };

    // Create first range: values [1, 2, 2, 3, 3, 3] with position tracking
    std::vector<TrackedValue> range1 = {
        {1, 1},  // position 1
        {2, 2},  // position 2
        {2, 3},  // position 3
        {3, 4},  // position 4
        {3, 5},  // position 5
        {3, 6}   // position 6
    };

    // Create second range: values [2, 2, 3, 4] with position tracking
    std::vector<TrackedValue> range2 = {
        {2, 7},  // position 7
        {2, 8},  // position 8
        {3, 9},  // position 9
        {4, 10}  // position 10
    };

    // Both ranges are already sorted

    // Test with std::inplace_merge (which is guaranteed to be stable)
    std::vector<TrackedValue> std_result = range1;
    std_result.insert(std_result.end(), range2.begin(), range2.end());
    auto std_middle = std_result.begin() + range1.size();
    std::inplace_merge(std_result.begin(), std_middle, std_result.end());

    // Test with our implementation
    std::vector<TrackedValue> our_result = range1;
    our_result.insert(our_result.end(), range2.begin(), range2.end());
    auto our_middle = our_result.begin() + range1.size();
    geert::merge_with_binary_search(our_result.begin(), our_middle, our_result.end());

    // Compare results - they should be identical for a stable merge
    ASSERT_EQ(std_result.size(), our_result.size()) << "Result sizes should match";

    for (size_t i = 0; i < std_result.size(); ++i) {
        EXPECT_EQ(std_result[i].value, our_result[i].value)
            << "Values should match at position " << i;
        EXPECT_EQ(std_result[i].original_position, our_result[i].original_position)
            << "Original positions should match at position " << i
            << " (stability requirement violated)";
    }

    // Additional verification: check specific stability requirements
    // For value=2: elements should appear in order [2, 3, 7, 8] (positions from left range first)
    // For value=3: elements should appear in order [4, 5, 6, 9] (positions from left range first)

    std::vector<int> value_2_positions, value_3_positions;
    for (const auto& item : our_result) {
        if (item.value == 2) {
            value_2_positions.push_back(item.original_position);
        } else if (item.value == 3) {
            value_3_positions.push_back(item.original_position);
        }
    }

    // For value=2: positions should be [2, 3] (from left range) followed by [7, 8] (from right
    // range)
    ASSERT_EQ(value_2_positions.size(), 4) << "Should have 4 elements with value=2";
    EXPECT_EQ(value_2_positions[0], 2) << "First value=2 should have position 2";
    EXPECT_EQ(value_2_positions[1], 3) << "Second value=2 should have position 3";
    EXPECT_EQ(value_2_positions[2], 7) << "Third value=2 should have position 7";
    EXPECT_EQ(value_2_positions[3], 8) << "Fourth value=2 should have position 8";

    // For value=3: positions should be [4, 5, 6] (from left range) followed by [9] (from right
    // range)
    ASSERT_EQ(value_3_positions.size(), 4) << "Should have 4 elements with value=3";
    EXPECT_EQ(value_3_positions[0], 4) << "First value=3 should have position 4";
    EXPECT_EQ(value_3_positions[1], 5) << "Second value=3 should have position 5";
    EXPECT_EQ(value_3_positions[2], 6) << "Third value=3 should have position 6";
    EXPECT_EQ(value_3_positions[3], 9) << "Fourth value=3 should have position 9";
}

class RemoveDuplicatesTest : public ::testing::Test {
protected:
    template <typename T>
    void test_remove_duplicates_behavior(std::vector<T> input, std::vector<T> expected) {
        // Sort the input first
        std::sort(input.begin(), input.end());

        // Test with our implementation
        std::vector<T> result = input;
        auto new_end = geert::remove_duplicates(result.begin(), result.end());
        result.erase(new_end, result.end());

        EXPECT_EQ(result, expected) << "Remove duplicates should keep only truly unique elements";
    }

    template <typename T, typename Compare>
    void test_remove_duplicates_behavior_with_comp(std::vector<T> input, std::vector<T> expected,
                                                   Compare comp) {
        // Sort the input first with the comparator
        std::sort(input.begin(), input.end(), comp);

        // Test with our implementation
        std::vector<T> result = input;
        auto new_end = geert::remove_duplicates(result.begin(), result.end(), comp);
        result.erase(new_end, result.end());

        EXPECT_EQ(result, expected) << "Remove duplicates should keep only truly unique elements";
    }
};

TEST_F(RemoveDuplicatesTest, EmptyRange) {
    std::vector<int> empty_vec;
    auto new_end = geert::remove_duplicates(empty_vec.begin(), empty_vec.end());
    EXPECT_EQ(new_end, empty_vec.end());
}

TEST_F(RemoveDuplicatesTest, SingleElement) {
    std::vector<int> single = {42};
    auto new_end = geert::remove_duplicates(single.begin(), single.end());
    EXPECT_EQ(new_end, single.end());
    EXPECT_EQ(single[0], 42);
}

TEST_F(RemoveDuplicatesTest, NoDuplicates) {
    test_remove_duplicates_behavior<int>({1, 2, 3, 4, 5}, {1, 2, 3, 4, 5});
}

TEST_F(RemoveDuplicatesTest, AllDuplicates) {
    std::vector<int> data = {3, 3, 3, 3, 3};
    auto new_end = geert::remove_duplicates(data.begin(), data.end());
    EXPECT_EQ(std::distance(data.begin(), new_end), 0);
}

TEST_F(RemoveDuplicatesTest, ConsecutivePairs) {
    // {1, 1, 2, 2, 3, 3, 4, 4} -> {} (all elements have duplicates)
    test_remove_duplicates_behavior<int>({1, 1, 2, 2, 3, 3, 4, 4}, {});
}

TEST_F(RemoveDuplicatesTest, ConsecutiveTriples) {
    // {1, 1, 1, 2, 2, 2, 3, 3, 3} -> {} (all elements have duplicates)
    test_remove_duplicates_behavior<int>({1, 1, 1, 2, 2, 2, 3, 3, 3}, {});
}

TEST_F(RemoveDuplicatesTest, MixedDuplicates) {
    // {1, 2, 2, 3, 4, 4, 4, 5, 6, 6} -> {1, 3, 5} (only elements without duplicates)
    test_remove_duplicates_behavior<int>({1, 2, 2, 3, 4, 4, 4, 5, 6, 6}, {1, 3, 5});
}

TEST_F(RemoveDuplicatesTest, StringData) {
    // {"apple", "apple", "banana", "cherry", "cherry"} -> {"banana"} (only unique element)
    test_remove_duplicates_behavior<std::string>({"apple", "apple", "banana", "cherry", "cherry"},
                                                 {"banana"});
}

TEST_F(RemoveDuplicatesTest, CustomComparator) {
    // Test with reverse order comparator
    auto reverse_comp = [](int a, int b) { return a > b; };
    // {5, 5, 4, 3, 3, 2, 1, 1} -> {4, 2} (only elements without duplicates)
    test_remove_duplicates_behavior_with_comp<int>({5, 5, 4, 3, 3, 2, 1, 1}, {4, 2}, reverse_comp);
}

TEST_F(RemoveDuplicatesTest, DefaultComparator) {
    // Test that default comparator works (std::less)
    std::vector<int> data = {1, 1, 2, 3, 3, 4, 5, 5, 5};
    auto new_end = geert::remove_duplicates(data.begin(), data.end());
    std::vector<int> result(data.begin(), new_end);
    std::vector<int> expected = {2, 4};  // Only unique elements remain
    EXPECT_EQ(result, expected);
}

TEST_F(RemoveDuplicatesTest, LargeData) {
    std::vector<int> large_data;
    std::vector<int> expected_unique;

    // Create pattern: 0,0,1,2,2,3,4,4,5,6,6,7...
    // Only elements that appear exactly once will be in expected result
    for (int i = 0; i < 1000; ++i) {
        large_data.push_back(i);
        if (i % 3 == 0 || i % 3 == 2) {  // Add duplicates for most numbers
            large_data.push_back(i);
        } else {  // i % 3 == 1, this element is unique
            expected_unique.push_back(i);
        }
    }
    std::sort(large_data.begin(), large_data.end());
    test_remove_duplicates_behavior(large_data, expected_unique);
}

TEST_F(RemoveDuplicatesTest, RandomData) {
    std::random_device rd;
    std::mt19937 gen(42);  // Fixed seed for reproducible tests
    std::uniform_int_distribution<> dis(1, 50);

    std::vector<int> random_data;
    for (int i = 0; i < 200; ++i) {
        random_data.push_back(dis(gen));
    }

    // Sort and manually compute expected result (elements that appear exactly once)
    std::sort(random_data.begin(), random_data.end());
    std::vector<int> expected;
    for (auto it = random_data.begin(); it != random_data.end();) {
        auto value = *it;
        auto next = std::upper_bound(it, random_data.end(), value);
        if (std::distance(it, next) == 1) {  // Appears exactly once
            expected.push_back(value);
        }
        it = next;
    }

    test_remove_duplicates_behavior(random_data, expected);
}
