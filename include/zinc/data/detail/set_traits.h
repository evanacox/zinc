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

#ifndef ZINC_DATA_DETAIL_SET_TRAITS
#define ZINC_DATA_DETAIL_SET_TRAITS

#include "zinc/data/detail/slot_traits.h"

namespace zinc::detail
{
    /// Implements a "default" for set traits, with decent enough defaults for `construct`, `destroy`,
    /// `copy_to`, `move_to` and `transfer_to`. Also provides defaults for all the required types
    /// based on the input type parameters
    template <typename K,
        HashFn<K> Hash,
        EqFn<K> Eq,
        Allocator<K> Alloc,
        SlotTraitsFor<K> SlotTraits = SetSlotTraits<K>>
    struct DefaultSetTraits : SlotTraits
    {
        using SlotTraits::construct;

        using SlotTraits::destroy;

        using SlotTraits::copy_to;

        using SlotTraits::move_to;

        using SlotTraits::transfer_to;

        using typename SlotTraits::SlotValue;

        using typename SlotTraits::ImmutableSlotValue;

        using typename SlotTraits::SlotTraitsHandledTypes;

        using Key = K;

        using MovableValue = K;

        using Value = const K;

        using Hasher = Hash;

        using KeyEq = Eq;

        using Allocator = Alloc;

        [[nodiscard]] constexpr static float max_load_factor() noexcept { return 0.72f; }

        [[nodiscard]] constexpr static const Key& key_from(const SlotValue& slot_value) { return slot_value; }

        [[nodiscard]] constexpr static const Value& value_from(const ImmutableSlotValue& slot_value)
        {
            return slot_value;
        }
    };

    template <typename Traits>
    concept SetTraits = SlotTraitsFor<Traits, typename Traits::Key>
        // clang-format off
        && requires(const typename Traits::SlotValue& slot, const typename Traits::ImmutableSlotValue& const_slot)
    {
        typename Traits::Hasher;
        typename Traits::KeyEq;
        typename Traits::Allocator;
        { Traits::max_load_factor() } noexcept -> std::same_as<float>;
        { Traits::key_from(slot) } -> std::same_as<const typename Traits::Key&>;
        { Traits::value_from(const_slot) } -> std::same_as<const typename Traits::Value&>;
    };
    // clang-format on

    template <SetTraits Traits> using SetSlot = typename Traits::SlotValue;

    template <SetTraits Traits> using SetImmutableSlot = typename Traits::ImmutableSlotValue;

    template <SetTraits Traits> using SetKey = typename Traits::Key;

    template <SetTraits Traits> using SetMovableValue = typename Traits::MovableValue;

    template <SetTraits Traits> using SetValue = typename Traits::Value;

    template <SetTraits Traits> using SetHasher = typename Traits::Hasher;

    template <SetTraits Traits> using SetKeyEq = typename Traits::KeyEq;

    template <SetTraits Traits> using SetAlloc = typename Traits::Allocator;
} // namespace zinc::detail

#endif