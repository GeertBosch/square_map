#pragma once
#include <algorithm>
#include <iterator>
#include <vector>

#include "merge_with_binary_search.h"

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

    class const_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = const square_map::value_type;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;

        const_iterator& operator++() {
            // Precondition: _it and _alt are dereferenceable.

            auto initial_key = _it->first;

            while (true) {
                // Case: s_0 = s_1 - we're at the last element, advance to end
                if (_it == _alt) {
                    ++_it;       // This makes _it point to end()
                    _alt = _it;  // Set _alt to end() as well to match cend() behavior
                    return *this;
                }

                // Advance s_0 to get s_0'
                ++_it;
                auto next_key = _it->first;

                // Case: s_0' = s_1 - we're at the last element after advancement
                if (_it == _alt) {
                    return *this;
                }

                // Case: *s_0' < *s_1 - valid iterator unless exhausting left range
                if (Compare()(_it->first, _alt->first)) {
                    if (Compare()(_it->first, initial_key)) _it = _alt;
                    return *this;
                }

                // Case: *s_1 < *s_0' - need to swap ranges, or deal with exhausting the left range
                if (Compare()(_alt->first, _it->first)) {
                    if (Compare()(_it->first, initial_key)) _it = _alt;
                    std::swap(_it, _alt);
                    return *this;
                }

                // Case: *s_0' = *s_1 (keys equal) - we're on a deleted item, loop again
                // This means _it->first == _alt->first (not _it == _alt)
                // Continue the loop to advance once more
            }
        }

        const_iterator operator++(int) {
            iterator pre = *this;
            return ++*this, pre;
        }

        bool operator==(const const_iterator& other) const {
            return _it == other._it;
        }

        bool operator!=(const const_iterator& other) const {
            return _it != other._it;
        }

        reference operator*() {
            return *_it;
        }

        pointer operator->() {
            return &*_it;
        }

    protected:
        // _it points to the current element, which either may be in either range. _alt  points
        // to the next (larger) element in the alternate range, or to a smaller one if that range is
        // exhausted. If _it points into the left range it is permissible for _alt to be pointed at
        // the first element of the right. Cursor advancement will reposition _alt using a linear
        // scan.
        container_iterator _it, _alt;

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

        reference operator*() {
            return *const_iterator::_it;
        }

        pointer operator->() {
            return &*const_iterator::_it;
        }

    private:
        friend square_map;
        static iterator make(container_iterator it, container_iterator alt) {
            iterator ret;
            ret._it = it;
            ret._alt = alt;
            return ret;
        }
        static iterator make(const_iterator it) {
            return make(it._it, it._alt);
        }
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
        return iterator::make(it._it, it._alt);
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
        return iterator::make(it._it, it._alt);
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
        return iterator::make(it._it, it._alt);
    }

    // Capacity
    bool empty() const noexcept {
        return !size();
    }

    size_type size() const noexcept { return _container.size() - _erased; }

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
        square_map fresh;  // Avoids resetting individual fields with chances of forgetting one.
        swap(fresh);
    }

    constexpr iterator erase(const_iterator pos) {
        // Precondition: pos is dereferenceable and belongs to *this.
        if (!_split) {
            // Only one range, just erase it. TODO: Deal with large unsplit container
            auto past_it = _container.erase(pos._it);  // Points past the erased element
            return iterator::make(past_it, past_it);
        }
        // Invariant: split > 0 && size() >= 3, as there is at least one element in the left range,
        // one at the split point that is smaller, and one at the end that is larger.

        // Item is in the right range or the last element of the left range, so just erase it.
        if (_container.begin() + (_split - 1) <= pos._it) {
            //  [ 1 2 4 | 3 5 7 ] <- _split = 3
            //        ^ pos._it = 2, erasing this merges both ranges: [ 1 2 3 | 5 7 ]
            //                                                              ^ returned
            //  [ 1 2 4 | 3 5 7 ] <- _split = 3
            //            ^ pos._it = 3, erasing this merges both ranges: [ 1 2 4 | 5 7 ]
            //                                                                      ^ returned

            // Points past the erased element, all other iterators are invalidated.
            auto past_it = _container.erase(pos._it);
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
        Key key = pos._it->first;
        auto it = std::lower_bound(_container.begin() + _split, _container.end(), key,
                                   value_key_compare());

        ++_erased;
        auto next_key = std::next(pos)->first;
        _container.emplace(it, value_type{key, T()});  // Invalidates all iterators.
        return find(next_key);
    }

    container_type extract() && { return std::move(_container); }

    // Create a square_map from a container and split point (opposite of extract)
    static square_map inject(container_type&& container, size_type split_index = 0) {
        square_map result;
        result._container = std::move(container);
        if (split_index >= result._container.size()) {
            result._split = 0;  // Treat as flat map
        } else {
            result._split = split_index;
        }
        result._erased = 0;
        return result;
    }

    // Overload that takes a const reference to the container
    static square_map inject(const container_type& container, size_type split_index = 0) {
        square_map result;
        result._container = container;
        if (split_index >= result._container.size()) {
            result._split = 0;  // Treat as flat map
        } else {
            result._split = split_index;
        }
        result._erased = 0;
        return result;
    }

    // Overload that takes an iterator to specify the split point
    static square_map inject(container_type&& container,
                             typename container_type::iterator split_it) {
        size_type split_index =
            split_it == container.end() ? 0 : std::distance(container.begin(), split_it);
        return inject(std::move(container), split_index);
    }

    // Overload that takes const container and iterator
    static square_map inject(const container_type& container,
                             typename container_type::const_iterator split_it) {
        size_type split_index =
            split_it == container.end() ? 0 : std::distance(container.begin(), split_it);
        return inject(container, split_index);
    }

    // Merge split ranges into a single flat range
    void merge() {
        if (_split == 0) return;  // Already flat

        merge_with_binary_search(
            _container.begin(), _container.begin() + _split, _container.end(),
            [](const value_type& a, const value_type& b) { return Compare()(a.first, b.first); });
        _split = 0;  // Now flat

        remove_duplicates();
    }

private:
    void remove_duplicates() {
        if (!_erased) return;

        auto it = _container.begin();

        // Find first duplicate pair of elements. There is at least such pair, as there is at least
        // one non-erased element (the last one), so no need to check for end() here.
        while (Compare()((it)->first, (it + 1)->first)) ++it;

        // First duplicate elements found: switch to compaction mode
        auto write_it = it;
        do {
            // We're at a duplicate element. Skip all consecutive duplicates.
            do {
                it += 2;
                --_erased;
            } while (!Compare()(it->first, (it + 1)->first));

            // We're at a unique element. Copy all unique elements until the next duplicate.
            do {
                *write_it++ = std::move(*it++);
            } while (Compare()(it->first, (it + 1)->first));
        } while (_erased);

        // All remaining elements are unique, copy them.
        write_it = std::move(it, _container.end(), write_it);

        // Erase the unused elements at the end
        _container.erase(write_it, _container.end());
    }

public:
    std::pair<iterator, bool> insert(value_type&& value) {
        auto begin = _container.begin(), end = _container.end();
        auto split = begin + _split;

        // Locate both left and right positions to check for erased items.
        auto leftIt = std::lower_bound(begin, split, value.first, value_key_compare());
        auto rightIt = std::lower_bound(split, end, value.first, value_key_compare());

        bool inLeft = leftIt != split && !Compare()(value.first, leftIt->first);
        bool inRight = rightIt != end && !Compare()(value.first, rightIt->first);

        // If the item was found on the left, it will stay there in all cases.
        if (inLeft) {
            // If the item is on both sides, it has been erased, so undo that erasure.
            if (inRight) --_erased, rightIt = _container.erase(rightIt);

            leftIt->second = std::move(value.second);
            return {iterator::make(leftIt, rightIt), false};
        }

        // If just found in the right side, return that one.
        if (inRight) {
            rightIt->second = std::move(value.second);
            return {iterator::make(rightIt, leftIt), false};
        }

        // Not found, insert a new item in the right range.

        // If the insertion point is not too far from the end of the container, just insert.
        auto move_distance = std::distance(rightIt, end);
        auto right_size = std::distance(split, end);
        auto leftIndex = std::distance(begin, leftIt);
        if (move_distance < kMinSplitSize || right_size * right_size * 4 < _split) {
            rightIt = _container.insert(rightIt, std::move(value));  // Invalidates all iterators.
            return {iterator::make(rightIt, _container.begin() + leftIndex), true};
        }

        // Need to merge, as inserting in the right range would require moving too many elements.
        if (_split) {
            // Use a comparator that only compares keys to ensure stability for equal keys
            auto key_compare = [](const value_type& a, const value_type& b) {
                return Compare()(a.first, b.first);
            };
            merge_with_binary_search(begin, split, end, key_compare);
        }

        // If there were erased items, they're now duplicates that must be removed.
        if (_erased) {
            // Use key-only comparator to identify duplicate keys
            auto key_equal = [](const value_type& a, const value_type& b) {
                return !Compare()(a.first, b.first) && !Compare()(b.first, a.first);
            };
            _container.erase(std::unique(_container.begin(), _container.end(), key_equal), _container.end());
        }
        _erased = 0;

        // Still need to insert, but doing it in the proper place would require moving too many
        // elements. Instead, insert right before the end and create a split point.
        rightIt = _container.insert(std::prev(_container.end()), std::move(value));
        _split = rightIt - _container.begin();
        return {iterator::make(rightIt, _container.begin() + leftIndex), true};
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
            return iterator::make(it, std::prev(c.end()));  // _alt is the last element
        }
        auto split = c.begin() + _split;
        auto leftIt = std::lower_bound(c.begin(), split, key, value_key_compare());
        auto rightIt = std::lower_bound(split, c.end(), key, value_key_compare());
        bool isLeft = leftIt < split && !Compare()(key, leftIt->first);
        bool isRight = rightIt < c.end() && !Compare()(key, rightIt->first);
        if (isLeft == isRight) return end();  // Erased or non-existing item
        return isLeft            ? iterator::make(leftIt, rightIt)
               : leftIt != split ? iterator::make(rightIt, leftIt)
                                 : iterator::make(rightIt, std::prev(c.end()));
    }

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