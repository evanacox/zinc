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

#ifndef ZINC_TYPES_POINTERS
#define ZINC_TYPES_POINTERS

#include <concepts>
#include <memory>

namespace zinc
{
    namespace detail
    {
        template <template <typename...> typename> constexpr bool has_member_template() { return true; }

        template <typename> constexpr bool has_member_template() { return false; }
    } // namespace detail

    /// Checks if a type is "pointer-like," aka whether or not they're usable with
    /// `std::pointer_traits`.
    template <typename T> concept Pointerlike = requires(typename std::pointer_traits<T>::element_type r)
    {
        typename std::pointer_traits<T>::pointer;
        typename std::pointer_traits<T>::difference_type;
        // clang-format off
        { std::pointer_traits<T>::pointer_to(r) } -> std::same_as<typename std::pointer_traits<T>::pointer>;
        // clang-format on
    }
    &&detail::has_member_template<std::pointer_traits<T>::template rebind>();

    /// Alias to `std::pointer_traits<T>`
    template <Pointerlike T> using PointerTraits = std::pointer_traits<T>;

    namespace detail
    {
        template <typename To, typename From>
        concept CastInto = Pointerlike<To>&& Pointerlike<From>&& requires(From* ptr)
        {
            static_cast<To*>(static_cast<void*>(ptr));
        };

        template <typename T> struct IsConstPointer : std::false_type
        {};

        template <typename T> struct IsConstPointer<const T*> : std::true_type
        {};
    } // namespace detail

    /// Checks if a type is a `const` pointer. Useful due to the fact
    /// that `std::is_const` only checks for `const`-ness of the **value**
    /// of the pointer, instead of checking if the pointed-to value is modifiable.
    template <typename T> concept ImmutablePointer = detail::IsConstPointer<T>();

    /// Does an unsafe cast of one pointer to another, using the C++ guarantee that
    /// casting a pointer to `void*` maintains the address.
    ///
    /// # Type Parameters
    /// - `To`: The pointer type to cast into after the pointer is casted to `void*`
    /// - `From`: The pointer type to cast into `void*`
    ///
    /// # Parameters
    /// - `ptr`: The pointer to cast from `From` into `To`
    ///
    /// # Returns
    /// Returns a pointer of `To`, the address of which is equal to `ptr`
    template <Pointerlike To, Pointerlike From>
    [[nodiscard]] constexpr To pointer_cast(From ptr) noexcept requires detail::CastInto<To, From>
    {
        // if From is `const` and it's casted to a `void*` we get a compile error, so
        // we need a conditional check to get a possibly-const `void*` type
        using MaybeConstVoidPtr = std::conditional_t<detail::IsConstPointer<From>::value, const void*, void*>;

        return static_cast<To>(static_cast<MaybeConstVoidPtr>(ptr));
    }
} // namespace zinc

#endif
