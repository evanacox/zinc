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

#ifndef ZINC_TYPES_ALIASES
#define ZINC_TYPES_ALIASES

#include "zinc/types/concepts.h"
#include <type_traits>

namespace zinc
{
    /// Signed word-sized integer type
    using SignedWord = std::make_signed_t<std::size_t>;

    /// Shorthand for `typename T::value_type`
    template <HasValueType T> using ValueT = typename T::value_type;

    /// Shorthand for `typename T::size_type`
    template <HasSizeType T> using SizeT = typename T::size_type;

    /// Shorthand for `typename T::pos_type`
    template <HasPosType T> using PosT = typename T::pos_type;

    /// Shorthand for `std::aligned_storage_t<sizeof(T), alignof(T)>`
    template <typename T> using AlignedStorageFor = std::aligned_storage_t<sizeof(T), alignof(T)>;
} // namespace zinc

#endif