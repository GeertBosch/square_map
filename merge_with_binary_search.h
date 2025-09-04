#pragma once
#include <algorithm>
#include <iterator>
#include <vector>

template <typename RandomIt, typename Compare>
void merge_with_binary_search(RandomIt first, RandomIt middle, RandomIt last, Compare comp) {
    using value_type = typename std::iterator_traits<RandomIt>::value_type;
    
    std::vector<value_type> buffer(std::make_move_iterator(middle), 
                                   std::make_move_iterator(last));

    for (auto it = buffer.rbegin(); it != buffer.rend(); ++it) {
        // Find the start of the range in [first, middle) that comes after *it
        auto pos = std::upper_bound(first, middle, *it, comp);
        
        last = std::move_backward(pos, middle, last);
        middle = pos;
        *--last = std::move(*it);
    }
}

template <typename RandomIt>
void merge_with_binary_search(RandomIt first, RandomIt middle, RandomIt last) {
    using value_type = typename std::iterator_traits<RandomIt>::value_type;
    merge_with_binary_search(first, middle, last, std::less<value_type>());
}
