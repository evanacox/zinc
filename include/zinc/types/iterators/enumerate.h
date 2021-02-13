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

#ifndef ZINC_TYPES_ITERATORS_ENUMERATE
#define ZINC_TYPES_ITERATORS_ENUMERATE

#include <cstddef>
#include <iterator>
#include <ranges>

namespace zinc
{
    namespace detail
    {
        template <std::forward_iterator I> struct EnumeratingIterator
        {
            std::size_t count;
            I iter;

            [[nodiscard]] bool operator==(const std::sentinel_for<I> auto& other) noexcept { return iter == other; }

            [[nodiscard]] std::pair<std::size_t, std::iter_reference_t<I>> operator*() noexcept
            {
                return {count, *iter};
            }

            EnumeratingIterator<I> operator++(int) noexcept(noexcept(++iter))
            {
                const auto copy = EnumeratingIterator<I>{count, iter};

                ++iter, ++count;

                return copy;
            }

            EnumeratingIterator<I>& operator++() noexcept(noexcept(++iter))
            {
                ++iter, ++count;

                return *this;
            }
        };

        template <std::bidirectional_iterator I> struct EnumeratingIterator<I>
        {
            std::size_t count;
            I iter;

            [[nodiscard]] bool operator==(const std::sentinel_for<I> auto& other) { return iter == other; }

            [[nodiscard]] std::pair<std::size_t, std::iter_reference_t<I>> operator*() noexcept
            {
                return {count, *iter};
            }

            EnumeratingIterator<I> operator++(int) noexcept(noexcept(++iter))
            {
                const auto copy = EnumeratingIterator<I>{count, iter};

                ++iter, ++count;

                return copy;
            }

            EnumeratingIterator<I>& operator++() noexcept(noexcept(++iter))
            {
                ++iter, ++count;

                return *this;
            }

            EnumeratingIterator<I> operator--(int) noexcept(noexcept(--iter))
            {
                const auto copy = EnumeratingIterator<I>{count, iter};

                --iter, --count;

                return copy;
            }

            EnumeratingIterator<I>& operator--() noexcept(noexcept(--iter))
            {
                --iter, --count;

                return *this;
            }
        };
    } // namespace detail

    /// Adapts a range to give both an index and the normal `operator*` result.
    /// Similar to `iter.enumerate()` in Rust.
    ///
    /// Allows code like:
    /// ```cpp
    /// for (auto [i, thing] : enumerate(object)) {
    ///     std::cout << "object[" << i << "]: " << thing << '\n';
    /// }
    /// ```
    template <std::ranges::range T> class EnumerateAdapter
    {
    public:
        using MaybeConstRef = std::conditional_t<std::is_const_v<T>, const T&, T&>;

        EnumerateAdapter() noexcept = delete;

        explicit EnumerateAdapter(MaybeConstRef iter) noexcept
            : iterable_(iter)
        {}

        explicit EnumerateAdapter(const T& iter, std::size_t start) noexcept
            : iterable_(iter)
            , start_index_(start)
        {}

        EnumerateAdapter(const EnumerateAdapter&) noexcept = default;

        EnumerateAdapter(EnumerateAdapter&&) noexcept = default;

        EnumerateAdapter& operator=(const EnumerateAdapter&) noexcept = default;

        EnumerateAdapter& operator=(EnumerateAdapter&&) noexcept = default;

        ~EnumerateAdapter() noexcept = default;

        [[nodiscard]] auto begin() noexcept
        {
            return detail::EnumeratingIterator{start_index_, std::ranges::begin(iterable_)};
        }

        [[nodiscard]] auto end() noexcept { return std::ranges::end(iterable_); }

    private:
        MaybeConstRef iterable_;
        const std::size_t start_index_ = 0;
    };

    /// Turns a range into an `EnumerateAdapter` wrapping a range.
    ///
    /// # Parameters
    /// - `iterable`: The object being iterated over
    /// - `start`: The first index to use (defaults to 0, but if you start at a non-0 position you can change this)
    ///
    /// # Returns
    /// An `EnumerateAdaptor` that returns `const` objects
    [[nodiscard]] constexpr auto enumerate(const std::ranges::range auto& iterable, std::size_t start = 0)
    {
        return EnumerateAdapter<decltype(iterable)>(iterable, start);
    }

    /// Turns a range into an `EnumerateAdapter` wrapping a **mutable** range.
    ///
    /// # Parameters
    /// - `iterable`: The object being iterated over
    /// - `start`: The first index to use (defaults to 0, but if you start at a non-0 position you can change this)
    ///
    /// # Returns
    /// An `EnumerateAdaptor` that returns non-`const` objects
    [[nodiscard]] constexpr auto enumerate_mut(std::ranges::range auto& iterable, std::size_t start = 0)
    {
        return EnumerateAdapter<decltype(iterable)>(iterable, start);
    }
} // namespace zinc

#endif
