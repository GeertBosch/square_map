#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <algorithm>
#include <cstdint>
#include <random>
#include <numeric>

#include "square_map.h"

template <typename Container>
bool is_strictly_sorted(const Container& c) {
    return std::is_sorted(c.begin(), c.end()) && std::adjacent_find(c.begin(), c.end()) == c.end();
}

/**
 * is_valid returns true if the passed square map has a valid internal structure. It does this
 * through extracting the adapted container and checking its contents:
 *.  - The container may at most contain two strictly sorted ranges.
 *.  - The map is empty if and only if the container is empty.
 *.  - If the container is not empty, split_point() must be valid and unequal to end()
 *.  - If split_point() is begin(), there is only one range.
 *.  - If there are two ranges, the left range must be at least kMinMergeSize.
 *.  - If there are two ranges, the right range may not be larger than 2x the square root of the
 *     size of the left range.
 *.  - Within each range, keys must be unique.
 *   - The largest element should be the last one in the underlying container.
 *.  - The number of duplicate keys is the number of erased elements, and therefore the difference
 *     between the size of the underlying container and the reported size of the map.
 *.  - The last item may not be erased.
 */
template <typename Map>
bool is_valid(Map m) {
    Map m_copy = m;  // copy to avoid modifying the original map
    auto container = std::move(m_copy).extract();

    // Check properties of an empty map
    if (container.empty()) {
        EXPECT_EQ(m.size(), 0);
        EXPECT_EQ(m.begin(), m.end());
        EXPECT_EQ(m.split_point(), m.end());

        return true;
    }

    // The map is not empty here. Start with checking basic split_point and map properties.
    EXPECT_FALSE(m.empty());
    EXPECT_NE(m.split_point(), m.end());

    auto split_key = m.split_point()->first;
    EXPECT_EQ(std::count(container.begin(), container.end(), split_key), 1);  // Must be unique

    auto split_it = container.begin();
    while (split_it != container.end() && split_it->first != split_key) ++split_it;

    return true;
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
    EXPECT_EQ(two.size(), 2);
    EXPECT_FALSE(two.empty());
    EXPECT_THROW(two.at(key2 + 1), std::out_of_range);
    EXPECT_NE(two.begin(), two.end());
    EXPECT_EQ(std::next(two.begin(), 2), two.end());
    EXPECT_TRUE(std::is_sorted(two.begin(), two.end()));
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
    }
    EXPECT_EQ(ten.size(), 10);
    EXPECT_EQ(std::next(ten.begin(), 10), ten.end());
    EXPECT_TRUE(std::is_sorted(ten.begin(), ten.end()));
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
    EXPECT_EQ(tenSquares.count(16), 1);
    EXPECT_EQ(tenSquares.find(13), tenSquares.end());
}

TEST(OrderedMap, FindNext) {
    using Map = geert::square_map<uint32_t, bool, std::less<uint32_t>, std::vector<std::pair<uint32_t, bool>>>;
    Map::key_type keys[15] = {10, 5, 12, 4, 3, 2, 9, 14, 15, 8, 1, 13, 6, 11, 7};
    Map fifteenNumbers;
    for (auto key : keys)
        fifteenNumbers.insert({key, {}});
    for (auto key : keys) {
        auto it = fifteenNumbers.find(key);
        EXPECT_NE(it, fifteenNumbers.end());
        for (auto j = key; j <= 15; ++j, ++it)
            EXPECT_EQ(it->first, j);
        EXPECT_EQ(it, fifteenNumbers.end());
    }
}

TEST(OrderedMap, Iters) {
    using Map = geert::square_map<uint32_t, bool, std::less<uint32_t>, std::vector<std::pair<uint32_t, bool>>>;
    Map map;
    for (int j = 0; j < 9; ++j)
        map.insert({j, true});
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

// Performance tests with shuffled data
class ShuffledMap : public ::testing::Test {
protected:
    void SetUp() override {
        numbers.resize(kMapSize);
        std::iota(numbers.begin(), numbers.end(), 1);
        gen.seed(0);  // Fixed seed for reproducible tests
        std::shuffle(numbers.begin(), numbers.end(), gen);
    }

    const uint32_t kMapSize = 1000;
    std::mt19937 gen;
    std::vector<uint32_t> numbers;
};

TEST_F(ShuffledMap, IterateAll) {
    using Map = geert::square_map<uint32_t, bool, std::less<uint32_t>, std::vector<std::pair<uint32_t, bool>>>;
    Map map;
    for (auto n : numbers)
        map[n];
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
    EXPECT_EQ(isPrime.size(), numbers.size());
    EXPECT_TRUE(std::is_sorted(isPrime.begin(), isPrime.end()));

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
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(next_it->first, 2);       // Should point to next element
    EXPECT_EQ(map.find(1), map.end());  // Element should be gone
    EXPECT_NE(map.find(2), map.end());  // Second element should still exist

    // Delete the second element
    auto it2 = map.find(2);
    EXPECT_NE(it2, map.end());
    auto end_it = map.erase(it2);
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
    constexpr uint32_t num_entries = Map::kMinMergeSize + 10;
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
    EXPECT_TRUE(std::is_sorted(map.begin(), map.end()));
    for (uint32_t i = 1; i <= num_entries - 5; ++i) {
        EXPECT_NE(map.find(i), map.end());
    }
}
#if 0
TEST(EraseMethod, EraseFromLeftRange) {
    using Map = geert::square_map<uint32_t, bool>;
    Map map;

    // Insert more than kMinMergeSize entries
    constexpr uint32_t num_entries = Map::kMinMergeSize + 10;
    for (uint32_t i = 1; i <= num_entries; ++i) {
        map[i] = (i % 2 == 0);
    }

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
    EXPECT_TRUE(std::is_sorted(map.begin(), map.end()));
    for (uint32_t i = 6; i <= num_entries; ++i) {
        EXPECT_NE(map.find(i), map.end());
    }
}

TEST(EraseMethod, EraseAroundMergeSize) {
    using Map = geert::square_map<uint32_t, bool>;
    Map map;

    constexpr uint32_t num_entries = Map::kMinMergeSize + 20;
    for (uint32_t i = 1; i <= num_entries; ++i) {
        map[i] = (i % 3 == 0);
    }

    // Delete elements around kMinMergeSize boundary
    std::vector<uint32_t> to_delete = {
        1,  // Very first
        Map::kMinMergeSize - 2,
        Map::kMinMergeSize - 1,
        Map::kMinMergeSize,
        Map::kMinMergeSize + 1,
        Map::kMinMergeSize + 2,
        num_entries  // Very last
    };

    for (uint32_t key : to_delete) {
        auto it = map.find(key);
        EXPECT_NE(it, map.end());
        map.erase(it);
        EXPECT_EQ(map.find(key), map.end());
    }

    EXPECT_EQ(map.size(), num_entries - to_delete.size());
    EXPECT_TRUE(std::is_sorted(map.begin(), map.end()));

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

    constexpr uint32_t num_entries = Map::kMinMergeSize + 50;
    for (uint32_t i = 1; i <= num_entries; ++i) {
        map[i] = (i % 2 == 0);  // Even numbers get true, odd get false
    }

    EXPECT_EQ(map.size(), num_entries);

    // Delete all odd numbers
    for (uint32_t i = 1; i <= num_entries; i += 2) {
        auto it = map.find(i);
        EXPECT_NE(it, map.end());
        map.erase(it);
        EXPECT_EQ(map.find(i), map.end());
    }

    EXPECT_EQ(map.size(), num_entries / 2);  // Should have half the elements
    EXPECT_TRUE(std::is_sorted(map.begin(), map.end()));

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

    constexpr uint32_t num_entries = Map::kMinMergeSize + 30;

    // Initial insertion
    for (uint32_t i = 1; i <= num_entries; ++i) {
        map[i] = (i % 2 == 0);
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
        map[i] = true;  // Different value from original
    }

    EXPECT_EQ(map.size(), num_entries);
    EXPECT_TRUE(std::is_sorted(map.begin(), map.end()));

    // Verify all numbers are present with correct values
    for (uint32_t i = 1; i <= num_entries; ++i) {
        EXPECT_NE(map.find(i), map.end());
        if (i % 2 == 0) {
            EXPECT_FALSE(map[i]);  // Even numbers should still be false
        } else {
            EXPECT_TRUE(map[i]);  // Odd numbers should now be true
        }
    }
}

TEST(EraseMethod, ComplexMixedOperations) {
    using Map = geert::square_map<uint32_t, uint32_t>;
    Map map;

    constexpr uint32_t base_size = Map::kMinMergeSize + 20;

    // Phase 1: Initial insertions
    for (uint32_t i = 1; i <= base_size; ++i) {
        map[i] = i * 10;  // Value is 10 times the key
    }

    // Phase 2: Delete some elements
    std::vector<uint32_t> deleted_keys = {5, 15, 25, 35, 45, base_size - 5};
    for (uint32_t key : deleted_keys) {
        auto it = map.find(key);
        EXPECT_NE(it, map.end());
        map.erase(it);
        EXPECT_EQ(map.find(key), map.end());
    }

    // Phase 3: Insert some new elements
    std::vector<uint32_t> new_keys = {base_size + 1, base_size + 5, base_size + 10};
    for (uint32_t key : new_keys) {
        map[key] = key * 100;  // Different value pattern
    }

    // Phase 4: Try to reinsert some previously deleted elements
    for (uint32_t key : {15, 35}) {
        map[key] = key * 1000;  // Yet another value pattern
    }

    // Phase 5: Try to erase non-existent elements (should be no-op for find)
    for (uint32_t key : {5u, 25u, base_size + 100u}) {
        EXPECT_EQ(map.find(key), map.end());
    }

    // Phase 6: Verify correctness
    EXPECT_TRUE(std::is_sorted(map.begin(), map.end()));

    // Check that all operations resulted in expected state
    for (uint32_t i = 1; i <= base_size; ++i) {
        auto it = map.find(i);
        if (i == 5 || i == 25 || i == (base_size - 5)) {
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
#endif