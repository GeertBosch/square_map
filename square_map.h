#pragma once
#include <algorithm>
#include <iterator>
#include <vector>

#include "merge_with_binary_search.h"

namespace geert {

constexpr std::size_t kMaxL1Size = 128;

template <class Key,
          class T,
          class Compare = std::less<Key>,
          class Container = std::vector<std::pair<Key, T>>>
class square_map {
public:
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

    struct value_key_compare_struct {
        constexpr bool operator()(const value_type& left, const key_type& right) const {
            return Compare()(left.first, right);
        }
    };
    using value_key_compare = value_key_compare_struct;

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

                    // *_alt must be in L1. As L1 has the last element, _alt must not yet be
                    // positioned, which happens when a find hits in L1. In this case, we can't do a
                    // binary search, but L1 has few elements, so a linear search is OK for these
                    // cases.
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
        // _it points to the current element, which either may be in either the L1 or the L2 range.
        // _alt  points to the next (larger) element in the alternate range, or to a smaller one if
        // that range is exhausted.
        // If _it points into L2, it is permissible for _alt to be pointed at the first element of
        // L1. Cursor advancement will reposition _alt using a linear scan.
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
        if (auto it = find(key); it != end())
            return it->second;
        throw std::out_of_range("level_map<>: index out of range");
    }

    T& operator[](const Key& key) {
        return insert({key, {}}).first->second;
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
        auto alt = it + _L2Size;
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

    size_type size() const noexcept {
        return _container.size() - _L2Erased;
    }

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
        // If pos points into L1 or at the last item of L2, directly remove the element.
        const size_type idx = std::distance(_container.begin(), pos._it);
        if (pos._it >= pos._alt || idx == --_L2Size)
            return iterator::make(_container.erase(pos._it), pos._alt);
        // For elements properly in L2, mark it as deleted, by replacement with its next larger
        // neighbor: copy the key, move the value and increment _L2Erased. Compact when needed.
        auto& neighbor = *(pos._it + 1);
        _L2Erased++;
        *pos._it = {neighbor.first, std::move(neighbor.second)};
        if (_L2Erased * _L2Erased <= _container.size())
            return iterator::make(_container.begin() + idx, pos._alt);

        // Maintain complexity bounds for operations after erasing sqrt(N) items.
        auto key = neighbor.first;  // Iterators will be invalidated.
        auto L2End = _container.begin() + _L2Size;
        L2End = std::unique(_container.begin(), L2End);
        _container.erase(L2End, _container.begin() + _L2Size);
        _L2Size = _L2Erased = 0;
        return find(key);
    }

    std::pair<iterator, bool> insert(value_type&& value) {
        // Bound complexity by merging when the L1 size is double the square root of the L2 size.
        if (auto L1Size = _container.size() - _L2Size;
            L1Size >= kMaxL1Size && L1Size * L1Size >= 4 * _L2Size) {
            merge_with_binary_search(_container.begin(), _container.begin() + _L2Size,
                                     _container.end());
            _L2Size = _container.size() - 1;  // keep the largest key in L1, iterators invalidated.
        }
        auto L1Begin = _container.begin() + _L2Size, L1End = _container.end();
        auto L1It = std::lower_bound(L1Begin, L1End, value.first, value_key_compare());
        auto L2It = std::lower_bound(_container.begin(), L1Begin, value.first, value_key_compare());
        if (L2It != L1Begin && !value_key_compare()(value, L2It->first))
            return {iterator::make(L2It, L1It), false};  // found an exact match in L2
        if (L1It != L1End && !value_key_compare()(value, L1It->first))
            return {iterator::make(L1It, L2It), false};  // found an exact match in L1
        return {iterator::make(_container.insert(L1It, std::move(value)), L2It), true};
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
        auto L1Begin = container.begin() + _L2Size;
        auto it2 = std::lower_bound(container.begin(), L1Begin, key, value_key_compare());
        if (it2 < L1Begin && !Compare()(key, it2->first))
            return iterator::make(it2, L1Begin);
        auto it1 = std::lower_bound(L1Begin, container.end(), key, value_key_compare());
        if (it1 < container.end() && !Compare()(key, it1->first))
            return iterator::make(it1, it2);
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
    size_type _L2Size = 0;
    size_type _L2Erased = 0;
};
}  // namespace geert