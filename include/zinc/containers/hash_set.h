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

#ifndef ZINC_CONTAINERS_HASH_SET
#define ZINC_CONTAINERS_HASH_SET

#include "zinc/containers/detail/raw_hash_set.h"
#include "zinc/types/functors.h"
#include <functional>
#include <memory>
#include <utility>

namespace zinc
{
    /// Better default hash set than `std::unordered_set`. Implemented using
    /// a flat hash table with linear probing. No separate chaining, no stability
    /// for values, but no indirection on accesses. `NodeHashMap` exists when
    /// stability is needed for the key.
    template <typename Key,
        typename Hash = std::hash<Key>,
        typename Eq = zinc::EqualTo<Key>,
        typename Allocator = std::allocator<Key>>
    class HashSet : private detail::RawHashSet<detail::DefaultSetTraits<Key, Hash, Eq, Allocator>>
    {
        using Traits = detail::DefaultSetTraits<Key, Hash, Eq, Allocator>;
        using Base = detail::RawHashSet<Traits>;

    public:
        using key_type = typename Base::key_type;
        using value_type = typename Base::value_type;
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

        using Base::operator==;
    };
} // namespace zinc

#endif