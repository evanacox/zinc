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

#ifndef ZINC_UTIL_OPTIONS_PARSER
#define ZINC_UTIL_OPTIONS_PARSER

#include "zinc/types/concepts.h"
#include "zinc/util/options/args.h"
#include <cstddef>
#include <optional>
#include <string>
#include <type_traits>

namespace zinc
{
    class OptParser
    {
    public:
        OptParser() = default;

        template <ArgumentType T> OptParser& add_arg(Arg<T> argument)
        {
            //

            return *this;
        }

    private:
        std::optional<std::string> help_message_;
    };
} // namespace zinc

#endif