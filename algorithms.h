#pragma once
#include <algorithm>
#include <iterator>
#include <vector>

namespace geert {

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

template <typename ForwardIt, typename Compare>
ForwardIt remove_duplicates(ForwardIt first, ForwardIt last, Compare comp) {
    if (first == last) return last;

    // Skip past non-duplicated elements
    while (first + 1 != last && comp(*first, *(first + 1))) ++first;

    // If a single element remains, all elements were unique and we're done
    if (first + 1 == last) return last;

    // Found a duplicate element. From here on we'll need to move elements for compaction.
    auto write_it = first;
    while (first + 1 != last) {
        // Loop invariant: first points a sequence of two or more duplicate elements, all
        // of which we need to remove from the output. Start with skipping the first one.
        ++first;

        // Skip past any additional duplicates beyond a single pair.
        while (first + 1 != last && !comp(*first, *(first + 1))) ++first;

        // Finally skip the last of the duplicate elements
        if (++first == last) break;

        // Move any unique elements
        while (first + 1 != last && comp(*first, *(first + 1))) *write_it++ = std::move(*first++);
    }
    if (first != last) *write_it++ = std::move(*first++);

    return write_it;  // erasing elements beyond write_it to be done by caller
}

}  // namespace geert
