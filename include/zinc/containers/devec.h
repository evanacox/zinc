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

#ifndef ZINC_CONTAINERS_DEVEC
#define ZINC_CONTAINERS_DEVEC

#include "zinc/types/functors.h"
#include <functional>
#include <memory>
#include <utility>

namespace zinc
{
    /// Similar in design to https://www.boost.org/doc/libs/1_75_0/doc/html/boost/container/devector.html
    ///
    /// Works by effectively trying to "balance" the elements in the middle of the
    /// allocation, allowing amortized O(1) insertion at both ends. Does not work
    /// Works best when there's very balanced front/back insertions.
    ///
    /// Main difference between this and `RingDeque<T>` is that `RingDeque<T>`
    /// expands inward from both ends and doesn't care which end you insert
    /// at more, but it also loses complete contiguity. This type is completely
    /// contiguous but has to do more work it keep it that way.
    template <typename T, typename Alloc = std::allocator<T>> class DeVec
    {
        //
    };
} // namespace zinc

#endif
