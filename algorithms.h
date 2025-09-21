#pragma once
#include <algorithm>
#include <iterator>
#include <vector>

namespace geert {

/**
 * Merges two consecutive sorted ranges [first, middle) and [middle, last) in place using a
 * binary search. The merge is stable, i.e. elements that compare equal maintain their relative
 * order. The algorithm has the same semantics as std::inplace_merge, but is optimized for cases
 * where the second range is much smaller than the first.
 * This implementation uses O(N) additional space, where N is the size of the second
 * range. The time complexity is O(N log(M)) comparisons and O(M + N) moves, where M and N are
 * the sizes of the first and second ranges respectively.
 */
template <typename RandomIt,
          typename Compare = std::less<typename std::iterator_traits<RandomIt>::value_type>>
void merge_with_binary_search(RandomIt first, RandomIt middle, RandomIt last, Compare comp = {}) {
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

/**
 * Removes consecutive duplicate elements from a sorted range [first, last).
 *
 * The range must be sorted according to the provided comparison function. The function returns an
 * iterator to the new logical end of the range, with duplicates removed. Elements beyond this point
 * are left in a valid but unspecified state and should be erased by the caller if necessary. The
 * remaining elements are those that had a unique key in the original range.
 *
 * This implementation performs O(N) comparisons and O(D) moves, where N is the number of elements
 * in the range and D is the distance between the first duplicate and the end of the range.
 *
 * Example:
 *   std::vector<int> vec = {1, 2, 2, 3, 4, 4, 4, 5};
 *   auto new_end = remove_duplicates(vec.begin(), vec.end());
 *   vec.erase(new_end, vec.end()); // The vector now contains {1, 3, 5}
 */
template <typename ForwardIt,
          typename Compare = std::less<typename std::iterator_traits<ForwardIt>::value_type>>
ForwardIt remove_duplicates(ForwardIt first, ForwardIt last, Compare comp = {}) {
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
