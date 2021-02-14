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

#ifndef ZINC_TYPES_FUNCTORS_EQUAL_TO
#define ZINC_TYPES_FUNCTORS_EQUAL_TO

#include "zinc/types/concepts.h"
#include "zinc/types/strings.h"
#include <concepts>
#include <functional>
#include <utility>

namespace zinc
{
    namespace detail
    {
        template <typename CharT> struct StringEq
        {
            using StringView = std::basic_string_view<CharT>;
            using StringRef = const std::basic_string<CharT>&;

            constexpr bool operator()(StringView lhs, StringView rhs) const noexcept { return lhs == rhs; }

            constexpr bool operator()(StringRef lhs, StringRef rhs) const noexcept { return lhs == rhs; }

            constexpr bool operator()(StringView lhs, StringRef rhs) const noexcept { return lhs == rhs; }

            constexpr bool operator()(StringRef lhs, StringView rhs) const noexcept { return lhs == rhs; }
        };
    } // namespace detail

    /// Zinc wrapper around `std::equal_to`. Used to provide heterogeneous lookup on containers,
    /// without messing with `std::equal_to`s interactions with `std::` types.
    template <typename T> struct EqualTo : std::equal_to<T>
    {};

    /// Specialization for `std::basic_string` and `std::basic_string_view` types that enables
    /// heterogeneous string lookup
    template <StringOrStringViewSpecialization T> struct EqualTo<T> : detail::StringEq<ValueT<T>>
    {};

    /// Checks if a functor is usable as an equality function for a type `T`.
    template <typename Eq, typename T> concept EqFn = requires(const Eq eq, T a, T b)
    {
        // clang-format off
        { eq(a, b) } -> std::same_as<bool>;
        // clang-format on
    };

    template <typename U, typename T, typename Eq = zinc::EqualTo<T>>
    concept EqComparable = EqFn<Eq, T>&& requires(const Eq eq, T t, U u)
    {
        // clang-format off
        { eq(t, u) } -> std::same_as<bool>;
        // clang-format on
    };
} // namespace zinc

#endif