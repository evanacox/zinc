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

#ifndef ZINC_CONTAINERS_DETAIL_MAP_TRAITS
#define ZINC_CONTAINERS_DETAIL_MAP_TRAITS

#include "zinc/containers/detail/set_traits.h"
#include "zinc/containers/detail/slot_traits.h"

namespace zinc::detail
{
    template <typename K,
        typename V,
        HashFn<K> Hash,
        EqFn<K> Eq,
        Allocator<K> Alloc,
        SlotTraitsFor<std::pair<K, V>, std::pair<const K, V>> SlotTraits = MapSlotTraits<K, V>>
    struct DefaultMapTraits : SlotTraits
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

        using Mapped = V;

        using MovableValue = std::pair<K, V>;

        using Value = std::pair<const K, V>;

        using Hasher = Hash;

        using KeyEq = Eq;

        using Allocator = Alloc;

        [[nodiscard]] constexpr static std::size_t initial_size() noexcept { return 32; }

        [[nodiscard]] constexpr static float max_load_factor() noexcept { return 0.72f; }

        [[nodiscard]] constexpr static const Key& key_from(const SlotValue& slot_value) { return slot_value.first; }

        [[nodiscard]] constexpr static const Value& value_from(const ImmutableSlotValue& slot_value)
        {
            return slot_value;
        }
    };

    template <typename Traits> concept MapTraits = SetTraits<Traits>&& requires { typename Traits::Mapped; };

    template <MapTraits Traits> using MapMapped = typename Traits::Mapped;
} // namespace zinc::detail

#endif