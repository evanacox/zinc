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

#ifndef ZINC_TYPES_STORAGE
#define ZINC_TYPES_STORAGE

#include "zinc/types/concepts.h"
#include <cstddef>

namespace zinc
{
    /// Slightly better `std::aligned_storage_t`, guaranteed to be exactly enough space
    /// for an aligned `T` and no more.
    template <typename T> struct StorageFor
    {
        alignas(T) std::byte storage[sizeof(std::byte)];
    };
} // namespace zinc

#endif