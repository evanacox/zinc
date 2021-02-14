//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox                                                   //
//                                                                           //
// Licensed under the Apache License, Version 2.0 (the "License");           //
// you may not use this file except in compliance with the License.          //
// You may obtain a copy of the License at                                   //
//                                                                           //
//    http://www.apache.org/licenses/LICENSE-2.0                             //
//                                                                           //
// Unless required by applicable law or agreed to in writing, software       //
// distributed under the License is distributed on an "AS IS" BASIS,         //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  //
// See the License for the specific language governing permissions and       //
// limitations under the License.                                            //
//                                                                           //
//======---------------------------------------------------------------======//

#ifndef ZINC_DATA_DETAIL_RAW_HASH_SET
#define ZINC_DATA_DETAIL_RAW_HASH_SET

#include "zinc/data/detail/set_traits.h"
#include "zinc/io/console.h"
#include "zinc/types/allocators.h"
#include "zinc/types/functors.h"
#include "zinc/types/iterators.h"
#include "zinc/types/pointers.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <ranges>
#include <span>
#include <utility>

namespace zinc::detail
{
    namespace ranges = std::ranges;

    enum class BucketState : unsigned char
    {
        empty,
        full,
        tombstone,
    };

    /// Better default hash map than `std::unordered_map`. Implemented using
    /// a hash table (as the name implies), with linear probing. No
    /// separate chaining, no stability for values, but no indirection on
    /// accesses. `NodeHashMap` exists when stability is needed for both key
    /// and value, and when only value stability is needed, something like
    /// a `HashMap<K, std::unique_ptr<V>>` works just fine.
    template <SetTraits Traits> class RawHashSet
    {
        using Stored = SetSlot<Traits>;
        using RealAlloc = AllocRebind<SetAlloc<Traits>, std::byte>;

    public:
        template <typename Value> struct RawHashSetIterator;

        using key_type = SetKey<Traits>;
        using value_type = SetValue<Traits>;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using hasher = SetHasher<Traits>;
        using key_equal = SetKeyEq<Traits>;
        using allocator_type = SetAlloc<Traits>;
        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = AllocPointer<allocator_type>;
        using const_pointer = AllocConstPointer<allocator_type>;
        using iterator = RawHashSetIterator<value_type>;
        using const_iterator = RawHashSetIterator<const value_type>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        RawHashSet() = default;

        RawHashSet(const RawHashSet& other)
            : hash_(other.hash_)
            , eq_(other.eq_)
            , alloc_(AllocTraits<RealAlloc>::select_on_container_copy_construction(other.alloc_ref()))
        {
            initialize_allocation(other.bucket_count());

            // meta_table is technically uninitialized, so needs uninitialized_copy
            // instead of just copy
            ranges::uninitialized_copy(other.meta_array(), meta_array());

            for_each_full_slot([&](size_type index) {
                // traits may need to do extra work, e.g with NodeHashSet
                Traits::copy_to(alloc_ref(), other.value_ptr_to(index), value_ptr_to(index));
            });
        }

        RawHashSet(RawHashSet&& other) noexcept
            : RawHashSet()
        {
            swap(*this, other);
        }

        explicit RawHashSet(size_type initial_capacity, allocator_type alloc = {})
            : alloc_(alloc)
        {
            initialize_allocation(initial_capacity);
            initialize_meta();
        }

        RawHashSet& operator=(RawHashSet other)
        {
            swap(*this, other);

            return *this;
        }

        ~RawHashSet() { destroy_allocation(); }

        /// Attempts to find an element that has a key equal to `key`, returns
        /// `end()` if none is found, else returns an iterator.
        ///
        /// # Parameters
        /// - `key`: The key to look for
        ///
        /// # Returns
        /// `end()` if the key was not found, an iterator otherwise
        template <EqComparable<const key_type&, key_equal> K> [[nodiscard]] iterator find(const K& key)
        {
            const auto hash_value = hash_key(key);
            const auto [probed_index, state] = probe_for<K, false>(key, hash_value);

            if (state == BucketState::empty)
                return end();
            else
                return iterator_to(probed_index);
        }

        /// Attempts to find an element that has a key equal to `key`, returns
        /// `end()` if none is found, else returns an iterator.
        ///
        /// # Parameters
        /// - `key`: The key to look for
        ///
        /// # Returns
        /// `end()` if the key was not found, an iterator otherwise
        [[nodiscard]] iterator find(const key_type& key) { return find<key_type>(key); }

        /// Attempts to find an element that has a key equal to `key`, returns
        /// `end()` if none is found, else returns an iterator.
        ///
        /// # Parameters
        /// - `key`: The key to look for
        ///
        /// # Returns
        /// `end()` if the key was not found, an iterator otherwise
        template <EqComparable<const key_type&, key_equal> K> const_iterator find(const K& key) const
        {
            const auto hash_value = hash_key(key);
            const auto [probed_index, state] = probe_for<K, false>(key, hash_value);

            if (state == BucketState::empty)
                return end();
            else
                return iterator_to(probed_index);
        }

        /// Attempts to find an element that has a key equal to `key`, returns
        /// `end()` if none is found, else returns an iterator.
        ///
        /// # Parameters
        /// - `key`: The key to look for
        ///
        /// # Returns
        /// `end()` if the key was not found, an iterator otherwise
        [[nodiscard]] const_iterator find(const key_type& key) const { return find<key_type>(key); }

        /// Gets the number of elements in the set that are equal to `key`,
        /// either `1` or `0` because duplicate elements are not allowed.
        ///
        /// # Parameters
        /// - `key`: The key to look for
        ///
        /// # Returns
        /// `1` or `0` depending on if the element was found or not
        template <EqComparable<const key_type&, key_equal> K> [[nodiscard]] size_type count(const K& key) const
        {
            return find(key) == end() ? 0 : 1;
        }

        /// Gets the number of elements in the set that are equal to `key`,
        /// either `1` or `0` because duplicate elements are not allowed.
        ///
        /// # Parameters
        /// - `key`: The key to look for
        ///
        /// # Returns
        /// `1` or `0` depending on if the element was found or not
        [[nodiscard]] size_type count(const key_type& key) const { return find(key) == end() ? 0 : 1; }

        /// Checks the table for an element matching `key`, returning `true` if one
        /// is found and `false` otherwise
        ///
        /// # Parameters
        /// - `key`: The key to look for
        ///
        /// # Returns
        /// `true` or `false` depending on whether or not the key was found in the set
        [[nodiscard]] bool contains(const EqComparable<const key_type&, key_equal> auto& key) const
        {
            return find(key) != end();
        }

        /// Checks the table for an element matching `key`, returning `true` if one
        /// is found and `false` otherwise
        ///
        /// # Parameters
        /// - `key`: The key to look for
        ///
        /// # Returns
        /// `true` or `false` depending on whether or not the key was found in the set
        [[nodiscard]] bool contains(const key_type& key) const { return find(key) != end(); }

        /// Inserts a value if a value with the same key does not already exist in the
        /// table, and returns a reference to that value.
        ///
        /// # Parameters
        /// - `value`: The value to insert into the set
        ///
        /// # Effects
        /// - If `load_factor() > bucket_count() * max_load_factor()`, set is re-hashed.
        ///
        /// # Returns
        /// Returns a reference to the inserted value
        std::pair<bool, iterator> insert(std::convertible_to<value_type> auto&& value)
        {
            rehash_if_required();

            const auto& key = Traits::key_from(value);
            const auto hash_value = hash_key(key);
            const auto [index, state] = probe_for<decltype(key), true>(key, hash_value);

            // tombstones can get re-used here, no point to leaving them as tombstones
            // when they'll just add to load_factor & get probed over anyway
            if (state != BucketState::full)
            {
                construct_at(index, std::forward<decltype(value)>(value));
                update_meta(index, BucketState::full);

                if (state == BucketState::empty)
                {
                    ++size_;
                }

                return {true, iterator_to(index)};
            }

            return {false, iterator_to(index)};
        }

        /// Gets the max load factor that the set will get to before it hashes itself
        [[nodiscard]] constexpr float max_load_factor() const noexcept { return Traits::max_load_factor(); }

        /// Gets the current load factor, equivalent to `static_cast<float>(table.size() / table.bucket_count());`
        /// UNLESS `table.bucket_count() == 0`, in which case it returns `0.f`.
        [[nodiscard]] float load_factor() const noexcept
        {
            return bucket_count() == 0 ? 0.f : static_cast<float>(size()) / static_cast<float>(bucket_count());
        }

        /// Gets the number of buckets in the set, aka the max number of
        /// elements it can hold with no collisions
        [[nodiscard]] size_type bucket_count() const noexcept { return capacity_; }

        /// Gets the current number of elements in the set.
        [[nodiscard]] size_type size() const noexcept { return size_; }

        /// Checks if the set is completely empty, equivalent to `set.size() == 0`
        [[nodiscard]] bool empty() const noexcept { return size() == 0; }

        /// If `new_size` is over `bucket_count()`, rehashes to have that many buckets. All elements
        /// are moved, and all iterators/pointers/references are invalidated.
        ///
        /// # Parameters
        /// - `new_size`: The new capacity to rehash to
        void rehash(size_type new_size)
        {
            if (new_size > bucket_count())
            {
                auto alloc = new_allocation(new_size);
                transfer_to_new_allocation(alloc);
                swap_allocation(alloc);
            }
        }

        /// Gets an iterator to the first value of the table
        [[nodiscard]] iterator begin() noexcept
        {
            const auto first_full = ranges::find(meta_array(), BucketState::full);
            const auto index = std::distance(meta_array().begin(), first_full);

            return iterator_to(index);
        }

        /// Gets a past-the-end iterator on the table
        [[nodiscard]] iterator end() noexcept
        {
            const auto meta = meta_array();
            const auto end_index = std::distance(meta.begin(), meta.end());

            return iterator_to(end_index);
        }

        [[nodiscard]] const_iterator begin() const noexcept
        {
            const auto first_full = ranges::find(meta_array(), BucketState::full);
            const auto index = std::distance(meta_array().begin(), first_full);

            return iterator_to(index);
        }

        [[nodiscard]] const_iterator end() const noexcept
        {
            const auto meta = meta_array();
            const auto end_index = std::distance(meta.begin(), meta.end());

            return iterator_to(end_index);
        }

        [[nodiscard]] const_iterator cbegin() const noexcept { return begin(); }

        [[nodiscard]] const_iterator cend() const noexcept { return end(); }

        [[nodiscard]] reverse_iterator rbegin() noexcept { return std::make_reverse_iterator(begin()); }

        [[nodiscard]] reverse_iterator rend() noexcept { return std::make_reverse_iterator(end()); }

        [[nodiscard]] const_reverse_iterator crbegin() const noexcept { return std::make_reverse_iterator(begin()); }

        [[nodiscard]] const_reverse_iterator crend() const noexcept { return std::make_reverse_iterator(end()); }

        /// Checks if two set are equal.
        ///
        /// A set is considered "equal" when the following conditions hold:
        /// - `a.size() == b.size()`
        /// - Every element in `a` has an element in `b` with the same key and same value
        [[nodiscard]] bool operator==(const RawHashSet& other) const noexcept
        {
            if (this == &other) return true;

            if (other.size() != size()) return false;

            // TODO

            return true;
        }

        /// Returns the allocator in use by the container
        [[nodiscard]] allocator_type get_allocator() const noexcept { return allocator_type(alloc_ref()); }

        /// Returns the allocator in use by the container
        [[nodiscard]] hasher hash_function() const noexcept { return hasher(hash_ref()); }

        /// Returns the allocator in use by the container
        [[nodiscard]] key_equal key_eq() const noexcept { return key_equal(eq_ref()); }

        /// Swaps a set with another set, ADL-compatible and self-swap safe
        ///
        /// # Parameters
        /// - `lhs`: The first set
        /// - `rhs`: The set to swap with `lhs`
        friend void swap(RawHashSet& lhs, RawHashSet& rhs) noexcept
        {
            using std::swap;

            if constexpr (AllocTraits<RealAlloc>::propagate_on_container_swap)
            {
                swap(lhs.alloc_, rhs.alloc_);
            }

            swap(lhs.hash_, rhs.hash_);
            swap(lhs.eq_, rhs.eq_);
            swap(lhs.values_, rhs.values_);
            swap(lhs.capacity_, rhs.capacity_);
            swap(lhs.size_, rhs.size_);
        }

        template <typename Value> struct RawHashSetIterator
        {
            using MaybeConstBuffer = std::conditional_t<std::is_const_v<Value>, const std::byte*, std::byte*>;
            using MaybeConstStored = std::conditional_t<std::is_const_v<Value>, const Stored, Stored>;
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = Value;
            using reference = Value&;
            using size_type = RawHashSet::size_type;

            RawHashSetIterator() = default;

            RawHashSetIterator(MaybeConstBuffer buffer, size_type capacity, size_type index = 0)
                : buffer_(buffer)
                , capacity_(capacity)
                , index_(index)
            {}

            RawHashSetIterator(const RawHashSetIterator&) noexcept = default;

            RawHashSetIterator(RawHashSetIterator&&) noexcept = default;

            RawHashSetIterator& operator=(const RawHashSetIterator&) noexcept = default;

            RawHashSetIterator& operator=(RawHashSetIterator&&) noexcept = default;

            ~RawHashSetIterator() noexcept = default;

            reference operator*() const noexcept
            {
                assert("not past the end" && !is_at_end());
                assert("has a value" && is_current_full());

                return Traits::value_from(pointer_cast<MaybeConstStored*>(buffer_)[index_]);
            }

            RawHashSetIterator& operator++() noexcept
            {
                find_next_full();

                return *this;
            }

            RawHashSetIterator operator++(int) noexcept
            {
                const auto copy = *this;

                find_next_full();

                return copy;
            }

            RawHashSetIterator& operator--() noexcept
            {
                find_next_full_backwards();

                return *this;
            }

            RawHashSetIterator operator--(int) noexcept
            {
                const auto copy = *this;

                find_next_full_backwards();

                return copy;
            }

            [[nodiscard]] bool operator==(RawHashSetIterator other) noexcept
            {
                return other.buffer_ == buffer_ && other.capacity_ == capacity_ && other.index_ == index_;
            }

        protected:
            [[nodiscard]] bool is_at_end() const noexcept { return capacity_ <= index_; }

            [[nodiscard]] BucketState current_value() const noexcept
            {
                return pointer_cast<const BucketState*>(pointer_cast<const Stored*>(buffer_) + capacity_)[index_];
            }

            [[nodiscard]] bool is_current_full() const noexcept { return current_value() == BucketState::full; }

            void find_next_full()
            {
                ++index_; // get past the current element, prevents an infinite loop

                while (!is_at_end() && !is_current_full())
                {
                    ++index_;
                }
            }

            void find_next_full_backwards()
            {
                --index_; // get past current element, prevents an infinite loop

                while (!is_at_end() && !is_current_full())
                {
                    --index_;
                };
            }

        private:
            MaybeConstBuffer buffer_ = nullptr;
            size_type capacity_ = 0;
            size_type index_ = 0;
        };

    protected:
        // compares `size + 1` to `bucket_count() * max_load_factor()`
        bool should_resize() noexcept { return size() + 1 > bucket_count() * max_load_factor(); }

        // checks if a bucket is both full and has a key equivalent to `key`
        bool full_and_key_eq(const key_type& key, BucketState state, size_type index) const
        {
            if (state == BucketState::full)
            {
                return eq_ref()(Traits::key_from(values_at(index)), key);
            }

            return false;
        }

        // implements the actual linear probing logic for the hash table
        // `ReturnTombstones` is for methods that do/dont want to get tombstone
        // slots returned, as insertion might want it while find might not (and the
        // choice should be made at compile time)
        template <EqComparable<key_type, key_equal> K, bool ReturnTombstones>
        std::pair<size_type, BucketState> probe_for(const K& key, size_type index) const
        {
            while (true)
            {
                const auto meta = meta_at(index);

                // empty slots can be used, but if the key is found we return this too.
                // probing happens for both insertion and finding
                if (meta == BucketState::empty || full_and_key_eq(key, meta, index))
                {
                    return {index, meta};
                }

                // if the caller so chooses, it can return tombstone values
                // as well, useful for insertion
                if constexpr (ReturnTombstones)
                {
                    if (meta == BucketState::tombstone)
                    {
                        return {index, meta};
                    }
                }

                index = (index + 1) % bucket_count();
            }
        }

        // calls a lambda for every full slot
        void for_each_full_slot(std::invocable<size_type> auto lambda)
        {
            for (auto i : zinc::range_of(size_type{0}, bucket_count()))
            {
                if (is_full_at(i))
                {
                    lambda(i);
                }
            }
        }

        // calls destructors on every full slot
        void clear_slots()
        {
            for_each_full_slot([this](size_type slot_index) {
                // in theory the types could be trivial, but because the allocator might have non-dtor side
                // effects, we need to actually call `AllocTraits::destroy`. If it doesn't, any good optimizer
                // will remove it anyway (as by default it's `std::destroy_at`, aka `ptr->~T();`
                AllocTraits<RealAlloc>::destroy(alloc_ref(), value_ptr_to(slot_index));
            });

            size_ = 0;
        }

        // destroys each value before deleting the entire allocation
        void destroy_allocation()
        {
            clear_slots();

            AllocTraits<RealAlloc>::deallocate(alloc_ref(), underlying_storage(), byte_size());
        }

        // checks if the table needs to be rehashed, and rehashes if it does
        void rehash_if_required()
        {
            if (should_resize())
            {
                rehash(capacity_ * 2);
            }
        }

        using Allocation = std::pair<Stored*, size_type>;

        void initialize_allocation(size_type initial_capacity) { swap_allocation(new_allocation(initial_capacity)); }

        // destroys the current storage and expands it to a new size, with enough
        // room for `new_capacity` `Stored` objects and `BucketState` objects.
        // Updates bucket_count but not size, size needs to be updated by caller.
        //
        // does not destruct any elements in the storage, must be done before calling
        Allocation new_allocation(size_type new_capacity)
        {
            const auto size = byte_size(new_capacity);
            const auto ptr = pointer_cast<Stored*>(AllocTraits<RealAlloc>::allocate(alloc_ref(), size));
            initialize_meta(pointer_cast<BucketState*>(ptr + new_capacity), new_capacity);

            return {ptr, new_capacity};
        }

        // uses Traits::transfer_to or Traits::copy_to to move each element to the a new allocation,
        // rehashing each element. `new_alloc` is a pair holding the number of slots
        // in the new allocation and a pointer to the beginning, must be a pointer to
        // the first element. Pointer is assumed to be from `new_allocation` and point to
        // data with the same layout as is expected for this class
        template <bool Copy> void setup_new_allocation(Allocation new_alloc)
        {
            const auto meta = meta_from(new_alloc);

            for_each_full_slot([&](size_type index) {
                const auto& [alloc, capacity] = new_alloc;
                const auto& key = Traits::key_from(values_at(index));
                const auto hash = hash_key_with_count(key, capacity);

                if constexpr (Copy)
                    Traits::copy_to(alloc_ref(), value_ptr_to(index), alloc + hash);
                else
                    Traits::transfer_to(alloc_ref(), value_ptr_to(index), alloc + hash);

                meta[hash] = BucketState::full;
            });
        }

        void copy_to_new_allocation(Allocation alloc) { setup_new_allocation<true>(alloc); }

        void transfer_to_new_allocation(Allocation alloc) { setup_new_allocation<false>(alloc); }

        // deallocates the current allocation and replaces it with a new one.
        // properly calls destructors on the current allocation
        void swap_allocation(Allocation new_alloc)
        {
            const auto& [alloc, capacity] = new_alloc;

            if (values()) [[likely]]
            {
                for_each_full_slot([&](size_type index) {
                    // traits may need to handle calling extra stuff that we dont know about
                    Traits::destroy(alloc_ref(), value_ptr_to(index));
                });

                AllocTraits<RealAlloc>::deallocate(alloc_ref(), underlying_storage(), byte_size());
            }

            values_ = alloc;
            meta_ = pointer_cast<BucketState*>(values() + capacity);
            capacity_ = capacity;
        }

        // initializes the meta table that `this` holds
        void initialize_meta() noexcept { initialize_meta(meta(), bucket_count()); }

        // initializes a meta table to all `BucketState::empty` correctly. Assumes
        // lifetime of each element hasn't started yet and constructs at, instead
        // of re-assigning.
        void initialize_meta(BucketState* ptr, size_type capacity)
        {
            const auto end = ptr + capacity;

            for (auto* it = ptr; it != end; ++it)
            {
                AllocTraits<RealAlloc>::construct(alloc_ref(), it, BucketState::empty);
            }
        }

        // Correctly hashes a key with the current capacity
        [[nodiscard]] size_type hash_key(const key_type& key) const noexcept
        {
            return hash_key_with_count(key, bucket_count());
        }

        // Correctly hashes a key with N as the bucket count
        [[nodiscard]] size_type hash_key_with_count(const key_type& key, size_type capacity) const noexcept
        {
            return hash_ref()(key) % capacity;
        }

        // Returns the byte-wise size of the current allocation
        [[nodiscard]] size_type byte_size() const noexcept { return byte_size(bucket_count()); }

        // Returns the full byte-wise size of the allocated space, assuming a bucket_count of
        // `bucket_count` values and the allocation containing both a `Stored[]` and `BucketState[]`
        // with both arrays being exactly `bucket_count` long.
        [[nodiscard]] constexpr static size_type byte_size(size_type capacity) noexcept
        {
            return (capacity * sizeof(Stored)) + (capacity * sizeof(BucketState));
        }

        // gets an iterator starting at a specific index
        [[nodiscard]] iterator iterator_to(size_type index) noexcept
        {
            return iterator{underlying_storage(), bucket_count(), index};
        }

        // gets a const iterator starting at a specific index
        [[nodiscard]] const_iterator iterator_to(size_type index) const noexcept
        {
            return const_iterator{underlying_storage(), bucket_count(), index};
        }

        // Gets a mutable pointer to the value section of the allocation
        [[nodiscard]] Stored* values() noexcept { return values_; }

        // Gets a pointer to the value section of the allocation
        [[nodiscard]] const Stored* values() const noexcept { return values_; }

        [[nodiscard]] Stored* value_ptr_to(size_type index) noexcept
        {
            assert("not attempting to access empty table" && underlying_storage());
            assert("attempting to access index beyond bucket_count" && index < bucket_count());

            return values() + index;
        }

        // Gets a pointer to a possibly uninitialized `Stored` object
        [[nodiscard]] const Stored* value_ptr_to(size_type index) const noexcept
        {
            assert("not attempting to access empty table" && underlying_storage());
            assert("attempting to access index beyond bucket_count" && index < bucket_count());

            return values() + index;
        }

        // Gets a reference to a definitely initialized `Stored` object
        [[nodiscard]] Stored& values_at(size_type index) noexcept
        {
            assert("attempting to access uninitialized value" && is_full_at(index));

            return *value_ptr_to(index);
        }

        // Gets a reference to a definitely initialized `Stored` object
        [[nodiscard]] const Stored& values_at(size_type index) const noexcept
        {
            assert("attempting to access uninitialized value" && is_full_at(index));

            return *value_ptr_to(index);
        }

        // Gets a pointer to the meta table section of the allocation
        [[nodiscard]] BucketState* meta() noexcept { return meta_; }

        // Gets a pointer to the meta table section of the allocation
        [[nodiscard]] const BucketState* meta() const noexcept { return meta_; }

        // Gets a span over the entire meta array, usable for ranged-for and friends
        [[nodiscard]] std::span<BucketState> meta_array() noexcept { return meta_array_at(0); }

        // Gets a span over the entire meta array, usable for ranged-for and friends
        [[nodiscard]] std::span<const BucketState> meta_array() const noexcept
        {
            return std::span{meta(), meta() + bucket_count()};
        }

        // Gets the meta value at a specific index
        [[nodiscard]] BucketState meta_at(size_type index) const noexcept
        {
            assert("not attempting to access empty table" && underlying_storage());
            assert("attempting to access index beyond bucket_count" && index < bucket_count());

            return meta()[index];
        }

        // Updates the meta table at an index and returns a reference to that new meta entry
        BucketState& update_meta(size_type index, BucketState new_state) noexcept
        {
            assert("not attempting to access empty table" && underlying_storage());
            assert("attempting to access index beyond bucket_count" && index < bucket_count());

            return meta()[index] = new_state;
        }

        // Gets a span over a subsection of the meta array, from `index` to the end.
        [[nodiscard]] std::span<BucketState> meta_array_at(size_type index) noexcept
        {
            return std::span{meta() + index, meta() + bucket_count()};
        }

        [[nodiscard]] BucketState* meta_from(Allocation alloc)
        {
            return pointer_cast<BucketState*>(alloc.first + alloc.second);
        }

        // in a few specific cases, raw access to the underlying allocation is needed. Typically
        // though only `values()` and `meta()` are needed, since everything about the allocation
        // *is* known by the class.
        [[nodiscard]] std::byte* underlying_storage() noexcept { return pointer_cast<std::byte*>(values()); }

        [[nodiscard]] const std::byte* underlying_storage() const noexcept
        {
            return pointer_cast<const std::byte*>(values());
        }

        void construct_at(size_type index, auto&&... args) noexcept
            requires std::constructible_from<value_type, decltype(args)...>
        {
            Traits::construct(alloc_ref(), value_ptr_to(index), std::forward<decltype(args)>(args)...);
        }

        [[nodiscard]] bool is_full_at(size_type slot) const noexcept { return meta_at(slot) == BucketState::full; }

        [[nodiscard]] bool is_empty_at(size_type slot) const noexcept { return meta_at(slot) == BucketState::empty; }

        [[nodiscard]] const hasher& hash_ref() const noexcept { return hash_; }

        [[nodiscard]] const key_equal& eq_ref() const noexcept { return eq_; }

        [[nodiscard]] RealAlloc& alloc_ref() noexcept { return alloc_; }

        [[nodiscard]] const RealAlloc& alloc_ref() const noexcept { return alloc_; }

    private:
        // Meta and data tables are all in one allocation, both tables being the same length,
        // at all times. meta_table()[i] will always be the state for whatever value lies at values()[i].
        //
        // [ VALUE, VALUE, VALUE, VALUE, meta, meta, meta, meta ]
        //   ^                           ^
        //   values_                     meta_
        Stored* values_ = nullptr;
        BucketState* meta_ = nullptr;
        size_type capacity_ = 0;
        size_type size_ = 0;
        [[no_unique_address]] hasher hash_;
        [[no_unique_address]] key_equal eq_;
        [[no_unique_address]] RealAlloc alloc_;
    };
} // namespace zinc::detail

#endif