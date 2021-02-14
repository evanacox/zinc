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

#include "zinc/zinc.h"
#include <iostream>

using namespace std::chrono_literals;
using zinc::detail::RawHashSet;

template <> struct zinc::EqualTo<int>
{
    constexpr bool operator()(int lhs, std::string_view) const noexcept { return lhs == 3; }

    constexpr bool operator()(std::string_view, int rhs) const noexcept { return rhs == 3; }

    constexpr bool operator()(int lhs, int rhs) const noexcept { return lhs == rhs; }
};

using Traits = zinc::detail::
    DefaultSetTraits<std::string, std::hash<std::string>, zinc::EqualTo<std::string>, std::allocator<std::string>>;

auto insert(RawHashSet<Traits>& set, std::string n)
{
    const auto result = set.insert(n);

    std::cout << "inserted '" << n << "'. load factor: " << set.load_factor() << '\n';

    return result;
}

int main()
{
    RawHashSet<Traits> set(5);

    insert(set, "lol");
    insert(set, "hello");
    insert(set, "thing");

    for (auto i : set)
    {
        std::cout << i << '\n';
    }

    const auto [success, _] = insert(set, "12");

    if (success)
    {
        std::cout << "lol wtf\n";
    }

    std::cout << set.contains("12");

    auto new_set = set;

    for (auto i : new_set)
    {
        std::cout << i << '\n';
    }
}