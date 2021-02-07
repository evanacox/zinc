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

#ifndef ZINC_SOURCE_LOGGER_TYPE
#define ZINC_SOURCE_LOGGER_TYPE

#include <iostream>
#include <string>

class ConstructLogger
{
public:
    ConstructLogger(std::string str)
        : _name(std::move(str))
    {
        std::cout << "ConstructLogger::ConstructLogger " << _name << '\n';
    }

    ConstructLogger(const ConstructLogger &other)
        : _name(other._name)
    {
        std::cout << "ConstructLogger::ConstructLogger(const ConstructLogger&) " << _name << '\n';
    }

    ConstructLogger(ConstructLogger &&other) noexcept
        : _name(std::move(other._name))
    {
        std::cout << "ConstructLogger::ConstructLogger(ConstructLogger&&) " << _name << '\n';
    }

    ConstructLogger &operator=(const ConstructLogger &other)
    {
        if (this != &other) _name = other._name;
        std::cout << "ConstructLogger::operator=(const ConstructLogger&) " << _name << '\n';
        return *this;
    }

    ConstructLogger &operator=(ConstructLogger &&other) noexcept
    {
        if (this != &other) _name = std::move(other._name);
        std::cout << "ConstructLogger::operator=(ConstructLogger&&) " << _name << '\n';
        return *this;
    }

    ~ConstructLogger() { std::cout << "ConstructLogger::~ConstructLogger " << _name << '\n'; }

private:
    std::string _name;
};

#endif
