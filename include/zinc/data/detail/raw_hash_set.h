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

#include "zinc/types/allocators.h"
#include "zinc/types/functors.h"
#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>

namespace zinc::detail
{
    enum class BucketState : unsigned char
    {
        empty,
        full,
        tombstone
    };

    template <typename Traits> concept HashSetTraitTypes = requires
    {
        typename Traits::Stored;
        typename Traits::Value;
        typename Traits::Allocator;
        typename Traits::Hasher;
        typename Traits::KeyEq;
    };

    template <HashSetTraitTypes Traits> using HSTStored = typename Traits::Stored;
    template <HashSetTraitTypes Traits> using HSTValue = typename Traits::Value;
    template <HashSetTraitTypes Traits> using HSTAllocator = typename Traits::Allocator;
    template <HashSetTraitTypes Traits> using HSTHasher = typename Traits::Hasher;
    template <HashSetTraitTypes Traits> using HSTKeyEq = typename Traits::KeyEq;

    template <typename Traits> concept HashSetTraits = HashSetTraitTypes<Traits>;

    /// Better default hash map than `std::unordered_map`. Implemented using
    /// a hash table (as the name implies), with linear probing. No
    /// separate chaining, no stability for values, but no indirection on
    /// accesses. `NodeHashMap` exists when stability is needed for both key
    /// and value, and when only value stability is needed, something like
    /// a `HashMap<K, std::unique_ptr<V>>` works just fine.
    template <HashSetTraits Traits> class RawHashSet
    {
        using Stored = HSTStored<Traits>;

    public:
        using key_type = HSTValue<Traits>;
        using value_type = HSTValue<Traits>;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using hasher = HSTHasher<Traits>;
        using key_equal = HSTKeyEq<Traits>;
        using allocator_type = HSTAllocator<Traits>;
        using pointer = HSTHasher<Traits>;

        RawHashSet() = default;

    protected:
        void update_meta(std::size_t index, BucketState new_state) noexcept
        {
            assert("attempting to access index beyond capacity" && index < capacity_);

            meta()[index] = new_state;
        }

        [[nodiscard]] Stored* values() noexcept { return values_; }

        [[nodiscard]] Stored& values(std::size_t index) noexcept
        {
            assert("attempting to access index beyond capacity" && index < capacity_);
            assert("attempting to access uninitialized value" && is_full_at(index));

            return values()[index];
        }

        [[nodiscard]] const Stored& values(std::size_t index) const noexcept { return values(index); }

        [[nodiscard]] BucketState* meta() noexcept { return reinterpret_cast<BucketState*>(values() + capacity_); }

        [[nodiscard]] BucketState meta(std::size_t index) const noexcept
        {
            assert("attempting to access index beyond capacity" && index < capacity_);

            return meta()[index];
        }

        [[nodiscard]] bool is_full_at(std::size_t index) const noexcept { return meta(index) == BucketState::full; }

        [[nodiscard]] bool is_empty_at(std::size_t index) const noexcept { return meta(index) == BucketState::empty; }

    private:
        // Meta and data tables are all in one allocation, both being **exactly**
        // the same length, at all times. meta(i) will always be the state for
        // whatever value lies at values(i).
        //
        // [ VALUE, VALUE, VALUE, VALUE, meta, meta, meta, meta ]
        //   ^                           ^
        //   values_                     meta_table()
        Stored* values_;
        std::size_t capacity_;
        std::size_t size_;
        [[no_unique_address]] hasher hash_;
        [[no_unique_address]] key_equal eq_;
        [[no_unique_address]] allocator_type alloc_;
    };
} // namespace zinc::detail

#endif