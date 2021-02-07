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

#ifndef ZINC_TYPES_CUSTOMIZATION
#define ZINC_TYPES_CUSTOMIZATION

namespace zinc
{
    /// Customization point to enable use of custom Char types, used in various
    /// string concepts/types. If `value` is true, `std::char_traits<T>` is
    /// expected to be usable
    template <typename T> struct ValidCharType
    {
        constexpr static bool value = false;
    };
} // namespace zinc

#endif