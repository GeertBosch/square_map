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
    static constexpr std::size_t kMinMergeSize = 128;
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
    using pointer = typename allocator_type::pointer;
    using const_pointer = typename allocator_type::const_pointer;

    struct value_key_compare {
        constexpr bool operator()(const value_type& left, const key_type& right) const {
            return Compare()(left.first, right);
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
            // If *_alt is larger than *_it, _alt has more elements to iterate over.
            auto& lastKey = _it++->first;
            do {
                if (!Compare()(lastKey, _alt->first)) {
                    if (_alt < _it)
                        return *this;  // _alt has no more elements, so no need to swap.

                    // *_alt must be in the right range. As the right range has the last element,
                    // _alt must not yet be positioned, which happens when a find hits in the right
                    // range. In this case, we can't do a binary search, but the right range has few
                    // elements, so a linear search is OK for these cases.
                    while (Compare()((++_alt)->first, lastKey))
                        ;
                }
                // Because we didn't exhaust _alt, _it is still dereferenceable after advancing.
                if ((Compare()(_alt->first, _it->first) || Compare()(_it->first, lastKey)))
                    std::swap(_it, _alt);
            } while (!Compare()(lastKey, _it->first));  // Skip equal (erased) items.
            return *this;  // All good keys still strictly increasing, no need to swap.
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
        // _it points to the current element, which either may be in either the range. _alt  points
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

    void merge_if_needed() {
        // Bound complexity by merging when the size of the right range is double the square
        // root of the size of the left range. Call before inserting into the right range.

        auto rightSize = _container.size() - _split;
        if (rightSize < kMinMergeSize || rightSize * rightSize < 4 * _split) return;
        merge_with_binary_search(_container.begin(), _container.begin() + _split, _container.end());
        // keep the largest key in the right range, iterators invalidated.
        _split = _container.size() - 1;
    }

    // Modifiers
    void clear() noexcept {
        square_map fresh;  // Avoids resetting individual fields with chances of forgetting one.
        swap(fresh);
    }

    constexpr iterator erase(const_iterator pos) {
        // Item is in the right range, so just erase it.
        if (!(pos._it < pos._alt)) return iterator::make(_container.erase(pos._it), pos._alt);

        // Item is in the left range, so add it to the right side as well to mark it as deleted.
        ++_erased;
        // merge_if_needed(); // Need to do, but invalidates pos. Skip for now.
        return iterator::make(_container.insert(++pos._alt, {pos._it->first, T()}), ++pos._it);
    }

    std::pair<iterator, bool> insert(value_type&& value) {
        merge_if_needed();

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
            if (inRight) --_erased, _container.erase(rightIt);

            leftIt->second = std::move(value.second);
            return {iterator::make(leftIt, rightIt), false};
        }

        // If just found in the right side, return that one.
        if (inRight) {
            rightIt->second = std::move(value.second);
            return {iterator::make(rightIt, leftIt), false};
        }

        // Not found, insert a new item in the right range.
        return {iterator::make(_container.insert(rightIt, std::move(value)), leftIt), true};
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
        auto& container = _vthis()->_container;
        auto split = container.begin() + _split;
        auto leftIt = std::lower_bound(container.begin(), split, key, value_key_compare());
        if (leftIt < split && !Compare()(key, leftIt->first)) return iterator::make(leftIt, split);
        auto rightIt = std::lower_bound(split, container.end(), key, value_key_compare());
        if (rightIt < container.end() && !Compare()(key, rightIt->first))
            return iterator::make(rightIt, leftIt);
        return end();
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