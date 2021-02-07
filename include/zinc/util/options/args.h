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

#ifndef ZINC_UTIL_OPTIONS_ARGS
#define ZINC_UTIL_OPTIONS_ARGS

#include "zinc/types/concepts.h"
#include <cstddef>
#include <optional>
#include <string>
#include <type_traits>

namespace zinc
{
    namespace detail
    {
        template <typename T> struct ValidArgTypeWithoutVec : std::false_type
        {};

        template <Fundamental T>
        requires(!OneOf<T, void, std::nullptr_t>) struct ValidArgTypeWithoutVec<T> : std::true_type
        {};

        template <> struct ValidArgTypeWithoutVec<std::string> : std::true_type
        {};

        template <typename T> struct ValidArgType : std::false_type
        {};

        template <Satisfies<ValidArgTypeWithoutVec> T> struct ValidArgType<T> : std::true_type
        {};

        template <Satisfies<ValidArgTypeWithoutVec> T> struct ValidArgType<std::vector<T>> : std::true_type
        {};
    } // namespace detail

    /// Checks that the type T fulfills one of the following:
    ///   - Is a fundamental type and NOT `void` or `std::nullptr_t`
    ///   - Is `std::string`
    ///   - Is a `std::vector<U>`, where `U` is a type that fulfills `ArgumentType<U>`
    template <typename T> concept ArgumentType = Satisfies<T, detail::ValidArgType>;

    template <ArgumentType T> class Arg
    {
    public:
        Arg() = delete;

        Arg(std::string_view name)
            : name_(name)
        {}

        Arg(const Arg&) = delete;

        Arg(Arg&&) noexcept = default;

        Arg& operator=(const Arg&) = delete;

        Arg& operator=(Arg&&) = delete;

        ~Arg() = default;

        Arg&& description(std::string_view help_message) noexcept
        {
            help_ = help_message;

            return std::move(*this);
        }

        Arg&& value_name(std::string_view value_name) noexcept
        {
            value_ = value_name;

            return std::move(*this);
        }

        Arg&& short_name(char short_name) noexcept
        {
            short_ = short_name;

            return std::move(*this);
        }

        Arg&& flag(bool state) noexcept
        {
            flag_ = state;

            return std::move(*this);
        }

    private:
        std::string_view name_;                                // -n, --{name}
        bool flag_ = true;                                     // whether or not the variable is just an on/off toggle
        std::optional<std::string_view> help_ = std::nullopt;  // -o, --out <VALUE>        Sets the output file.
        std::optional<std::string_view> value_ = std::nullopt; // -o, --out <VALUE>
        std::optional<char> short_ = std::nullopt;             // -{short}, --long
        std::optional<T> default_value_ = std::nullopt;        // a default value of some sort
    };

    Arg arg = Arg<std::string>("opt")
                  .short_name('o')
                  .value_name("OPT-LEVEL")
                  .flag(false)
                  .description("The optimization level to use");
} // namespace zinc

#endif