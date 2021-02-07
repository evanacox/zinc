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

#ifndef ZINC_TYPES_CONCEPTS
#define ZINC_TYPES_CONCEPTS

#include <concepts>
#include <type_traits>

namespace zinc
{
    /// Checks if a type is one of N types
    template <typename T, typename... Ts> concept OneOf = (sizeof...(Ts) > 0) && (std::same_as<T, Ts> || ...);

    /// Checks if a type satisfies a standard C++ type trait with a `::value` constant
    template <typename T, template <typename> typename Trait> concept Satisfies = Trait<T>::value;

    /// Checks if a type satisfies a list of type traits
    template <typename T, template <typename> typename... Traits> concept SatisfiesAll = (Satisfies<T, Traits> && ...);

    /// Checks if a list of types all satisfy a trait
    template <template <typename> typename Trait, typename... Ts> concept AllSatisfy = (Satisfies<Ts, Trait> && ...);

    /// Checks if a type is empty
    template <typename T> concept Empty = std::is_empty_v<T>;

    /// Checks if a type is "fundamental"
    template <typename T> concept Fundamental = std::is_fundamental_v<T>;

    /// Checks if a type is a pointer
    template <typename T> concept Pointer = std::is_pointer_v<T>;

    /// Checks if a type is default constructible
    template <typename T> concept DefaultConstructible = std::constructible_from<T>;

    /// Checks if a type has a `::value_type` member type
    template <typename T> concept HasValueType = requires { typename T::value_type; };

    /// Checks if a type has a `::size_type` member type
    template <typename T> concept HasSizeType = requires { typename T::size_type; };

    /// Checks if a type has a `::pos_type` member type
    template <typename T> concept HasPosType = requires { typename T::pos_type; };
} // namespace zinc

#endif