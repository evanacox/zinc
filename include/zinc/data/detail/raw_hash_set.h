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

    template <typename Value, typename Table> struct RawHashSetIterator;

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
        using iterator = RawHashSetIterator<value_type, RawHashSet>;
        using const_iterator = RawHashSetIterator<const value_type, RawHashSet>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        RawHashSet() = default;

        RawHashSet(const RawHashSet& other)
            : hash_(other.hash_)
            , alloc_(AllocTraits<RealAlloc>::select_on_container_copy_construction(other.alloc_))
            , eq_(other.eq_)
        {
            expand_allocation_to(other.bucket_count());

            // meta_table is technically uninitialized, so needs uninitialized_copy
            // instead of just copy
            ranges::uninitialized_copy(other.meta_array(), meta_array());

            for_each_full_slot([&](std::size_t index) {
                Traits::copy_to(alloc_ref(), other.values_ptr_to(index), value_ptr_to(index));
            });
        }

        RawHashSet(RawHashSet&& other) noexcept
            : values_(std::exchange(other.values_, nullptr))
            , capacity_(std::exchange(other.capacity_, 0))
            , size_(std::exchange(other.size_, 0))
            , hash_(std::move(other.hash_))
            , alloc_(std::move(other.alloc_))
            , eq_(std::move(other.eq_))
        {}

        explicit RawHashSet(size_type initial_capacity, allocator_type alloc = {})
            : alloc_(alloc)
        {
            expand_allocation_to(initial_capacity);
            initialize_meta();
        }

        RawHashSet& operator=(const RawHashSet& other)
        {
            if (this != &other) // add an actual *this != other AFTER self-assignment check
            {
                // TODO
            }

            return *this;
        }

        RawHashSet& operator=(RawHashSet&& other) noexcept
        {
            if (this != &other) // add an actual *this != other AFTER self-assignment check
            {
                // TODO
            }

            return *this;
        }

        ~RawHashSet() { destroy_allocation(); }

        [[nodiscard]] iterator find(const key_type& key)
        {
            const auto hash_value = hash_key(key);
            const auto [probed_index, state] = probe_for<false>(key, hash_value);

            if (state == BucketState::empty)
                return end();
            else
                return iterator_to(probed_index);
        }

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
        std::pair<bool, iterator> insert(value_type&& value)
        {
            rehash_if_required();

            const auto& key = Traits::key_from(value);
            const auto hash_value = hash_key(key);
            const auto [index, state] = probe_for<true>(key, hash_value);

            if (state != BucketState::full)
            {
                Traits::construct(alloc_ref(), value_ptr_to(index), std::forward<decltype(value)>(value));
                meta_array()[index] = BucketState::full;

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
        [[nodiscard]] float load_factor() const noexcept { return bucket_count() == 0 ? 0.f : size() / bucket_count(); }

        /// Gets the number of buckets in the set, aka the max number of
        /// elements it can hold with no collisions
        [[nodiscard]] size_type bucket_count() const noexcept { return capacity_; }

        /// Gets the current number of elements in the set.
        [[nodiscard]] size_type size() const noexcept { return size_; }

        /// Checks if the set is completely empty, equivalent to `set.size() == 0`
        [[nodiscard]] bool empty() const noexcept { return size() == 0; }

        /// If `new_size` is over `size()`, rehashes to have that many buckets. All elements
        /// are moved, and all iterators/pointers/references are invalidated.
        void rehash(size_type) { std::abort(); }

        void swap(RawHashSet& other) noexcept
        {
            using std::swap;

            if constexpr (AllocTraits<RealAlloc>::propagate_on_container_swap)
            {
                swap(alloc_, other.alloc_);
            }

            swap(hash_, other.hash_);
            swap(eq_, other.eq_);
            swap(values_, other.values_);
            swap(capacity_, other.capacity_);
            swap(size_, other.size_);
        }

        /// Gets an iterator to the first value of the table
        [[nodiscard]] iterator begin() noexcept
        {
            const auto first_full = ranges::find(meta_array(), BucketState::full);
            const auto index = std::distance(meta_array().begin(), first_full);

            return iterator{this, static_cast<size_type>(index)};
        }

        /// Gets a past-the-end iterator on the table
        [[nodiscard]] iterator end() noexcept
        {
            const auto meta = meta_array();
            const auto end_index = std::distance(meta.begin(), meta.end());

            return iterator{this, static_cast<size_type>(end_index)};
        }

    protected:
        // compares `load_factor()` to `bucket_count() * max_load_factor()`
        bool should_resize() noexcept { return load_factor() > bucket_count() * max_load_factor(); }

        // checks if a bucket is both full and has a key equivalent to `key`
        bool full_and_key_eq(const key_type& key, BucketState state, size_type index)
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
        template <bool ReturnTombstones>
        std::pair<size_type, BucketState> probe_for(const key_type& key, size_type index)
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
        void for_each_full_slot(std::invocable<std::size_t> auto lambda)
        {
            for (auto i : zinc::range_of(std::size_t{0}, bucket_count()))
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
            for_each_full_slot([this](std::size_t slot_index) {
                // in theory the types could be trivial, but because the allocator might have non-dtor side
                // effects, we need to actually call `AllocTraits::destroy`. If it doesn't, any good optimizer
                // will remove it anyway (as by default it's `std::destroy_at`, aka `ptr->~T();`
                std::allocator_traits<RealAlloc>::destroy(alloc_ref(), value_ptr_to(slot_index));
            });

            size_ = 0;
        }

        // destroys each value
        void destroy_allocation()
        {
            clear_slots();

            std::allocator_traits<RealAlloc>::deallocate(alloc_ref(), underlying_storage(), byte_size());
        }

        // checks if the table needs to be rehashed, and rehashes if it does
        void rehash_if_required()
        {
            if (should_resize())
            {
                // resize correctly, transfer to then call dtors
            }
        }

        // destroys the current storage and expands it to a new size, with enough
        // room for `new_capacity` `Stored` objects and `BucketState` objects.
        // Updates bucket_count but not size, size needs to be updated by caller.
        //
        // does not destruct any elements in the storage, must be done before calling
        void expand_allocation_to(size_type new_capacity)
        {
            // the only case where this **isn't** true is the initial allocation
            if (values()) [[likely]]
            {
                std::allocator_traits<RealAlloc>::deallocate(alloc_ref(), underlying_storage(), byte_size());
            }

            values_ = std::allocator_traits<RealAlloc>::allocate(alloc_ref(), byte_size(new_capacity));
            capacity_ = new_capacity;
        }

        // initializes the meta table to all `BucketState::empty` correctly. Assumes
        // lifetime of each element hasn't started yet and constructs at, instead
        // of re-assigning.
        void initialize_meta() noexcept
        {
            const auto end = meta() + bucket_count();

            for (auto* ptr = meta(); ptr != end; ++ptr)
            {
                // very likely to be `std::construct_at` and therefore just `new (addr)`, but
                // on the off chance the allocator defines its own `construct` we use this
                std::allocator_traits<RealAlloc>::construct(alloc_ref(), ptr, BucketState::empty);
            }
        }

        // Correctly hashes a key
        [[nodiscard]] size_type hash_key(const key_type& key) const noexcept
        {
            return hash_ref()(key) % bucket_count();
        }

        // Returns the byte-wise size of the current allocation
        [[nodiscard]] size_type byte_size() const noexcept { return byte_size(bucket_count()); }

        // Returns the full byte-wise size of the allocated space, assuming a bucket_count of
        // `bucket_count` values and the allocation containing both a `Stored[]` and `BucketState[]`
        // with both arrays being exactly `bucket_count` long.
        [[nodiscard]] constexpr static size_type byte_size(std::size_t capacity) noexcept
        {
            return (capacity * sizeof(Stored)) + (capacity * sizeof(BucketState));
        }

        // gets an iterator starting at a specific index
        [[nodiscard]] iterator iterator_to(size_type index) noexcept { return iterator{this, index}; }

        // gets a const iterator starting at a specific index
        [[nodiscard]] const_iterator const_iterator_to(size_type index) noexcept { return const_iterator{this, index}; }

        // Gets a mutable pointer to the value section of the allocation
        [[nodiscard]] Stored* values() noexcept { return pointer_cast<Stored*>(values_); }

        // Gets a pointer to the value section of the allocation
        [[nodiscard]] const Stored* values() const noexcept { return pointer_cast<const Stored*>(values_); }

        // Gets a pointer to the meta table section of the allocation
        [[nodiscard]] BucketState* meta() noexcept { return pointer_cast<BucketState*>(values() + bucket_count()); }

        // Gets a span over the entire meta array, usable for ranged-for and friends
        [[nodiscard]] std::span<BucketState> meta_array() noexcept { return meta_array_at(0); }

        [[nodiscard]] Stored* value_ptr_to(size_type index) noexcept
        {
            assert("attempting to access index beyond bucket_count" && index < bucket_count());

            return values() + index;
        }

        [[nodiscard]] const Stored* value_ptr_to(size_type index) const noexcept
        {
            assert("attempting to access index beyond bucket_count" && index < bucket_count());

            return values() + index;
        }

        [[nodiscard]] Stored& values_at(size_type index) noexcept
        {
            assert("attempting to access uninitialized value" && is_full_at(index));

            return *value_ptr_to(index);
        }

        [[nodiscard]] const Stored& values_at(size_type index) const noexcept
        {
            assert("attempting to access uninitialized value" && is_full_at(index));

            return *value_ptr_to(index);
        }

        [[nodiscard]] const BucketState* meta() const noexcept
        {
            assert("not attempting to access empty table" && underlying_storage());

            return pointer_cast<const BucketState*>(values() + bucket_count());
        }

        [[nodiscard]] BucketState* meta_ptr_to(size_type index) noexcept
        {
            assert("attempting to access index beyond bucket_count" && index < bucket_count());

            return meta() + index;
        }

        [[nodiscard]] const BucketState* meta_ptr_to(size_type index) const noexcept
        {
            assert("attempting to access index beyond bucket_count" && index < bucket_count());

            return meta() + index;
        }

        [[nodiscard]] BucketState meta_at(size_type index) const noexcept { return *meta_ptr_to(index); }

        // Gets a span over a subsection of the meta array, from `index` to the end.
        [[nodiscard]] std::span<BucketState> meta_array_at(std::size_t index) noexcept
        {
            return std::span{meta() + index, meta() + bucket_count()};
        }

        // in a few specific cases, raw access to the underlying allocation is needed. Typically
        // though only `values()` and `meta()` are needed, since everything about the allocation
        // *is* known by the class.
        [[nodiscard]] std::byte* underlying_storage() noexcept { return values_; }

        [[nodiscard]] const std::byte* underlying_storage() const noexcept { return values_; }

        [[nodiscard]] bool is_full_at(std::size_t slot) const noexcept { return meta_at(slot) == BucketState::full; }

        [[nodiscard]] bool is_empty_at(std::size_t slot) const noexcept { return meta_at(slot) == BucketState::empty; }

        [[nodiscard]] const hasher& hash_ref() const noexcept { return hash_; }

        [[nodiscard]] const key_equal& eq_ref() const noexcept { return eq_; }

        [[nodiscard]] RealAlloc& alloc_ref() noexcept { return alloc_; }

    private:
        friend struct RawHashSetIterator<value_type, RawHashSet>;
        friend struct RawHashSetIterator<const value_type, RawHashSet>;

        // Meta and data tables are all in one allocation, both tables being the same length,
        // at all times. meta_table()[i] will always be the state for whatever value lies at values()[i].
        //
        // [ VALUE, VALUE, VALUE, VALUE, meta, meta, meta, meta ]
        //   ^                           ^
        //   values_                     meta_table()
        std::byte* values_ = nullptr;
        std::size_t capacity_ = 0;
        std::size_t size_ = 0;
        [[no_unique_address]] hasher hash_;
        [[no_unique_address]] key_equal eq_;
        [[no_unique_address]] RealAlloc alloc_;
    };

    template <typename Value, typename Table> struct RawHashSetIterator
    {
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = Value;
        using reference = Value&;
        using size_type = typename Table::size_type;

        reference operator*() const noexcept
        {
            assert("not past the end" && !is_at_end());
            assert("has a value" && table->is_full_at(index));

            return table->values_at(index);
        }

        RawHashSetIterator& operator++() noexcept
        {
            find_next_full();

            return *this;
        }

        RawHashSetIterator operator++(int) noexcept
        {
            const auto copy = RawHashSetIterator{table, index};

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
            const auto copy = RawHashSetIterator{table, index};

            find_next_full_backwards();

            return copy;
        }

        [[nodiscard]] bool operator==(RawHashSetIterator other) { return other.table == table && other.index == index; }

        // public to make it an aggregate / trivial
        Table* table;
        size_type index;

    protected:
        [[nodiscard]] bool is_at_end() const noexcept { return table->bucket_count() <= index; }

        [[nodiscard]] BucketState current_value() const noexcept { return table->meta_at(index); }

        [[nodiscard]] bool is_current_full() const noexcept { return current_value() == BucketState::full; }

        void find_next_full()
        {
            ++index; // get past the current element, prevents an infinite loop

            while (!is_at_end() && !is_current_full())
                ++index;
        }

        void find_next_full_backwards()
        {
            --index; // get past current element, prevents an infinite loop

            while (!is_at_end() && !is_current_full())
                --index;
        }
    };
} // namespace zinc::detail

#endif