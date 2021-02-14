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

#ifndef ZINC_DATA_FLAT_HASH_SET
#define ZINC_DATA_FLAT_HASH_SET

#include "zinc/data/detail/raw_hash_set.h"
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
    class HashSet
    {
        //
    };
} // namespace zinc

#endif