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

#ifndef ZINC_TYPES_ITERATORS_RANGE
#define ZINC_TYPES_ITERATORS_RANGE

#include <concepts>
#include <cstddef>
#include <iterator>
#include <ranges>

namespace zinc
{
    namespace detail
    {
        template <std::integral T> struct RangeIterator
        {
            using iterator_category = std::bidirectional_iterator_tag;

            [[nodiscard]] constexpr bool operator==(T other) noexcept { return count == other; }

            [[nodiscard]] constexpr bool operator==(RangeIterator<T> other) noexcept { return count == other.count; }

            T operator*() noexcept { return count; }

            constexpr RangeIterator<T> operator++(int) noexcept
            {
                const auto old_count = count;

                ++count;

                return RangeIterator<T>{old_count};
            }

            constexpr RangeIterator<T>& operator++() noexcept
            {
                ++count;

                return *this;
            }

            constexpr RangeIterator<T> operator--(int) noexcept
            {
                const auto old_count = count;

                --count;

                return RangeIterator<T>{old_count};
            }

            constexpr RangeIterator<T>& operator--() noexcept
            {
                --count;

                return *this;
            }

            std::size_t count;
        };
    } // namespace detail

    /// Range adapter that returns iterators iterators returning `i` between two values.
    ///
    /// Similar to `range(x, y)` in Python. Allows one to write code like:
    /// ```cpp
    /// for (auto i : range(5, 12)) {
    ///    std::cout << i << '\n';
    /// }
    /// ```
    template <std::integral T = SignedWord> class NumericRange
    {
    public:
        NumericRange() noexcept = default;

        NumericRange(const NumericRange&) noexcept = default;

        NumericRange(NumericRange&&) noexcept = default;

        NumericRange& operator=(const NumericRange&) noexcept = default;

        NumericRange& operator=(NumericRange&&) noexcept = default;

        constexpr NumericRange(T start, T finish) noexcept
            : begin_(start)
            , end_(finish)
        {}

        constexpr explicit NumericRange(T finish) noexcept
            : end_(finish)
        {}

        [[nodiscard]] constexpr auto begin() noexcept { return detail::RangeIterator<T>{begin_}; }

        [[nodiscard]] constexpr auto end() noexcept { return detail::RangeIterator<T>{end_}; }

    private:
        T begin_ = 0;
        T end_ = 0;
    };

    /// Gets a `NumericRange` representing [`start`, `end`).
    ///
    /// # Parameters
    /// - `start`: The number to start at
    /// - `end`: The number to end at (exclusive)
    ///
    /// # Returns
    /// A `NumericRange` that returns `SignedWord`s
    [[nodiscard]] constexpr NumericRange<> range(SignedWord start, SignedWord end)
    {
        return NumericRange<>(start, end);
    }

    /// Gets a `NumericRange` representing [`start`, `end`].
    ///
    /// # Parameters
    /// - `start`: The number to start at
    /// - `end`: The number to end at (inclusive)
    ///
    /// # Returns
    /// A `NumericRange` that returns `SignedWord`s
    [[nodiscard]] constexpr NumericRange<> range_inclusive(SignedWord start, SignedWord end)
    {
        return range(start, end + 1);
    }

    /// Gets a `NumericRange` representing [0, `end`).
    ///
    /// # Parameters
    /// - `end`: The number to end at (exclusive)
    ///
    /// # Returns
    /// A `NumericRange` that returns `SignedWord`s
    [[nodiscard]] constexpr NumericRange<> zero_to(SignedWord end) { return range(0, end); }

    /// Gets a `NumericRange` representing [`start`, `end`) with a specific type
    ///
    /// # Type Parameters
    /// - `T`: The type of number to return from the `NumericRange`
    ///
    /// # Parameters
    /// - `start`: The number to start at
    /// - `end`: The number to end at (exclusive)
    ///
    /// # Returns
    /// A `NumericRange` representing the sequence of `T`
    template <std::integral T> [[nodiscard]] constexpr NumericRange<T> range_of(T start, T end)
    {
        return NumericRange<T>(start, end);
    }
} // namespace zinc
#endif
