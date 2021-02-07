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

#ifndef ZINC_UTIL_FINALLY
#define ZINC_UTIL_FINALLY

#include <concepts>
#include <utility>

namespace zinc
{
    /// Type that encapsulates a "deferred" callback.
    ///
    /// The type holds some callable object, and when it gets destroyed,
    /// that callback is invoked. Can not be copied or re-assigned, although
    /// moving is permitted.
    ///
    /// Use in place of `goto exit` or one-off RAII objects for deferred
    /// cleanup code
    template <std::invocable Lambda> class FinallyAction
    {
    public:
        FinallyAction() = delete;

        FinallyAction(const FinallyAction&) = delete;

        FinallyAction(FinallyAction&& other) noexcept
            : data_(std::move(other.data_))
        {
            other.should_invoke_ = false;
        }

        explicit FinallyAction(Lambda&& lambda)
            : data_(std::forward<Lambda>(lambda))
        {}

        FinallyAction& operator=(const FinallyAction&) = delete;

        FinallyAction& operator=(FinallyAction&&) = delete;

        ~FinallyAction()
        {
            if (should_invoke_)
            {
                std::invoke(data_);
            }
        }

    private:
        Lambda data_;
        bool should_invoke_ = true;
    };

    /// Registers some callback that is guaranteed to happen at the end of the current scope,
    /// with exception safety and the like built in.
    ///
    /// Returns an object that will call the callback in its destructor, this object can
    /// be stored.
    ///
    /// # Parameters
    /// - `functor`: The callable to invoke whenever the returned object is destructed
    template <std::invocable Fn> FinallyAction<Fn> finally(Fn&& functor)
    {
        return FinallyAction<Fn>(std::forward<Fn>(functor));
    }
} // namespace zinc

#endif
