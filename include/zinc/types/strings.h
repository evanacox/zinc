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

#ifndef ZINC_TYPES_STRINGS
#define ZINC_TYPES_STRINGS

#include "zinc/types/aliases.h"
#include "zinc/types/concepts.h"
#include "zinc/types/customization.h"
#include <concepts>
#include <string_view>

namespace zinc
{
    /// Checks if a type is usable as a character type, aka "is `std::char_traits<T>` valid".
    ///
    /// By default checks if `T` is one of the built-in C++ `char` types, but specializations
    /// of `zinc::ValidChar<T>` are checked as well
    template <typename T>
    concept Charlike = OneOf<T, char, wchar_t, char16_t, char32_t, char8_t> || ValidCharType<T>::value;

    /// Checks if a type is able to be converted to a `std::string_view` (or more accurately, a
    /// `std::basic_string_view<ValueT<T>>`). Used for things like heterogenous lookup in hash tables,
    /// and a few other places.
    template <typename T>
    concept Stringlike = Charlike<ValueT<T>>&& std::convertible_to<T, std::basic_string_view<ValueT<T>>>;

    /// Checks if a Traits type is a valid drop-in for `std::char_traits` (or even is `char_traits`)
    /// for type `CharT`. Used in a few types that work with character/string data
    template <typename Traits, typename CharT>
    concept CharTraitsCompatible =
        (std::same_as<Traits, std::char_traits<CharT>>)
        // clang-format off
        || (std::same_as<typename Traits::char_type, CharT>
        && requires(CharT& a, const CharT& b, CharT c, CharT* dest, const CharT* src, std::size_t count, typename Traits::int_type n)
    {
        typename Traits::int_type;
        typename Traits::off_type;
        typename Traits::pos_type;
        typename Traits::state_type;
        { Traits::assign(a, b) } noexcept -> std::same_as<void>;
        { Traits::eq(c, c) } noexcept -> std::same_as<bool>;
        { Traits::lt(c, c) } noexcept -> std::same_as<bool>;
        { Traits::move(dest, src, count) } -> std::same_as<CharT*>;
        { Traits::copy(dest, src, count) } -> std::same_as<CharT*>;
        { Traits::compare(src, src, count) } -> std::same_as<int>;
        { Traits::length(src) } -> std::same_as<std::size_t>;
        { Traits::find(src, count, b) } -> std::same_as<const CharT*>;
        { Traits::to_char_type(n) } noexcept -> std::same_as<CharT>;
        { Traits::to_int_type(c) } noexcept -> std::same_as<typename Traits::int_type>;
        { Traits::eq_int_type(n, n) } noexcept -> std::same_as<bool>;
        { Traits::eof() } noexcept -> std::same_as<typename Traits::int_type>;
        { Traits::not_eof() } noexcept -> std::same_as<typename Traits::int_type>;
    });
    // clang-format on
} // namespace zinc

#endif