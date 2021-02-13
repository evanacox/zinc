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

#ifndef ZINC_TYPES_ALLOCATORS
#define ZINC_TYPES_ALLOCATORS

#include "zinc/types/pointers.h"
#include <concepts>
#include <cstddef>
#include <memory>

namespace zinc
{
    namespace detail
    {
        template <typename A> concept AllocatorTypes = requires
        {
            typename A::value_type;
            typename std::allocator_traits<A>::value_type;
            typename std::allocator_traits<A>::pointer;
            typename std::allocator_traits<A>::const_pointer;
            typename std::allocator_traits<A>::void_pointer;
            typename std::allocator_traits<A>::const_void_pointer;
            typename std::allocator_traits<A>::size_type;
            typename std::allocator_traits<A>::difference_type;
        }
        // this one is after, as `std::allocator_traits` will break without a `value_type` and give a really crappy
        // error message, if this one is evaluated afterwards the error is better
        &&has_member_template<std::allocator_traits<A>::template rebind_alloc>();
    } // namespace detail

    /// Gets an allocator's `value_type` type through `std::allocator_traits`
    template <detail::AllocatorTypes Alloc> using AllocValue = typename std::allocator_traits<Alloc>::value_type;

    /// Gets an allocator's `size_type` type through `std::allocator_traits`
    template <detail::AllocatorTypes Alloc> using AllocSize = typename std::allocator_traits<Alloc>::size_type;

    /// Gets an allocator's `difference_type` type through `std::allocator_traits`
    template <detail::AllocatorTypes Alloc>
    using AllocDifference = typename std::allocator_traits<Alloc>::difference_type;

    /// Gets an allocator's `pointer` type through `std::allocator_traits`
    template <detail::AllocatorTypes Alloc> using AllocPointer = typename std::allocator_traits<Alloc>::pointer;

    /// Gets an allocator's `const_pointer` type through `std::allocator_traits`
    template <detail::AllocatorTypes Alloc>
    using AllocConstPointer = typename std::allocator_traits<Alloc>::const_pointer;

    /// Gets an allocator's `void_pointer` type through `std::allocator_traits`
    template <detail::AllocatorTypes Alloc>
    using AllocVoidPointer = typename std::allocator_traits<Alloc>::void_pointer;

    /// Gets an allocator's `const_void_pointer` type through `std::allocator_traits`
    template <detail::AllocatorTypes Alloc>
    using AllocConstVoidPointer = typename std::allocator_traits<Alloc>::const_void_pointer;

    /// Rebinds an allocator to another type
    template <detail::AllocatorTypes Alloc, typename T>
    using AllocRebind = typename std::allocator_traits<Alloc>::template rebind_alloc<T>;

    /// Rebinds an allocator to another type, and returns `allocator_traits` for that new allocator
    template <detail::AllocatorTypes Alloc, typename T>
    using AllocRebindTraits = typename std::allocator_traits<AllocRebind<Alloc, T>>;

    /// Returns `std::allocator_traits` for a given alloc_ref
    template <detail::AllocatorTypes Alloc> using AllocTraits = typename std::allocator_traits<Alloc>;

    namespace detail
    {
        /// Used for `Allocator` concept as the `Y` type mentioned in the standard's requirements
        struct AnonymousType
        {};
    } // namespace detail

    /// Checks if a type is a valid `Cpp17Allocator`. See the standard for the "complete" list:
    /// https://timsong-cpp.github.io/cppwp/n4861/allocator.requirements
    ///
    /// Do note that some of the requirements are simply not possible to cover in a concept, and
    /// require some specific run-time behaviour.
    ///
    /// Other notes: Does not **exactly** follow the standard for the "defaults," relies on
    /// `std::allocator_traits` actually having those defaults to work correctly.
    template <typename A, typename T>
    // clang-format off
    concept Allocator = detail::AllocatorTypes<A>
        && Pointerlike<AllocPointer<A>>
        && requires(A a, A& a1, A& a2, AllocRebind<A, detail::AnonymousType> b, detail::AnonymousType* c,
                    AllocPointer<A> p, AllocConstPointer<A> q, AllocValue<A>& r, AllocVoidPointer<A> w,
                    AllocConstVoidPointer<A> x, AllocSize<A> n)
    {
        { *p } noexcept -> std::same_as<AllocValue<A>&>;
        { *q } noexcept -> std::same_as<const AllocValue<A>&>;
        { static_cast<AllocPointer<A>>(w) } noexcept -> std::same_as<AllocPointer<A>>;
        { static_cast<AllocConstPointer<A>>(x) } noexcept -> std::same_as<AllocConstPointer<A>>;
        { PointerTraits<AllocPointer<A>>::pointer_to(r) } noexcept -> std::same_as<AllocPointer<A>>;
        { a.allocate(n) } -> std::same_as<AllocPointer<A>>;
        { AllocTraits<A>::allocate(a, n, x) } -> std::same_as<AllocPointer<A>>;
        { a.deallocate(p, n) } -> std::same_as<void>;
        { AllocTraits<A>::max_size(a) } noexcept -> std::same_as<AllocSize<A>>;
        { a1 == a2 } -> std::same_as<bool>;
        { a1 != a2 } -> std::same_as<bool>;
        { a == b } -> std::same_as<bool>;
        { a != b } -> std::same_as<bool>;
        { A(a) };
        { A(b) };
        { A(std::move(a)) };
        { A(std::move(b)) };
        { AllocTraits<A>::construct(a, c) } -> std::same_as<void>;
        { AllocTraits<A>::destroy(a, c) } -> std::same_as<void>;
        { AllocTraits<A>::select_on_container_copy_construction(a) } -> std::same_as<A>;
        typename AllocTraits<A>::propagate_on_container_copy_assignment;
        typename AllocTraits<A>::propagate_on_container_move_assignment;
        typename AllocTraits<A>::propagate_on_container_swap;
        typename AllocTraits<A>::is_always_equal;
    };
    // clang-format on
} // namespace zinc

#endif