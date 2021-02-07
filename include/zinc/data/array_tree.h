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

#ifndef ZINC_DATA_ARRAY_TREE
#define ZINC_DATA_ARRAY_TREE

#include "zinc/types/strings.h"
#include <memory>
#include <utility>

namespace zinc
{
    /// Rope data structure, as described in the following:
    /// https://en.wikipedia.org/w/index.php?title=Rope_(data_structure)&oldid=989349711
    ///
    /// Effectively a BTree containing many smaller strings, allowing for more efficient
    /// insertions/prepends/other manipulations of long strings that would be too expensive
    /// with a linear string representation
    template <Charlike CharT, typename Alloc = std::allocator<std::basic_string<CharT>>> class Rope
    {
        //
    };
} // namespace zinc

#endif
