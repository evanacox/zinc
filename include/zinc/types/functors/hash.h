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

#ifndef ZINC_TYPES_FUNCTORS_HASH
#define ZINC_TYPES_FUNCTORS_HASH

#include "zinc/types/concepts.h"
#include <concepts>
#include <functional>
#include <utility>

namespace zinc
{
    template <typename T>
    // clang-format off
    concept Hashable = DefaultConstructible<std::hash<T>>
        && std::copy_constructible<std::hash<T>>
        && std::move_constructible<std::hash<T>>
        && std::is_copy_assignable_v<std::hash<T>>
        && std::is_move_assignable_v<std::hash<T>>
        && requires(const T& v)
    {
        { std::hash<T>{} };
        { std::hash<T>{}(v) } -> std::same_as<std::size_t>;
    };
    // clang-format on

    namespace detail
    {
        template <typename F, typename... Args> void for_each_arg(F f, Args&&... args)
        {
            (f(std::forward<Args>(args)), ...);
        }
    } // namespace detail

    template <typename... Args> requires(Hashable<Args>&&...) std::size_t hash(const Args&... args)
    {
        std::size_t val = 0;

        // clang-format off
        detail::for_each_arg([&](const auto& arg) {
            using Type = std::remove_cvref_t<decltype(arg)>;

            val ^= std::hash<Type>{}(arg) + (0x9e3779b9 + (val << 6) + (val >> 2));
        }, std::forward<Args>(args)...);
        // clang-format on

        return val;
    }

    /// Zinc wrapper around `std::hash`. Used to provide a few utilities for implementing
    /// hashes on non-`std` types, but still build on `std::hash` and any specializations of it.
    template <typename T> struct Hash : std::hash<T>
    {
        // implement some magic in here
    };

    /// Checks that a type is a valid `Cpp17Hash`. See https://timsong-cpp.github.io/cppwp/n4861/hash.requirements
    /// for the specifics, this checks what it can
    template <typename Hash, typename T> // clang-format off
    concept HashFn = std::copy_constructible<Hash>
        && std::destructible<Hash>
        && requires(const Hash h, const T& t)
    {
        { h(t) } -> std::same_as<std::size_t>;
    };
    // clang-format on
} // namespace zinc

#endif