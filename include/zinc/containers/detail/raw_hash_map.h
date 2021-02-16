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

#ifndef ZINC_CONTAINERS_DETAIL_RAW_HASH_MAP
#define ZINC_CONTAINERS_DETAIL_RAW_HASH_MAP

#include "zinc/containers/detail/map_traits.h"
#include "zinc/containers/detail/raw_hash_set.h"
#include "zinc/types/functors.h"
#include <functional>
#include <memory>
#include <utility>

namespace zinc::detail
{
    /// Extensions to `RawHashSet` meant for K:V stores instead of just V stores
    template <MapTraits Traits> class RawHashMap : private RawHashSet<Traits>
    {
        using Base = RawHashSet<Traits>;

    public:
        using key_type = typename Base::key_type;
        using value_type = typename Base::value_type;
        using mapped_type = MapMapped<Traits>;
        using size_type = typename Base::size_type;
        using difference_type = typename Base::difference_type;
        using hasher = typename Base::hasher;
        using key_equal = typename Base::key_equal;
        using allocator_type = typename Base::allocator_type;
        using reference = typename Base::reference;
        using const_reference = typename Base::const_reference;
        using pointer = typename Base::pointer;
        using const_pointer = typename Base::const_pointer;
        using iterator = typename Base::iterator;
        using const_iterator = typename Base::const_iterator;

        using Base::Base;

        using Base::operator=;

        using Base::get_allocator;

        using Base::begin;

        using Base::end;

        using Base::cbegin;

        using Base::cend;

        using Base::empty;

        using Base::size;

        using Base::clear;

        using Base::insert;

        using Base::emplace;

        using Base::emplace_hint;

        using Base::erase;

        using Base::extract;

        using Base::merge;

        using Base::count;

        using Base::find;

        using Base::contains;

        using Base::bucket_count;

        using Base::load_factor;

        using Base::max_load_factor;

        using Base::rehash;

        using Base::reserve;

        using Base::hash_function;

        using Base::key_eq;

        std::pair<iterator, bool> try_emplace(const key_type& key, auto&&... args) requires std::
            constructible_from<value_type, std::piecewise_construct_t, const key_type&, decltype(args)...>
        {
            const auto hash_value = hash_key(key);
            const auto [index, state] = probe_for<true>(key, hash_value);

            // tombstones can get re-used here, no point to leaving them as tombstones
            // when they'll just add to load_factor & get probed over anyway
            if (state != BucketState::full)
            {
                Traits::construct(Base::alloc_ref(),
                    Base::value_ptr_to(index),
                    std::piecewise_construct,
                    key,
                    std::forward<decltype(args)>(args)...);

                update_meta(index, BucketState::full);

                if (state == BucketState::empty)
                {
                    ++Base::size_;
                }

                return {iterator_to(index), true};
            }

            return {iterator_to(index), false};
        }

        std::pair<iterator, bool> try_emplace(key_type&& key, auto&&... args) requires std::
            constructible_from<value_type, std::piecewise_construct_t, key_type&&, decltype(args)...>
        {
            const auto hash_value = hash_key(key);
            const auto [index, state] = probe_for<true>(key, hash_value);

            // tombstones can get re-used here, no point to leaving them as tombstones
            // when they'll just add to load_factor & get probed over anyway
            if (state != BucketState::full)
            {
                Traits::construct(Base::alloc_ref(),
                    Base::value_ptr_to(index),
                    std::piecewise_construct,
                    std::move(key),
                    std::forward<decltype(args)>(args)...);

                update_meta(index, BucketState::full);

                if (state == BucketState::empty)
                {
                    ++Base::size_;
                }

                return {iterator_to(index), true};
            }

            return {iterator_to(index), false};
        }

        iterator try_emplace(const_iterator, const key_type& key, auto&&... args)
        {
            return try_emplace(key, std::forward<decltype(args)>(args)...).first;
        }

        iterator try_emplace(const_iterator, key_type&& key, auto&&... args)
        {
            return try_emplace(std::move(key), std::forward<decltype(args)>(args)...).first;
        }

        [[nodiscard]] mapped_type& at(const EqComparable<const key_type&, key_equal> auto& key)
        {
            auto it = find(key);

            if (it == end())
            {
                throw std::out_of_range("RawHashMap: key was not found");
            }

            return *it;
        }

        [[nodiscard]] const mapped_type& at(const EqComparable<const key_type&, key_equal> auto& key) const
        {
            auto it = find(key);

            if (it == end())
            {
                throw std::out_of_range("RawHashMap: key was not found");
            }

            return *it;
        }

        mapped_type& operator[](const EqComparable<const key_type&, key_equal> auto& key)
        {
            return try_emplace(key).first->second;
        }

        [[nodiscard]] bool operator==(const RawHashMap& other) const
        {
            if (other.size() != size()) return false;

            const auto other_end = other.end();

            for (const auto& element : *this)
            {
                const auto it = other.find(element.first);

                if (it == other_end || *it != element)
                {
                    return false;
                }
            }

            return true;
        }
    };
} // namespace zinc::detail

#endif