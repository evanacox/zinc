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

#ifndef ZINC_IO_CONCEPTS
#define ZINC_IO_CONCEPTS

#include <iostream>

namespace zinc
{
    /// Checks if a type is actually outputtable to an `std::ostream`
    template <typename T, typename CharT = char>
    concept Printable = requires(std::basic_ostream<CharT> &stream, const T &t)
    {
        // clang-format off
        { stream << t } -> std::same_as<decltype(stream)>;
        // clang-format on
    };
} // namespace zinc

#endif
