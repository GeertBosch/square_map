#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <algorithm>
#include <cstdint>
#include <random>
#include <numeric>

#include "square_map.h"

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
