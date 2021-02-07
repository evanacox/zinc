#======----------------------------------------------------------------======#
#                                                                            #
#  Copyright 2021 Evan Cox                                                   #
#                                                                            #
#  Licensed under the Apache License, Version 2.0 (the "License");           #
#  you may not use this file except in compliance with the License.          #
#  You may obtain a copy of the License at                                   #
#                                                                            #
#     http:#www.apache.org/licenses/LICENSE-2.0                              #
#                                                                            #
#  Unless required by applicable law or agreed to in writing, software       #
#  distributed under the License is distributed on an "AS IS" BASIS,         #
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  #
#  See the License for the specific language governing permissions and       #
#  limitations under the License.                                            #
#                                                                            #
#======----------------------------------------------------------------======#

cmake_minimum_required(VERSION 3.15)
project(libzinc)

function(enable_warnings)
    if (MSVC)
        set(WARNING_OPTIONS "/W4 /WX")
    else ()
        set(WARNING_OPTIONS "-Wall -Wextra -Wpedantic -Wimplicit-fallthrough -Wshadow -Werror")
    endif ()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_OPTIONS}" PARENT_SCOPE)
endfunction()

function(force_ansi_color_output)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always" PARENT_SCOPE)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics" PARENT_SCOPE)
    endif ()

    message(STATUS "Enabled colored output")
endfunction()

function(enable_cxx_properties_for_target target)
    target_include_directories(${target} PUBLIC ${CMAKE_SOURCE_DIR}/include)
    set_target_properties(${target} PROPERTIES CXX_STANDARD 20 CXX_EXTENSIONS OFF)
endfunction()

option(ZINC_DEVBUILD "Whether or not to build tests/benchmarks/build with -Wall -Wextra -Wpedantic -Werror and the like" OFF)

if (ZINC_DEVBUILD)
    enable_warnings()
    force_ansi_color_output()
    add_subdirectory(tests)
    add_subdirectory(benchmark)
endif ()

add_subdirectory(src)
