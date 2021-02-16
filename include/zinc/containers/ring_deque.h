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

#ifndef ZINC_CONTAINERS_RING_DEQUE
#define ZINC_CONTAINERS_RING_DEQUE

#include "zinc/types/functors.h"
#include <cassert>
#include <functional>
#include <iterator>
#include <memory>
#include <utility>

namespace zinc
{
    namespace detail
    {
        template <typename T> struct RingDequeIterator
        {
            RingDequeIterator& operator++() noexcept {}

            T* buffer;
            T* head;
            T* tail;
            std::size_t index;
        };
    } // namespace detail

    /// Very similar to `VecDeque<T>` in Rust, implemented like:
    /// https://gts3.org/blog/cve-2018-1000657.assets/ring-buffer.png
    ///
    /// Able to be a drop-in replacement for `std::deque<T>` if desired.
    ///
    /// Implements a deque with a ring buffer approach, with insertion
    /// starting at both ends and gradually moving inward.
    template <typename T, Allocator<T> Alloc = std::allocator<T>> class RingDeque
    {
    public:
        RingDeque() = default;

    private:
        [[no_unique_address]] Alloc alloc_;
        T* start_ = nullptr;
        T* end_ = nullptr;
        T* head_ = nullptr;
        T* tail_ = nullptr;
    };
} // namespace zinc

#endif
