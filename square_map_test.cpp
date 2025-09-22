#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <algorithm>
#include <cstdint>
#include <random>
#include <numeric>

#include "square_map.h"

/**
 * Returns true if and only if the container is sorted and has no duplicates.
 */
template <typename It>
bool is_strictly_sorted(It begin, It end) {
    return std::is_sorted(begin, end) && std::adjacent_find(begin, end) == end;
}

/**
 * check_valid verifies that the passed square map has a valid internal structure. It does this
 * through extracting the adapted container and checking its contents:
 *   - The map is empty if and only if the container is empty.
 *   - If the container is not empty, split_point() must be valid and unequal to end()
 *   - If split_point() is begin(), the map is merged into one strictly sorted range and equal
 *     to a flat map
 *   - If split_point() is not begin():
 *       1. the container has two strictly sorted ranges.
 *       2. The first key of the right range must be less than the last key of the left range,
 *          ensuring that the split is necessary.
 *       3. The last key of the right range is larger than the last key of the left range, ensuring
 *          that the last element in the map is also the last element in the container. This
 *          property also implies that the last element cannot be a deleted element.
 *       4. The size of the map is equal to the size of the container minus the number of duplicate
 *          keys (indicating erased elements) that are present in both ranges. Due to the above
 *          requirements it is not possible to make any guarantees about the relative sizes of the
 *          two ranges, or even whether the left range is larger than kMinSplitSize: repeatedly
 *          erasing the largest element in the left range can make that range arbitrarily small, as
 *          inserting a duplicate in the right range for erasure would violate rule 2.
 */
template <typename Map>
void check_valid(Map m) {
    Map m_copy = m;  // copy to avoid modifying the original map
    auto container = std::move(m_copy).extract();

    // Check some universal properties. Note that sizes differ iff the map contains erased items.
    EXPECT_EQ(container.empty(), m.empty());

    // Check properties of an empty map
    if (m.empty()) {
        EXPECT_EQ(m.size(), 0);
        EXPECT_EQ(m.begin(), m.end());
        EXPECT_EQ(m.split_point(), m.end());
        return;
    }

    // Properties of a non-empty map
    EXPECT_GT(m.size(), 0);
    EXPECT_NE(m.begin(), m.end());

    // Check properties of a map with a single range
    if (m.split_point() == m.end()) {
        // The map is merged into one range, so it should be a flat map.
        EXPECT_TRUE(is_strictly_sorted(container.begin(), container.end()));
        EXPECT_EQ(m.size(), container.size());
        return;
    }

    // The map is split into two ranges. Elements that exist in both the left and the right
    // range are considered erased, so the split point in the container is not necessarily
    // container.begin + std::distance(m.begin, m.split_point). Find the container split point.
    auto split_it = container.begin();
    EXPECT_NE(m.split_point(), m.end());
    auto split_key = m.split_point()->first;
    while (split_it != container.end() && split_it->first != split_key) ++split_it;

    // 1. Check that the two ranges are strictly sorted
    EXPECT_TRUE(is_strictly_sorted(container.begin(), split_it));
    EXPECT_TRUE(is_strictly_sorted(split_it, container.end()));

    // 2. The first key of the right range must be less than the last key of the left range.
    EXPECT_LT(split_key, (std::prev(split_it))->first);

    // 3. The last key of the right range is larger than the last key of the left range.
    EXPECT_GT((std::prev(container.end()))->first, (std::prev(split_it))->first);

    // Compute the number of keys that are duplicated between the left and right ranges by
    // iterating over the elements of the right (typically smaller) range and looking each
    // key up in the left range. These duplicate keys indicate erased elements.
    size_t num_erased = std::count_if(split_it, container.end(), [&](const auto& element) {
        // Use a custom projection that extracts the key from pairs for comparison
        auto key_compare = [](const typename Map::value_type& a,
                              const typename Map::value_type& b) { return a.first < b.first; };
        typename Map::value_type search_pair{element.first, {}};
        return std::binary_search(container.begin(), split_it, search_pair, key_compare);
    });

    // 4. Check that the size difference is equal to the number of erased elements: two elements
    //    are present for each erased key, one in each range.
    EXPECT_EQ(m.size() + 2 * num_erased, container.size());
}

/**
 * Helper functions for inject methods that take split_index parameters.
 * These functions were moved from square_map.h to the test file for eventual deprecation.
 * Updated to use the new replace method instead of the removed inject methods.
 */
template <typename Map>
Map inject_with_split_index(typename Map::container_type&& container,
                            typename Map::size_type split_index = 0) {
    Map result;
    auto split_it = split_index == 0
                        ? container.end()
                        : std::next(container.begin(), std::min(split_index, container.size()));
    result.replace(std::move(container), split_it);
    return result;
}

template <typename Map>
Map inject_with_split_index(const typename Map::container_type& container,
                            typename Map::size_type split_index = 0) {
    Map result;
    auto container_copy = container;  // Copy the const container
    auto split_it = split_index == 0 ? container_copy.end()
                                     : std::next(container_copy.begin(),
                                                 std::min(split_index, container_copy.size()));
    result.replace(std::move(container_copy), split_it);
    return result;
}

TEST(OrderedMap, Empty) {
    using Map = geert::square_map<uint32_t, bool, std::less<uint32_t>, std::vector<std::pair<uint32_t, bool>>>;
    Map empty;
    Map::key_type key = {};
    EXPECT_EQ(empty.size(), 0);
    EXPECT_TRUE(empty.empty());
    EXPECT_EQ(empty.begin(), empty.end());
    EXPECT_EQ(empty.count(key), 0);
    EXPECT_EQ(empty.find(key), empty.end());
    EXPECT_THROW(empty.at(key), std::out_of_range);
}

TEST(OrderedMap, SingleValue) {
    using Map = geert::square_map<uint32_t, bool, std::less<uint32_t>, std::vector<std::pair<uint32_t, bool>>>;
    Map single;
    Map::key_type key = {};
    Map::mapped_type mapped = {};
    single.insert({key, mapped});
    check_valid(single);
    EXPECT_EQ(single.size(), 1);
    EXPECT_FALSE(single.empty());
    EXPECT_NE(single.begin(), single.end());
    EXPECT_EQ(std::next(single.begin()), single.end());
    EXPECT_EQ(single.count(key), 1);
    EXPECT_EQ(single.find(key)->second, mapped);
    EXPECT_EQ(single[key], mapped);
    EXPECT_EQ(single.size(), 1);
}

TEST(OrderedMap, TwoValues) {
    using Map = geert::square_map<uint32_t, bool, std::less<uint32_t>, std::vector<std::pair<uint32_t, bool>>>;
    Map two;
    Map::key_type key1 = 0;
    Map::key_type key2 = key1 + 1;
    Map::mapped_type mapped = {};
    two.insert({key2, mapped});
    two.insert({key1, mapped});
    check_valid(two);

    EXPECT_EQ(two.size(), 2);
    EXPECT_FALSE(two.empty());
    EXPECT_THROW(two.at(key2 + 1), std::out_of_range);
    EXPECT_NE(two.begin(), two.end());
    EXPECT_EQ(std::next(two.begin(), 2), two.end());
    EXPECT_TRUE(is_strictly_sorted(two.begin(), two.end()));
}

TEST(OrderedMap, EraseTwo) {
    using Map = geert::square_map<uint32_t, bool, std::less<uint32_t>, std::vector<std::pair<uint32_t, bool>>>;
    Map two;
    Map::key_type key1 = 0;
    Map::key_type key2 = key1 + 1;
    Map::mapped_type mapped = {};
    two.insert({key2, mapped});
    two.insert({key1, mapped});
    two.erase(two.find(key1));
    EXPECT_EQ(two.find(key2), two.begin());
    two.insert({key1, mapped});
    EXPECT_EQ(two.find(key1), two.begin());
    check_valid(two);
}

TEST(OrderedMap, SortTenValues) {
    using Map = geert::square_map<uint32_t, bool, std::less<uint32_t>, std::vector<std::pair<uint32_t, bool>>>;
    Map ten;
    Map::key_type keys[10] = {4, 3, 2, 7, 9, 1, 6, 8, 10, 5};
    Map::mapped_type mapped = {};
    for (auto key : keys) {
        auto ret = ten.insert({key, mapped});
        EXPECT_TRUE(ret.second);
        EXPECT_EQ(ret.first->first, key);
        check_valid(ten);
    }
    EXPECT_EQ(ten.size(), 10);
    EXPECT_EQ(std::next(ten.begin(), 10), ten.end());
    EXPECT_TRUE(is_strictly_sorted(ten.begin(), ten.end()));
    for (auto key : keys) {
        EXPECT_EQ(ten.count(key), 1);
        EXPECT_EQ(ten.find(key)->first, key);
    }
}

TEST(OrderedMap, TenSquares) {
    using Map = geert::square_map<uint32_t, bool, std::less<uint32_t>, std::vector<std::pair<uint32_t, bool>>>;
    Map tenSquares;
    Map::key_type keys[10] = {5, 3, 2, 10, 8, 6, 9, 4, 1, 7};
    for (auto key : keys)
        tenSquares.insert({key * key, {}});
    check_valid(tenSquares);
    EXPECT_EQ(tenSquares.count(16), 1);
    EXPECT_EQ(tenSquares.find(13), tenSquares.end());
}

TEST(OrderedMap, FindNext) {
    using Map = geert::square_map<uint32_t, bool, std::less<uint32_t>, std::vector<std::pair<uint32_t, bool>>>;
    Map::key_type keys[15] = {10, 5, 12, 4, 3, 2, 9, 14, 15, 8, 1, 13, 6, 11, 7};
    Map fifteenNumbers;
    for (auto key : keys)
        fifteenNumbers.insert({key, {}});
    check_valid(fifteenNumbers);
    for (auto key : keys) {
        auto it = fifteenNumbers.find(key);
        EXPECT_NE(it, fifteenNumbers.end());
        for (auto j = key; j <= 15; ++j, ++it) EXPECT_EQ(it->first, j);
        EXPECT_EQ(it, fifteenNumbers.end());
    }
}

TEST(OrderedMap, Iters) {
    using Map = geert::square_map<uint32_t, bool, std::less<uint32_t>, std::vector<std::pair<uint32_t, bool>>>;
    Map map;
    for (int j = 0; j < 9; ++j)
        map.insert({j, true});
    check_valid(map);
    Map::iterator it = map.begin();
    Map::const_iterator cit = map.cbegin();
    [](const Map& cmap, Map::iterator it) {
        EXPECT_EQ(cmap.begin(), it);
        it->second = false;
        EXPECT_FALSE(cmap.begin()->second);
        ++it;
        EXPECT_TRUE(it->second);
    }(map, it);
    EXPECT_EQ(it, cit);
    while (it != map.cend())
        ++it;
}

// Correctness tests with shuffled data
class ShuffledMap : public ::testing::Test {
protected:
    void SetUp() override {
        numbers.resize(kMapSize);
        std::iota(numbers.begin(), numbers.end(), 1);
        gen.seed(0);  // Fixed seed for reproducible tests
        std::shuffle(numbers.begin(), numbers.end(), gen);
    }

    static constexpr uint32_t kMapSize = 1000;
    static_assert(kMapSize > 3 * geert::square_map<uint32_t, bool>::kMinSplitSize);
    std::mt19937 gen;
    std::vector<uint32_t> numbers;
};

TEST_F(ShuffledMap, IterateAll) {
    using Map = geert::square_map<uint32_t, bool, std::less<uint32_t>, std::vector<std::pair<uint32_t, bool>>>;
    Map map;
    for (auto n : numbers)
        map[n];
    check_valid(map);
    auto it = map.begin();
    for (uint32_t j = 1; j <= kMapSize; ++j)
        EXPECT_EQ(it++->first, j);
}

TEST_F(ShuffledMap, IterateRange) {
    using Map = geert::square_map<uint32_t, bool, std::less<uint32_t>, std::vector<std::pair<uint32_t, bool>>>;
    uint32_t kRangeSize = 3;

    Map map;
    for (auto n : numbers)
        map[n];
    check_valid(map);
    std::uniform_int_distribution<> distrib(1, kRangeSize);

    for (uint32_t j = 1; j <= kMapSize - kRangeSize; j += distrib(gen)) {
        auto it = map.find(j);
        EXPECT_NE(it, map.end());
        EXPECT_EQ(it->first, j);
        for (uint32_t k = 0; k < kRangeSize; ++k)
            EXPECT_EQ(it++->first, j + k);
    }
}

TEST_F(ShuffledMap, Sieve1000) {
    using Map = geert::square_map<uint32_t, bool, std::less<uint32_t>, std::vector<std::pair<uint32_t, bool>>>;
    Map isPrime;
    for (auto x : numbers)
        isPrime[x] = true;  // Until proven otherwise.
    check_valid(isPrime);
    EXPECT_EQ(isPrime.size(), numbers.size());
    EXPECT_TRUE(is_strictly_sorted(isPrime.begin(), isPrime.end()));

    isPrime[1] = false;
    uint32_t sumPrimes = 0;
    for (auto it : isPrime) {
        if (auto divisor = it.first; it.second) {
            sumPrimes += divisor;
            if (divisor * divisor > numbers.size())
                continue;
            for (auto notAPrime = 2 * divisor; notAPrime <= numbers.size(); notAPrime += divisor)
                isPrime[notAPrime] = false;
        }
    }
    check_valid(isPrime);
    EXPECT_EQ(sumPrimes, 76127);  // Sum of all primes up to 1000
}

// Comprehensive tests for the erase method
TEST(EraseMethod, BasicTwoElementsErase) {
    using Map = geert::square_map<uint32_t, bool>;
    Map map;
    map[1] = true;
    map[2] = true;

    EXPECT_EQ(map.size(), 2);

    // Delete the first element
    auto it1 = map.find(1);
    EXPECT_NE(it1, map.end());
    auto next_it = map.erase(it1);
    check_valid(map);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(next_it->first, 2);       // Should point to next element
    EXPECT_EQ(map.find(1), map.end());  // Element should be gone
    EXPECT_NE(map.find(2), map.end());  // Second element should still exist

    // Delete the second element
    auto it2 = map.find(2);
    EXPECT_NE(it2, map.end());
    auto end_it = map.erase(it2);
    check_valid(map);
    EXPECT_EQ(map.size(), 0);
    EXPECT_EQ(end_it, map.end());       // Should point to end
    EXPECT_EQ(map.find(2), map.end());  // Element should be gone
    EXPECT_TRUE(map.empty());
}

TEST(EraseMethod, EraseNonExistentElements) {
    using Map = geert::square_map<uint32_t, bool>;
    Map empty_map;

    // Test on empty map - should not crash, but find will return end()
    EXPECT_EQ(empty_map.find(1), empty_map.end());

    Map non_empty_map;
    non_empty_map[1] = true;
    non_empty_map[3] = true;

    // Test finding non-existent element in non-empty map
    EXPECT_EQ(non_empty_map.find(2), non_empty_map.end());
    EXPECT_EQ(non_empty_map.find(0), non_empty_map.end());
    EXPECT_EQ(non_empty_map.find(4), non_empty_map.end());

    // Size should remain unchanged
    EXPECT_EQ(non_empty_map.size(), 2);
}

TEST(EraseMethod, EraseFromRightRange) {
    using Map = geert::square_map<uint32_t, bool>;
    Map map;

    // Insert more than kMinMergeSize entries to ensure we have items in the right range
    constexpr uint32_t num_entries = Map::kMinSplitSize + 10;
    for (uint32_t i = 1; i <= num_entries; ++i) {
        map[i] = (i % 2 == 0);
    }

    EXPECT_EQ(map.size(), num_entries);

    // Delete the last few entries (which should be in the right range)
    for (uint32_t i = num_entries; i > num_entries - 5; --i) {
        auto it = map.find(i);
        EXPECT_NE(it, map.end());
        auto next_it = map.erase(it);
        EXPECT_EQ(map.find(i), map.end());  // Should be gone

        // Check that next iterator points correctly
        if (i > num_entries - 4) {
            EXPECT_EQ(next_it, map.end());  // Last element erased
        }
    }

    EXPECT_EQ(map.size(), num_entries - 5);

    // Verify remaining elements are still there and sorted
    EXPECT_TRUE(is_strictly_sorted(map.begin(), map.end()));
    for (uint32_t i = 1; i <= num_entries - 5; ++i) {
        EXPECT_NE(map.find(i), map.end());
    }
    check_valid(map);
}

TEST(EraseMethod, EraseFromLeftRange) {
    using Map = geert::square_map<uint32_t, bool>;
    Map map;

    // Insert more than kMinSplitSize entries
    constexpr uint32_t num_entries = Map::kMinSplitSize + 10;
    for (uint32_t i = 1; i <= num_entries; ++i) {
        map[i] = (i % 2 == 0);
    }
    check_valid(map);

    EXPECT_EQ(map.size(), num_entries);

    // Delete the first few entries (which should be in the left range)
    for (uint32_t i = 1; i <= 5; ++i) {
        auto it = map.find(i);
        EXPECT_NE(it, map.end());
        auto next_it = map.erase(it);
        EXPECT_EQ(map.find(i), map.end());  // Should be gone

        // Check that next iterator points correctly
        if (i < 5) {
            EXPECT_EQ(next_it->first, i + 1);  // Should point to next element
        }
    }

    EXPECT_EQ(map.size(), num_entries - 5);

    // Verify remaining elements are still there and sorted
    EXPECT_TRUE(is_strictly_sorted(map.begin(), map.end()));
    for (uint32_t i = 6; i <= num_entries; ++i) {
        EXPECT_NE(map.find(i), map.end());
    }
}

TEST(EraseMethod, EraseAroundMergeSize) {
    using Map = geert::square_map<uint32_t, bool>;
    Map map;

    constexpr uint32_t num_entries = Map::kMinSplitSize + 20;
    for (uint32_t i = 1; i <= num_entries; ++i) {
        map[i] = (i % 3 == 0);
    }

    // Delete elements around kMinSplitSize boundary
    std::vector<uint32_t> to_delete = {
        1,  // Very first
        Map::kMinSplitSize - 2,
        Map::kMinSplitSize - 1,
        Map::kMinSplitSize,
        Map::kMinSplitSize + 1,
        Map::kMinSplitSize + 2,
        num_entries  // Very last
    };

    for (uint32_t key : to_delete) {
        auto it = map.find(key);
        EXPECT_NE(it, map.end());
        map.erase(it);
        EXPECT_EQ(map.find(key), map.end());
    }

    EXPECT_EQ(map.size(), num_entries - to_delete.size());
    EXPECT_TRUE(is_strictly_sorted(map.begin(), map.end()));

    // Verify other elements still exist
    for (uint32_t i = 1; i <= num_entries; ++i) {
        bool should_exist = std::find(to_delete.begin(), to_delete.end(), i) == to_delete.end();
        if (should_exist) {
            EXPECT_NE(map.find(i), map.end());
        } else {
            EXPECT_EQ(map.find(i), map.end());
        }
    }
}
TEST(EraseMethod, EraseAllOddNumbers) {
    using Map = geert::square_map<uint32_t, bool>;
    Map map;

    constexpr uint32_t num_entries = Map::kMinSplitSize + 50;
    for (uint32_t i = 1; i <= num_entries; ++i) {
        map[i] = (i % 2 == 0);  // Even numbers get true, odd get false
        if (i % 2 == 0) EXPECT_TRUE(map[i]);
    }

    EXPECT_EQ(map.size(), num_entries);

    // Delete all odd numbers
    for (uint32_t i = 1; i <= num_entries; i += 2) {
        auto it = map.find(i);
        EXPECT_NE(it, map.end());
        map.erase(it);
        EXPECT_EQ(map.find(i), map.end());
    }
    check_valid(map);
    EXPECT_TRUE(map[2]);

    EXPECT_EQ(map.size(), num_entries / 2);  // Should have half the elements
    EXPECT_TRUE(is_strictly_sorted(map.begin(), map.end()));

    // Verify only even numbers remain
    for (uint32_t i = 2; i <= num_entries; i += 2) {
        EXPECT_NE(map.find(i), map.end());
        EXPECT_TRUE(map[i]);  // Should be true as set above
    }

    // Verify odd numbers are gone
    for (uint32_t i = 1; i <= num_entries; i += 2) {
        EXPECT_EQ(map.find(i), map.end());
    }
}

TEST(EraseMethod, EraseAndReinsertOddNumbers) {
    using Map = geert::square_map<uint32_t, bool>;
    Map map;

    constexpr uint32_t num_entries = Map::kMinSplitSize + 30;

    // Initial insertion
    for (uint32_t i = 1; i <= num_entries; ++i) {
        map[i] = (i % 2 == 0);  // Even numbers get true, odd get false
    }

    // Delete all odd numbers
    for (uint32_t i = 1; i <= num_entries; i += 2) {
        auto it = map.find(i);
        EXPECT_NE(it, map.end());
        map.erase(it);
    }

    EXPECT_EQ(map.size(), num_entries / 2);

    // Reinsert odd numbers with different values
    for (uint32_t i = 1; i <= num_entries; i += 2) {
        map[i] = true;  // Different value from original, odd numbers now true
        EXPECT_TRUE(map[i]);
    }
    check_valid(map);

    EXPECT_EQ(map.size(), num_entries);
    EXPECT_TRUE(is_strictly_sorted(map.begin(), map.end()));

    // Verify all numbers are present with correct values
    for (uint32_t i = 1; i <= num_entries; ++i) {
        EXPECT_NE(map.find(i), map.end());
        EXPECT_EQ(map.find(i)->first, i);
        EXPECT_EQ(map.find(i)->second, true);
        EXPECT_TRUE(map[i]);  // All numbers should now be true
    }
}

TEST(EraseMethod, ComplexMixedOperations) {
    using Map = geert::square_map<uint32_t, uint32_t>;
    Map map;

    constexpr uint32_t base_size = Map::kMinSplitSize + 20;

    // Phase 1: Initial insertions
    for (uint32_t i = 1; i <= base_size; ++i) {
        map[i] = i * 10;  // Value is 10 times the key
    }

    // Phase 2: Delete some elements
    std::vector<uint32_t> deleted_keys = {1, 15, 25, 35, 45};
    if (base_size > 50) deleted_keys.push_back(base_size - 5);
    for (uint32_t key : deleted_keys) {
        auto it = map.find(key);
        if (key <= base_size) {
            EXPECT_NE(it, map.end());
            map.erase(it);
        }
        EXPECT_EQ(map.find(key), map.end());
    }

    // Phase 3: Insert some new elements
    std::vector<uint32_t> new_keys = {base_size + 100, base_size + 500, base_size + 900};
    for (uint32_t key : new_keys) {
        map[key] = key * 100;  // Different value pattern
    }

    // Phase 4: Try to reinsert some previously deleted elements
    for (uint32_t key : {15, 35}) {
        map[key] = key * 1000;  // Yet another value pattern
    }

    // Phase 5: Try to erase non-existent elements (should be no-op for find)
    for (uint32_t key : {1u, 25u, base_size + 10'000u}) {
        EXPECT_EQ(map.find(key), map.end());
    }

    // Phase 6: Verify correctness
    EXPECT_TRUE(is_strictly_sorted(map.begin(), map.end()));

    // Check that all operations resulted in expected state
    for (uint32_t i = 1; i <= base_size; ++i) {
        auto it = map.find(i);
        if (i == 1 || i == 25 || (base_size > 50 && i == (base_size - 5))) {
            // These should still be deleted
            EXPECT_EQ(it, map.end());
        } else if (i == 15 || i == 35) {
            // These were reinserted with new values
            EXPECT_NE(it, map.end());
            EXPECT_EQ(it->second, i * 1000);
        } else if (i == 45) {
            // This was deleted and not reinserted
            EXPECT_EQ(it, map.end());
        } else {
            // Original elements with original values
            EXPECT_NE(it, map.end());
            EXPECT_EQ(it->second, i * 10);
        }
    }

    // Check new elements
    for (uint32_t key : new_keys) {
        auto it = map.find(key);
        EXPECT_NE(it, map.end());
        EXPECT_EQ(it->second, key * 100);
    }

    // Phase 7: Test iteration from begin to end
    uint32_t count = 0;
    uint32_t prev_key = 0;
    for (auto it = map.begin(); it != map.end(); ++it) {
        EXPECT_GT(it->first, prev_key);  // Should be strictly increasing
        prev_key = it->first;
        count++;
    }
    EXPECT_EQ(count, map.size());

    // Phase 8: Test iteration after find operations
    auto find_it = map.find(10);
    if (find_it != map.end()) {
        uint32_t iter_count = 0;
        uint32_t last_key = find_it->first;
        for (auto it = find_it; it != map.end(); ++it) {
            EXPECT_GE(it->first, last_key);
            last_key = it->first;
            iter_count++;
        }
        EXPECT_GT(iter_count, 0);
    }
}

// Tests that expose the need for duplicate removal after merging
TEST(DuplicateRemoval, EraseFromLeftThenTriggerMerge) {
    using Map = geert::square_map<uint32_t, uint32_t>;

    // Create a container that will form valid split ranges when injected with split_index=5
    // Left range (index 0-4): sorted elements
    // Right range (index 5-9): sorted elements with overlap requirement
    Map::container_type container = {
        {10, 100}, {20, 200}, {30, 300}, {40, 400}, {50, 500},  // Left range (index 0-4)
        {5, 50},   {15, 150}, {25, 250}, {35, 350}, {60, 600}   // Right range (index 5-9)
    };

    Map map = inject_with_split_index<Map>(std::move(container), 5);

    // Debug: print split_point status
    if (map.split_point() == map.end()) {
        std::cout << "DEBUG: Map has no split (flat map)" << std::endl;
    } else {
        std::cout << "DEBUG: Map has split at key " << map.split_point()->first << std::endl;
    }

    check_valid(map);

    if (map.split_point() == map.end()) {
        GTEST_SKIP() << "Could not create split condition";
        return;
    }

    // Test that erase followed by merge works correctly
    // Erase key 10 from the map - this should create an erased element situation
    auto it = map.find(10);
    if (it != map.end()) {
        map.erase(it);

        check_valid(map);

        // Key should not be visible after erase
        EXPECT_EQ(map.find(10), map.end()) << "Key should not be visible after erase";

        map.merge();

        check_valid(map);

        // After merge, key should still not exist (was erased)
        EXPECT_EQ(map.find(10), map.end()) << "Key should not exist after merge (was erased)";
        EXPECT_EQ(map.split_point(), map.end()) << "Should be merged";
    } else {
        GTEST_SKIP() << "Test key not available";
    }
}

// Test that demonstrates the bug: std::unique only removes one duplicate, not both
TEST(DuplicateRemoval, InsertAfterEraseShouldNotUndoErasure) {
    using Map = geert::square_map<uint32_t, uint32_t>;

    // Directly create a container with erased elements (duplicates across ranges)
    // Left range: {10, 20, 30, 40, 50} where 30 is erased (also in right range)
    // Right range: {25, 30, 60} where 30 is erased (duplicate from left range)
    // Key invariant: last right (60) > last left (50) ✓
    Map::container_type container = {
        {10, 100}, {20, 200}, {30, 300}, {40, 400}, {50, 500},  // Left range
        {25, 250}, {30, 300}, {60, 600}  // Right range (30 is duplicate = erased)
    };

    // Map map = Map::inject(std::move(container), 5);  // Split at index 5
    // map.merge();
    // check_valid(map);

    // // Verify that key 30 is correctly erased (invisible due to duplicates)
    // EXPECT_EQ(map.find(30), map.end()) << "Key 30 should be erased (appears as duplicate)";

    // check_valid(map);
}

// Tests for the public merge() method
class MergeMethod : public ::testing::Test {
protected:
    using Map = geert::square_map<uint32_t, uint32_t>;

    void SetUp() override {
        // Create a test map with split ranges for testing
        map.clear();
    }

    Map map;

    // Helper to create a map with split ranges using inject method
    void create_split_map() {
        // Create a container with two sorted ranges that satisfy square_map invariants:
        // Left range: {10, 20, 30, 40, 50}
        // Right range: {5, 15, 25, 35, 60}
        // Invariants: split_key(5) < last_left(50) AND last_right(60) > last_left(50)
        Map::container_type container = {
            {10, 100}, {20, 200}, {30, 300}, {40, 400}, {50, 500},  // Left range (sorted)
            {5, 50},   {15, 150}, {25, 250}, {35, 350}, {60, 600}   // Right range (sorted)
        };

        // Create split map with split at index 5 (between ranges)
        map = inject_with_split_index<Map>(std::move(container), 5);

        EXPECT_NE(map.split_point(), map.end())
            << "Failed to create split map - inject method should guarantee split";
        check_valid(map);
    }

    // Helper to create erased elements (duplicates in both ranges)
    void create_erased_elements() {
        // Create a container with overlapping elements (duplicates)
        // Left range: {10, 20, 30, 40, 50}
        // Right range: {5, 15, 20, 30, 60} - keys 20,30 are duplicates
        // Invariants: split_key(5) < last_left(50) AND last_right(60) > last_left(50)
        Map::container_type container = {
            {1, 10},   {2, 20},   {3, 30},   {7, 70},   {9, 90},  // Extra elements to ensure split
            {10, 100}, {20, 200}, {30, 300}, {40, 400}, {50, 500},  // Left range has 10 elems
            {5, 50},   {60, 600}                                    // Right range with duplicates
        };

        // Create split map with split at index 5
        map = inject_with_split_index<Map>(std::move(container), 10);
        EXPECT_EQ(map.size(), 12);  // 10 left + 2 right
        map.erase(map.find(20));    // Erase key 20 from left range
        map.erase(map.find(30));    // Erase key 30 from left range

        // Check size
        EXPECT_EQ(map.size(), 10);  // 12 original - 2 erased = 10
        check_valid(map);
        EXPECT_NE(map.split_point(), map.end())
            << "Failed to create split map with erased elements";

        // Extract the container from a copy of the  map and verify that its size reflects erased
        auto new_map = map;  // Copy
        auto new_container = std::move(new_map).extract();
        EXPECT_EQ(new_container.size(), 14)  // 12 original + 2 duplicates
            << "Extracted container size should reflect erased elements";
    }
};

TEST_F(MergeMethod, MergeEmptyMap) {
    // Test merge on empty map
    EXPECT_TRUE(map.empty());

    map.merge();

    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.split_point(), map.end());
    check_valid(map);
}

TEST_F(MergeMethod, MergeSingleRangeMap) {
    // Test merge on map that's already in single range (flat map state)
    for (uint32_t i = 1; i <= 10; ++i) {
        map[i] = i * 10;
    }

    EXPECT_EQ(map.split_point(), map.end());  // Should be flat already
    auto original_size = map.size();

    map.merge();

    EXPECT_EQ(map.size(), original_size);
    EXPECT_EQ(map.split_point(), map.end());
    EXPECT_TRUE(is_strictly_sorted(map.begin(), map.end()));
    check_valid(map);
}

TEST_F(MergeMethod, MergeSplitRangesNoErased) {
    // Test merge on split map with no erased elements
    create_split_map();

    EXPECT_NE(map.split_point(), map.end());  // Should be split
    auto original_size = map.size();

    map.merge();

    EXPECT_EQ(map.size(), original_size);     // Size should be preserved
    EXPECT_EQ(map.split_point(), map.end());  // Should be flat after merge
    EXPECT_TRUE(is_strictly_sorted(map.begin(), map.end()));
    check_valid(map);

    // Verify all original elements are still present and accessible
    std::vector<uint32_t> expected_keys = {5, 10, 15, 20, 25, 30, 35, 40, 50, 60};
    for (uint32_t key : expected_keys) {
        EXPECT_NE(map.find(key), map.end()) << "Key " << key << " should exist after merge";
        // All values follow pattern: key * 10, except right-range key 5 has value 50
        auto expected_value = (key == 5) ? 50 : key * 10;
        EXPECT_EQ(map[key], expected_value) << "Value for key " << key << " should be preserved";
    }
}

TEST_F(MergeMethod, MergeSplitRangesWithErased) {
    // Test merge on split map with erased elements (duplicates)
    create_erased_elements();

    // Only proceed if we have a split
    if (map.split_point() == map.end()) {
        GTEST_FAIL() << "Could not create split condition with erased elements";
        return;
    }

    auto original_size = map.size();

    // Track which keys should be missing after merge
    std::vector<uint32_t> erased_keys = {20, 30};
    std::vector<uint32_t> missing_keys;
    for (uint32_t key : erased_keys) {
        EXPECT_EQ(map.find(key), map.end()) << "Key " << key << " should be erased before merge";
        missing_keys.push_back(key);
    }

    map.merge();

    EXPECT_EQ(map.size(), original_size);     // Size should be preserved
    EXPECT_EQ(map.split_point(), map.end());  // Should be flat after merge
    check_valid(map);

    // Verify erased keys are still missing
    for (uint32_t key : missing_keys) {
        EXPECT_EQ(map.find(key), map.end())
            << "Key " << key << " should still be erased after merge";
    }

    // Verify non-erased keys are still present
    std::vector<uint32_t> remaining_keys = {1, 3, 5, 7, 9, 10};
    for (uint32_t key : remaining_keys) {
        auto it = map.find(key);
        if (it != map.end()) {  // Only check if it existed before merge
            EXPECT_EQ(it->second, key * 10) << "Value for key " << key << " should be preserved";
        }
    }
}

TEST_F(MergeMethod, MergeStability) {
    // Test that merge preserves stability - elements visible before merge remain visible after

    // Create a split map with no erased elements using inject
    create_split_map();

    if (map.split_point() == map.end()) {
        GTEST_SKIP() << "Could not create split condition for stability test";
        return;
    }

    // Before merge, record the visible elements (no erased elements in this test)
    std::vector<std::pair<uint32_t, uint32_t>> before_merge;
    for (const auto& pair : map) {
        before_merge.emplace_back(pair.first, pair.second);
    }

    map.merge();

    // After merge, verify same elements exist in same order
    std::vector<std::pair<uint32_t, uint32_t>> after_merge;
    for (const auto& pair : map) {
        after_merge.emplace_back(pair.first, pair.second);
    }

    EXPECT_EQ(before_merge, after_merge)
        << "Merge should preserve element order and values for non-erased elements";
    EXPECT_EQ(map.split_point(), map.end());
    check_valid(map);
}

TEST_F(MergeMethod, MergeMultipleTimes) {
    // Test that calling merge() multiple times is safe and idempotent
    create_erased_elements();

    // Only proceed if we have appropriate conditions
    if (map.split_point() == map.end()) {
        GTEST_SKIP() << "Could not create split condition for multiple merge test";
        return;
    }

    auto original_size = map.size();

    // First merge
    map.merge();
    EXPECT_EQ(map.split_point(), map.end());
    check_valid(map);
    auto size_after_first = map.size();

    // Second merge (should be no-op)
    map.merge();
    EXPECT_EQ(map.split_point(), map.end());
    EXPECT_EQ(map.size(), size_after_first);
    check_valid(map);

    // Third merge (should still be no-op)
    map.merge();
    EXPECT_EQ(map.split_point(), map.end());
    EXPECT_EQ(map.size(), size_after_first);
    check_valid(map);

    EXPECT_TRUE(is_strictly_sorted(map.begin(), map.end()));
}

TEST_F(MergeMethod, MergeAfterOperations) {
    // Test merge after various operations
    create_split_map();

    // Add more elements
    map[200] = 2000;
    map[150] = 1500;

    // Erase some elements
    auto it = map.find(5);
    if (it != map.end()) map.erase(it);

    check_valid(map);

    // Merge should consolidate everything
    map.merge();

    EXPECT_EQ(map.split_point(), map.end());
    EXPECT_TRUE(is_strictly_sorted(map.begin(), map.end()));
    check_valid(map);

    // Should be able to continue using the map normally after merge
    map[300] = 3000;
    EXPECT_EQ(map[300], 3000);
    EXPECT_EQ(map.find(5), map.end());  // Should still be erased

    check_valid(map);
}

// Tests for the inject method (opposite of extract)
class InjectMethod : public ::testing::Test {
protected:
    using Map = geert::square_map<uint32_t, uint32_t>;
    using Container = std::vector<std::pair<uint32_t, uint32_t>>;
};

TEST_F(InjectMethod, InjectEmptyContainer) {
    Container empty_container;

    Map map;
    map.replace(std::move(empty_container));

    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0);
    EXPECT_EQ(map.split_point(), map.end());
    check_valid(map);
}

TEST_F(InjectMethod, InjectFlatContainer) {
    Container container = {{1, 10}, {2, 20}, {3, 30}, {4, 40}, {5, 50}};

    // Inject as flat map (no split)
    auto map = inject_with_split_index<Map>(std::move(container), 0);

    EXPECT_EQ(map.size(), 5);
    EXPECT_EQ(map.split_point(), map.end());  // No split
    EXPECT_TRUE(is_strictly_sorted(map.begin(), map.end()));
    check_valid(map);

    // Verify all elements are accessible
    for (uint32_t i = 1; i <= 5; ++i) {
        EXPECT_NE(map.find(i), map.end());
        EXPECT_EQ(map[i], i * 10);
    }
}

TEST_F(InjectMethod, InjectSplitContainer) {
    Container container = {{1, 10}, {3, 30}, {5, 50}, {2, 20}, {4, 40}, {6, 60}};

    // Inject with split at position 3 (elements 0,1,2 in left range, 3,4,5 in right range)
    auto map = inject_with_split_index<Map>(std::move(container), 3);

    EXPECT_EQ(map.size(), 6);
    EXPECT_NE(map.split_point(), map.end());  // Should have split
    EXPECT_EQ(map.split_point()->first, 2);   // First element of right range
    check_valid(map);

    // Verify all elements are accessible
    for (uint32_t i = 1; i <= 6; ++i) {
        EXPECT_NE(map.find(i), map.end()) << "Key " << i << " should exist";
        EXPECT_EQ(map[i], i * 10);
    }

    // Verify iteration produces sorted order
    std::vector<uint32_t> keys;
    for (const auto& pair : map) {
        keys.push_back(pair.first);
    }
    EXPECT_EQ(keys, (std::vector<uint32_t>{1, 2, 3, 4, 5, 6}));
}

TEST_F(InjectMethod, InjectWithIterator) {
    Container container = {{10, 100}, {30, 300}, {50, 500}, {20, 200}, {60, 600}};

    // Split at iterator pointing to 4th element (index 3) -> {20, 60}
    auto split_it = container.begin() + 3;
    Map map;
    map.replace(std::move(container), split_it);

    EXPECT_EQ(map.size(), 5);
    EXPECT_NE(map.split_point(), map.end());
    EXPECT_EQ(map.split_point()->first, 20);  // First element of right range

    // Verify elements are accessible through the map interface
    for (const auto& expected : {10, 20, 30, 50, 60}) {
        EXPECT_NE(map.find(expected), map.end()) << "Key " << expected << " should exist";
        EXPECT_EQ(map[expected], expected * 10);
    }
}

TEST_F(InjectMethod, InjectConstContainer) {
    const Container container = {{1, 10}, {2, 20}, {3, 30}};

    auto map = inject_with_split_index<Map>(container, 0);  // Use const version

    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map.split_point(), map.end());
    check_valid(map);

    for (uint32_t i = 1; i <= 3; ++i) {
        EXPECT_EQ(map[i], i * 10);
    }
}

TEST_F(InjectMethod, InjectExtractRoundTrip) {
    // Create original map
    Map original;
    for (uint32_t i = 1; i <= 10; ++i) {
        original[i] = i * 10;
    }

    // Force a split by adding elements in reverse order
    for (uint32_t i = 20; i > 15; --i) {
        original[i] = i * 10;
        if (original.split_point() != original.end()) break;
    }

    check_valid(original);
    auto original_size = original.size();
    auto has_split = original.split_point() != original.end();

    // Extract and inject
    auto container = std::move(original).extract();
    auto reconstructed = inject_with_split_index<Map>(std::move(container),
                                                      has_split ? 10 : 0);  // Approximate split

    EXPECT_EQ(reconstructed.size(), original_size);
    check_valid(reconstructed);

    // Verify elements are preserved (though split point might be different)
    for (uint32_t i = 1; i <= 10; ++i) {
        EXPECT_NE(reconstructed.find(i), reconstructed.end());
        EXPECT_EQ(reconstructed[i], i * 10);
    }
}

TEST_F(InjectMethod, InjectInvalidSplitIndex) {
    Container container = {{1, 10}, {2, 20}, {3, 30}};

    // Split index beyond container size should be treated as flat map (split = 0)
    auto map = inject_with_split_index<Map>(std::move(container), 100);

    EXPECT_EQ(map.size(), 3);
    // With split_index >= container.size(), should be treated as flat
    EXPECT_EQ(map.split_point(), map.end());  // Should be flat

    // Verify basic functionality works
    EXPECT_EQ(map[1], 10);
    EXPECT_EQ(map[2], 20);
    EXPECT_EQ(map[3], 30);
    check_valid(map);
}

// Tests for the replace method
class ReplaceMethod : public ::testing::Test {
protected:
    using Map = geert::square_map<uint32_t, uint32_t>;
    using Container = std::vector<std::pair<uint32_t, uint32_t>>;
};

TEST_F(ReplaceMethod, ReplaceEmptyContainer) {
    Map original;
    original.insert({1, 10});
    original.insert({2, 20});
    original.insert({3, 30});
    check_valid(original);
    EXPECT_EQ(original.size(), 3);

    Container empty_container;
    original.replace(std::move(empty_container));

    EXPECT_TRUE(original.empty());
    EXPECT_EQ(original.size(), 0);
    EXPECT_EQ(original.split_point(), original.end());  // Should be flat
    check_valid(original);
}

TEST_F(ReplaceMethod, ReplaceWithSortedContainer) {
    Map original;
    original.insert({5, 50});
    original.insert({6, 60});
    check_valid(original);

    Container new_container{{1, 10}, {2, 20}, {3, 30}, {4, 40}};
    original.replace(std::move(new_container));

    EXPECT_EQ(original.size(), 4);
    EXPECT_EQ(original.split_point(), original.end());  // Should be flat after replace
    check_valid(original);

    // Verify all elements are accessible
    for (uint32_t i = 1; i <= 4; ++i) {
        EXPECT_EQ(original[i], i * 10);
    }
}

TEST_F(ReplaceMethod, ReplacePreservesContainer) {
    Map original;
    original.insert({10, 100});

    Container source_container{{1, 10}, {2, 20}, {3, 30}};
    auto container_copy = source_container;  // Keep a copy for verification

    original.replace(std::move(source_container));

    // Verify the move occurred (source should be empty)
    EXPECT_TRUE(source_container.empty());

    // Verify original now has the contents
    EXPECT_EQ(original.size(), 3);
    for (uint32_t i = 1; i <= 3; ++i) {
        EXPECT_EQ(original[i], i * 10);
    }
    check_valid(original);
}

TEST_F(ReplaceMethod, ReplaceResetsState) {
    // Create a map with split state and erased elements
    Map original;
    original.insert({1, 10});
    original.insert({3, 30});
    original.insert({2, 20});  // This should create a split

    // Check that we have a split state
    auto has_split_initially = original.split_point() != original.end();

    // Erase an element to create erased state
    auto it = original.find(2);
    if (it != original.end()) {
        original.erase(it);
    }

    Container new_container{{4, 40}, {5, 50}, {6, 60}};
    original.replace(std::move(new_container));

    // After replace, should be in flat state with no erased elements
    EXPECT_EQ(original.size(), 3);
    EXPECT_EQ(original.split_point(), original.end());  // Should be flat
    check_valid(original);

    // Verify new contents
    for (uint32_t i = 4; i <= 6; ++i) {
        EXPECT_EQ(original[i], i * 10);
    }
}

TEST_F(ReplaceMethod, ExtractReplaceRoundTrip) {
    // Create original map with various states
    Map original;
    for (uint32_t i = 1; i <= 5; ++i) {
        original.insert({i, i * 10});
    }
    check_valid(original);
    auto original_size = original.size();

    // Extract container
    auto container = std::move(original).extract();
    EXPECT_EQ(container.size(), original_size);

    // Create new map and replace with extracted container
    Map reconstructed;
    reconstructed.replace(std::move(container));

    EXPECT_EQ(reconstructed.size(), original_size);
    EXPECT_EQ(reconstructed.split_point(), reconstructed.end());  // Should be flat
    check_valid(reconstructed);

    // Verify all elements are preserved
    for (uint32_t i = 1; i <= 5; ++i) {
        EXPECT_EQ(reconstructed[i], i * 10);
    }
}

TEST_F(ReplaceMethod, ReplaceAfterComplexOperations) {
    Map original;

    // Perform various operations to get the map in a complex state
    for (uint32_t i = 10; i >= 1; --i) {
        original.insert({i, i * 10});
    }

    // Erase some elements
    original.erase(original.find(3));
    original.erase(original.find(7));

    // Insert some more to potentially create splits
    original.insert({15, 150});
    original.insert({12, 120});

    check_valid(original);
    auto size_before = original.size();

    // Replace with simple container
    Container simple{{100, 1000}, {200, 2000}, {300, 3000}};
    original.replace(std::move(simple));

    EXPECT_EQ(original.size(), 3);
    EXPECT_EQ(original.split_point(), original.end());  // Should be flat
    check_valid(original);

    EXPECT_EQ(original[100], 1000);
    EXPECT_EQ(original[200], 2000);
    EXPECT_EQ(original[300], 3000);
}

TEST_F(ReplaceMethod, ReplaceWithSplitAtBeginning) {
    Map original;
    original.insert({10, 100});
    original.insert({20, 200});

    Container new_container{{1, 10}, {2, 20}, {3, 30}, {4, 40}, {5, 50}};
    auto split_it = new_container.begin();  // Split at beginning (flat map)

    original.replace(std::move(new_container), split_it);

    EXPECT_EQ(original.size(), 5);
    EXPECT_EQ(original.split_point(), original.end());  // Should be flat (split at beginning)
    check_valid(original);

    for (uint32_t i = 1; i <= 5; ++i) {
        EXPECT_EQ(original[i], i * 10);
    }
}

TEST_F(ReplaceMethod, ReplaceWithSplitAtEnd) {
    Map original;
    original.insert({10, 100});

    Container new_container{{1, 10}, {2, 20}, {3, 30}, {4, 40}, {5, 50}};
    auto split_it = new_container.end();  // Split at end (flat map)

    original.replace(std::move(new_container), split_it);

    EXPECT_EQ(original.size(), 5);
    EXPECT_EQ(original.split_point(), original.end());  // Should be flat (split at end)
    check_valid(original);

    for (uint32_t i = 1; i <= 5; ++i) {
        EXPECT_EQ(original[i], i * 10);
    }
}

TEST_F(ReplaceMethod, ReplaceWithSplitInMiddle) {
    Map original;
    original.insert({100, 1000});

    Container new_container{{1, 10}, {3, 30}, {5, 50}, {2, 20}, {4, 40}, {6, 60}};
    auto split_it = new_container.begin() + 3;  // Split after {5, 50}

    original.replace(std::move(new_container), split_it);

    EXPECT_EQ(original.size(), 6);
    EXPECT_NE(original.split_point(), original.end());  // Should have a split
    check_valid(original);

    // Verify all elements are accessible
    EXPECT_EQ(original[1], 10);
    EXPECT_EQ(original[2], 20);
    EXPECT_EQ(original[3], 30);
    EXPECT_EQ(original[4], 40);
    EXPECT_EQ(original[5], 50);
    EXPECT_EQ(original[6], 60);
}

TEST_F(ReplaceMethod, ReplaceWithSplitPreservesIterator) {
    Map original;
    original.insert({99, 990});

    Container source_container{{10, 100}, {30, 300}, {50, 500}, {20, 200}, {60, 600}};
    auto split_it = source_container.begin() + 3;  // Split after {50, 500}

    original.replace(std::move(source_container), split_it);

    // Verify the move occurred (source should be empty)
    EXPECT_TRUE(source_container.empty());

    // Verify the split was established correctly
    EXPECT_EQ(original.size(), 5);
    EXPECT_NE(original.split_point(), original.end());  // Should have a split
    check_valid(original);

    EXPECT_EQ(original[10], 100);
    EXPECT_EQ(original[20], 200);
    EXPECT_EQ(original[30], 300);
    EXPECT_EQ(original[50], 500);
    EXPECT_EQ(original[60], 600);
}

TEST_F(ReplaceMethod, ReplaceWithSplitResetsErasedCount) {
    Map original;
    // Create a map with erased elements
    original.insert({1, 10});
    original.insert({2, 20});
    original.insert({3, 30});

    // Erase an element to create erased state
    auto it = original.find(2);
    if (it != original.end()) {
        original.erase(it);
    }

    Container new_container{{4, 40}, {6, 60}, {8, 80}, {5, 50}, {9, 90}};
    auto split_it = new_container.begin() + 3;  // Split after {8, 80}

    original.replace(std::move(new_container), split_it);

    // After replace, erased count should be reset
    EXPECT_EQ(original.size(), 5);                      // All elements should be accessible
    EXPECT_NE(original.split_point(), original.end());  // Should have a split
    check_valid(original);

    // Verify all elements are accessible
    EXPECT_EQ(original[4], 40);
    EXPECT_EQ(original[5], 50);
    EXPECT_EQ(original[6], 60);
    EXPECT_EQ(original[8], 80);
    EXPECT_EQ(original[9], 90);
}
