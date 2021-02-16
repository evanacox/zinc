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

#ifndef ZINC_CONTAINERS_HASH_MAP
#define ZINC_CONTAINERS_HASH_MAP

#include "zinc/containers/detail/raw_hash_map.h"
#include "zinc/types/functors.h"
#include <functional>
#include <memory>
#include <utility>

namespace zinc
{
    namespace detail
    {
        template <typename K, typename V, typename H, typename E, typename A>
        using HashMapTraits = DefaultMapTraits<K, V, H, E, A>;
    }

    /// Better default hash map than `std::unordered_map`. Implemented using
    /// a hash table (as the name implies), with linear probing. No
    /// separate chaining, no stability for values, but no indirection on
    /// accesses. `NodeHashMap` exists when stability is needed for both key
    /// and value, and when only value stability is needed, something like
    /// a `HashMap<K, std::unique_ptr<V>>` works just fine.
    template <typename Key,
        typename Value,
        HashFn<Key> Hash = zinc::Hash<Key>,
        EqFn<Key> Eq = zinc::EqualTo<Key>,
        Allocator<std::pair<const Key, Value>> Allocator = std::allocator<std::pair<const Key, Value>>>
    class HashMap : private detail::RawHashMap<detail::DefaultMapTraits<Key, Value, Hash, Eq, Allocator>>
    {
        using Traits = detail::DefaultMapTraits<Key, Value, Hash, Eq, Allocator>;
        using Base = detail::RawHashMap<Traits>;

    public:
        using key_type = typename Base::key_type;
        using value_type = typename Base::value_type;
        using mapped_type = typename Base::mapped_type;
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

        using Base::at;

        using Base::operator[];

        using Base::insert;

        using Base::emplace;

        using Base::emplace_hint;

        using Base::try_emplace;

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