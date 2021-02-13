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

using Traits = zinc::detail::DefaultSetTraits<int, std::hash<int>, std::equal_to<>, std::allocator<int>>;

int main()
{
    zinc::detail::RawHashSet<Traits> set(5);
    set.insert(1);
    set.insert(5);
    set.insert(3);
    set.insert(6);

    for (auto i : set)
    {
        std::cout << i << '\n';
    }
}