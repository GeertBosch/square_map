#pragma once
#include <algorithm>
#include <iterator>
#include <vector>

#include "algorithms.h"

namespace geert {

template <class Key,
          class T,
          class Compare = std::less<Key>,
          class Container = std::vector<std::pair<Key, T>>>
class square_map {
public:
#if defined(DEBUG)
    static constexpr std::size_t kMinSplitSize = 5;  // Below this size treat as a flat map
#else
    static constexpr std::size_t kMinSplitSize = 50;  // Below this size treat as a flat map
#endif
    using key_type = Key;
    using mapped_type = T;
    using container_type = Container;
    using value_type = typename container_type::value_type;
    using container_iterator = typename container_type::iterator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using key_compare = Compare;
    using allocator_type = typename container_type::allocator_type;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;

    struct value_key_compare {
        constexpr bool operator()(const value_type& left, const key_type& right) const {
            return Compare()(left.first, right);
        }
        constexpr bool operator()(const key_type& left, const value_type& right) const {
            return Compare()(left, right.first);
        }
    };

    struct value_compare {
        constexpr bool operator()(const value_type& left, const value_type& right) const {
            return Compare()(left.first, right.first);
        }
    };

    class const_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = const square_map::value_type;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;

        const_iterator& operator++() {
            // Precondition: s0 and s1 are dereferenceable.

            auto initial_key = s0->first;

            while (true) {
                // Case: s0 == s1 - we're at the last element, advance to end
                if (s0 == s1) {
                    ++s0;     // This makes s0 point to end()
                    s1 = s0;  // Set s1 to end() as well to match cend() behavior
                    return *this;
                }

                // Advance s0 to get s0'
                ++s0;
                auto next_key = s0->first;

                // Case: s0' = s1 - we're at the last element after advancement
                if (s0 == s1) {
                    return *this;
                }

                // Case: *s0' < *s1 - valid iterator unless exhausting left range
                if (Compare()(s0->first, s1->first)) {
                    if (Compare()(s0->first, initial_key)) s0 = s1;
                    return *this;
                }

                // Case: *s1 < *s0' - need to swap ranges, or deal with exhausting the left range
                if (Compare()(s1->first, s0->first)) {
                    if (Compare()(s0->first, initial_key)) s0 = s1;
                    std::swap(s0, s1);
                    return *this;
                }

                // Case: *s0' == *s1 (keys equal) - we're on a deleted item, loop again
                // This means s0->first == s1->first (not s0 == s1)
                // Continue the loop to advance once more
            }
        }

        const_iterator operator++(int) {
            iterator pre = *this;
            return ++*this, pre;
        }

        bool operator==(const const_iterator& other) const { return s0 == other.s0; }

        bool operator!=(const const_iterator& other) const { return s0 != other.s0; }

        reference operator*() { return *s0; }

        pointer operator->() { return &*s0; }

    private:
        // s0 points to the current element, which either may be in either range. s1  points
        // to the next (larger) element in the alternate range, or to a smaller one if that range is
        // exhausted. If s0 points into the left range it is permissible for s1 to be pointed at
        // the first element of the right. Cursor advancement will reposition s1 using a linear
        // scan.
        container_iterator s0, s1;

        friend square_map;
    };

    class iterator : public const_iterator {
    public:
        using value_type = square_map::value_type;
        using pointer = value_type*;
        using reference = value_type&;

        iterator& operator++() {
            const_iterator::operator++();
            return *this;
        }

        const_iterator operator++(int) {
            iterator pre = *this;
            return ++*this, pre;
        }

        reference operator*() { return *const_iterator::s0; }

        pointer operator->() { return &*const_iterator::s0; }

    private:
        friend square_map;
        static iterator make(container_iterator s0, container_iterator s1) {
            iterator ret;
            ret.s0 = s0;
            ret.s1 = s1;
            return ret;
        }
        static iterator make(const_iterator it) { return make(it.s0, it.s1); }
    };

    static_assert(sizeof(iterator) == 2 * sizeof(container_iterator));

    // Element Access
    T& at(const Key& key) {
        return const_cast<T&>(_cthis()->at(key));
    }
    const T& at(const Key& key) const {
        if (auto it = find(key); it != end()) return it->second;
        throw std::out_of_range("square_map<>: index out of range");
    }

    T& operator[](const Key& key) {
        auto it = find(key);
        if (it != end()) return it->second;
        // Insert default-constructed value
        auto result = insert({key, T()});
        return result.first->second;
    }

    // Iterators
    iterator begin() noexcept {
        auto it = cbegin();
        return iterator::make(it.s0, it.s1);
    }
    const_iterator begin() const noexcept {
        return cbegin();
    }
    const_iterator cbegin() const noexcept {
        if (empty())
            return cend();
        auto it = _vthis()->_container.begin();
        auto alt = it + _split;
        return Compare()(alt->first, it->first) ? iterator::make(alt, it) : iterator::make(it, alt);
    }

    iterator end() noexcept {
        auto it = cend();
        return iterator::make(it.s0, it.s1);
    }
    const_iterator end() const noexcept {
        return cend();
    }
    const_iterator cend() const noexcept {
        auto end = _vthis()->_container.end();
        return iterator::make(end, end);
    }

    iterator split_point() noexcept {
        if (_container.empty() || _split == 0 || _split >= _container.size()) return end();
        return find(_container[_split].first);
    }

    const_iterator split_point() const noexcept {
        auto it = _vthis()->_container.split_point();
        return iterator::make(it.s0, it.s1);
    }

    // Capacity
    bool empty() const noexcept {
        return !size();
    }

    size_type size() const noexcept { return _container.size() - 2 * _erased; }

    size_type max_size() const noexcept {
        return _container.max_size();
    }

    size_type capacity() const noexcept {
        return _container.capacity();
    }

    void reserve(size_type new_capacity) {
        _container.reserve(new_capacity);
    }

    void shrink_to_fit() {
        _container.shrink_to_fit();
    }

    // Modifiers
    void clear() noexcept {
        square_map fresh;
        swap(fresh);  // Avoids resetting individual fields with chances of forgetting one.
    }

    constexpr iterator erase(const_iterator pos) {
        // Precondition: pos is dereferenceable and belongs to *this.
        if (!_split) {
            // Only one range, just erase it. TODO: Deal with large unsplit container efficiency
            auto past_it = _container.erase(pos.s0);  // Points past the erased element
            return iterator::make(past_it, past_it);
        }
        // Invariant: split > 0 && size() >= 3, as there is at least one element in the left range,
        // one at the split point that is smaller, and one at the end that is larger.

        // Item is in the right range or the last element of the left range, so just erase it.
        if (_container.begin() + (_split - 1) <= pos.s0) {
            //  [ 1 2 4 | 3 5 7 ] <- _split = 3
            //        ^ pos.s0 = 2, erasing this merges both ranges: [ 1 2 3 | 5 7 ]
            //                                                              ^ returned
            //  [ 1 2 4 | 3 5 7 ] <- _split = 3
            //            ^ pos.s0 = 3, erasing this merges both ranges: [ 1 2 4 | 5 7 ]
            //                                                                      ^ returned

            // Points past the erased element, all other iterators are invalidated.
            auto past_it = _container.erase(pos.s0);
            // If we erased the last remaining item from the left range, meaning past_it points to
            // begin(), if past_it == _container.end() == _container.begin() + _split, or if past_it
            // points to an element whose key is strictly larger than the immediately preceding key,
            // we have effectively merged both ranges and will reset the split point to zero.

            bool merged = past_it == _container.begin() ||
                          (past_it == _container.end() && _split == _container.size()) ||
                          (past_it == _container.begin() + _split &&
                           Compare()((std::prev(past_it))->first, past_it->first));
            if (merged) _split = 0;
            // Still need to think about erased elements
            if (past_it == _container.end()) return end();
            return find(past_it->first);
        }

        // Item is strictly in the left range, so insert it into the right to mark it as erased.
        Key key = pos.s0->first;
        auto it = std::lower_bound(_container.begin() + _split, _container.end(), key,
                                   value_key_compare());

        ++_erased;
        auto&& next_key = std::next(pos)->first;
        _container.emplace(it, value_type{key, T()});  // Invalidates all iterators.
        return find(next_key);
    }

    container_type extract() && { return std::move(_container); }

    // Replaces the underlying container with the given one
    void replace(container_type&& cont) {
        _container = std::move(cont);
        _split = 0;   // Reset to flat state
        _erased = 0;  // Reset erased count
    }

    // Overload that takes an iterator to specify the split point
    void replace(container_type&& cont, typename container_type::iterator split_it) {
        auto split_index = std::distance(cont.begin(), split_it);
        _container = std::move(cont);
        _split = split_index == _container.size() ? 0 : split_index;
        _erased = 0;  // Reset erased count
    }

    // Merge split ranges into a single flat range
    void merge() {
        if (!_split) return;  // Already flat

        geert::merge_with_binary_search(_container.begin(), _container.begin() + _split,
                                        _container.end(), value_compare());
        _split = 0;  // Now flat

        if (!_erased) return;

        _container.erase(
            geert::remove_duplicates(_container.begin(), _container.end(), value_compare()),
            _container.end());
        _erased = 0;
    }

public:
    std::pair<iterator, bool> insert(value_type&& value) {
        auto begin = _container.begin(), end = _container.end();
        auto split = begin + _split;

        // Locate both left and right positions to check for erased items.
        auto left_it = std::lower_bound(begin, split, value.first, value_key_compare());
        auto right_it = std::lower_bound(split, end, value.first, value_key_compare());

        bool in_left = left_it != split && !Compare()(value.first, left_it->first);
        bool in_right = right_it != end && !Compare()(value.first, right_it->first);

        // If the item was found on the left, it will stay there in all cases.
        if (in_left) {
            // If the item is on both sides, it has been erased, so undo that erasure.
            if (in_right) --_erased, right_it = _container.erase(right_it);

            left_it->second = std::move(value.second);
            return {iterator::make(left_it, right_it), false};
        }

        // If just found in the right side, return that one.
        if (in_right) {
            right_it->second = std::move(value.second);
            return {iterator::make(right_it, left_it), false};
        }

        // Not found, insert a new item in the right range.

        // If the insertion point is not too far from the end of the container, just insert.
        auto move_distance = std::distance(right_it, end);
        auto right_size = std::distance(split, end);
        auto leftIndex = std::distance(begin, left_it);
        if (move_distance < kMinSplitSize || right_size * right_size * 4 < _split) {
            right_it = _container.insert(right_it, std::move(value));  // Invalidates all iterators.
            return {iterator::make(right_it, _container.begin() + left_index), true};
        }

        // Need to merge, as inserting in the right range would require moving too many elements.
        merge();

        // Still need to insert, but doing it in the proper place would require moving too many
        // elements. Instead, insert right before the end and create a split point.
        right_it = _container.insert(std::prev(_container.end()), std::move(value));
        _split = right_it - _container.begin();
        return {iterator::make(right_it, _container.begin() + left_index), true};
    }

    void swap(square_map& other) noexcept {
        std::swap(*this, other);
    }

    // Lookup
    size_type count(const Key& key) const {
        return find(key) != cend();  // Keys are unique.
    }
    iterator find(const Key& key) {
        return iterator::make(_cthis()->find(key));
    }
    const_iterator find(const Key& key) const {
        auto& c = _vthis()->_container;
        if (!_split) {
            // Only one range, do a single binary search. No erased items to check for.
            auto it = std::lower_bound(c.begin(), c.end(), key, value_key_compare());
            if (it == c.end() || Compare()(key, it->first)) return end();
            return iterator::make(it, std::prev(c.end()));  // s1 is the last element
        }
        auto split = c.begin() + _split;
        auto left_it = std::lower_bound(c.begin(), split, key, value_key_compare());
        auto right_it = std::lower_bound(split, c.end(), key, value_key_compare());
        bool in_left = left_it < split && !Compare()(key, left_it->first);
        bool in_right = right_it < c.end() && !Compare()(key, right_it->first);
        if (in_left == in_right) return end();  // Erased or non-existing item
        return in_left            ? iterator::make(left_it, right_it)
               : left_it != split ? iterator::make(right_it, left_it)
                                  : iterator::make(right_it, std::prev(c.end()));
    }

    // Test-only accessors
    size_type& test_erased_ref() { return _erased; }
    const size_type& test_erased_ref() const { return _erased; }

private:
    const square_map* _cthis() noexcept {
        return const_cast<const square_map*>(this);
    }
    square_map* _vthis() const noexcept {
        return const_cast<square_map*>(this);
    }
    container_type _container;
    size_type _split = 0;
    size_type _erased = 0;
};
}  // namespace geert