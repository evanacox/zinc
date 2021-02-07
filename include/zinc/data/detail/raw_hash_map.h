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

#ifndef ZINC_DATA_DETAIL_RAW_HASH_MAP
#define ZINC_DATA_DETAIL_RAW_HASH_MAP

#include "zinc/data/detail/raw_hash_set.h"
#include "zinc/types/functors.h"
#include <functional>
#include <memory>
#include <utility>

namespace zinc::detail
{
    /// Extensions to `RawHashSet` meant for K:V stores instead of just V stores
    template <typename HashTraits> class RawHashMap : detail::RawHashSet<HashTraits>
    {
        //
    };
} // namespace zinc::detail

#endif