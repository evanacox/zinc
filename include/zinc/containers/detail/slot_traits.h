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

#ifndef ZINC_CONTAINERS_DETAIL_SLOT_TRAITS
#define ZINC_CONTAINERS_DETAIL_SLOT_TRAITS

#include "zinc/types/allocators.h"
#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

namespace zinc::detail
{
    // sets up default operations on hash table "slots." Each operation is overridable
    // by a deriving traits object, but defaults are provided
    template <typename SlotT, typename ImmutableT = const SlotT> struct SlotTraits
    {
        // type returned by operations like .find(), expected to have a constant key
        using ImmutableSlotValue = ImmutableT;

        // type used internally by the table, should have non-const key to enable moves and whatnot
        using SlotValue = SlotT;

        // constructs an element into `slot` using `args`
        static void construct(Allocator<SlotValue> auto& alloc, SlotValue* slot, auto&&... args)
        {
            AllocTraits<std::remove_cvref_t<decltype(alloc)>>::construct(alloc,
                slot,
                std::forward<decltype(args)>(args)...);
        }

        // destroys the element in `slot`
        static void destroy(Allocator<SlotValue> auto& alloc, SlotValue* slot)
        {
            AllocTraits<std::remove_cvref_t<decltype(alloc)>>::destroy(alloc, slot);
        }

        // copies a slot to a new slot, without destroying the old one
        static void copy_to(Allocator<SlotValue> auto& alloc, const SlotValue* source, SlotValue* dest)
        {
            construct(alloc, dest, *source);
        }

        // moves a slot into a new slot, without destroying the old one
        static void move_to(Allocator<SlotValue> auto& alloc, SlotValue* source, SlotValue* dest) noexcept
        {
            // if it throws during move construction, user needs to fix their crap. it gets
            // static-asserted by the type anyway
            construct(alloc, dest, std::move(*source));
        }

        // moves a slot to a new slot, then destroys the old one
        static void transfer_to(Allocator<SlotValue> auto& alloc, SlotValue* source, SlotValue* dest)
        {
            move_to(alloc, source, dest);
            destroy(alloc, source);
        }
    };

    // default provided for Set structures, which don't use a pair
    template <typename K> struct SetSlotTraits : SlotTraits<std::remove_const_t<K>, const K>
    {
        using SlotTraitsHandledTypes = std::tuple<K>;
    };

    // default provided for Map structures, which use a pair (and only the first type needs to be const)
    template <typename K, typename V>
    struct MapSlotTraits : SlotTraits<std::pair<std::remove_const_t<K>, V>, std::pair<const std::remove_const_t<K>, V>>
    {
        using SlotTraitsHandledTypes = std::tuple<K, V>;
    };

    template <typename Traits, typename K, typename... V>
    concept SlotTraitsFor = std::same_as<typename Traits::SlotTraitsHandledTypes, std::tuple<K, V...>>
        // clang-format off
        && requires(std::allocator<typename Traits::SlotValue> alloc, typename Traits::SlotValue* a,
                    typename Traits::SlotValue* b, const typename Traits::SlotValue* c)
    {
        // can't test `construct` without knowing arguments for the types' ctor
        { Traits::destroy(alloc, a) } -> std::same_as<void>;
        { Traits::copy_to(alloc, c, a) } -> std::same_as<void>;
        { Traits::move_to(alloc, a, b) } -> std::same_as<void>;
        { Traits::transfer_to(alloc, a, b) } -> std::same_as<void>;
    };
    // clang-format on
} // namespace zinc::detail

#endif