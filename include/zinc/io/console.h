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

#ifndef ZINC_IO_PRINT
#define ZINC_IO_PRINT

#include "zinc/io/concepts.h"
#include <iostream>
#include <utility>

namespace zinc
{
    template <typename... Args> requires(Printable<Args> &&...) void sprint(std::ostream &out, Args &&...args)
    {
        (out << ... << args);
    }

    template <typename... Args> requires(Printable<Args> &&...) void print(Args &&...args)
    {
        return sprint(std::cout, std::forward<Args>(args)...);
    }

    template <typename... Args> requires(Printable<Args> &&...) void println(Args &&...args)
    {
        return print(std::forward<Args>(args)..., '\n');
    }
} // namespace zinc

#endif
