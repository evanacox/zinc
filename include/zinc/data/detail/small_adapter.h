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

#ifndef ZINC_DATA_SMALL
#define ZINC_DATA_SMALL

#include "zinc/types/aliases.h"
#include "zinc/types/functors.h"
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace zinc::detail
{
    struct SmallAdapterPlaceholder
    {};

    /// Adapts containers to be able to use an inline "small buffer"
    template <typename Container, std::size_t N> class SmallAdapter : public Container
    {
    public:
        SmallAdapter()
            : Container(SmallAdapterPlaceholder{}, buffer_, N)
        {}

        /// Checks if the container being adapted is actually using the
        /// inline buffer, or if it offloaded to the free store/allocator
        [[nodiscard]] bool using_small() const noexcept { return Container::raw_storage() == buffer_; }

    protected:
        AlignedStorageFor<ValueT<Container>> buffer_[N];
    };
} // namespace zinc::detail

#endif
