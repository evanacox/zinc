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

#ifndef ZINC_UTIL_DEFER
#define ZINC_UTIL_DEFER

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
    template <std::invocable Lambda> class DeferredAction
    {
    public:
        DeferredAction() = delete;

        DeferredAction(const DeferredAction&) = delete;

        DeferredAction(DeferredAction&& other) noexcept
            : data_(std::move(other.data_))
            , should_invoke_(std::exchange(other.should_invoke_, false))
        {}

        explicit DeferredAction(Lambda&& lambda)
            : data_(std::forward<Lambda>(lambda))
        {}

        DeferredAction& operator=(const DeferredAction&) = delete;

        DeferredAction& operator=(DeferredAction&&) = delete;

        ~DeferredAction()
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
    /// with exception safety and the like built-in. Useful for RAII semantics without needing
    /// to make one-off RAII types.
    ///
    /// Returns an object that will call the callback in its destructor, this object can
    /// be stored.
    ///
    /// # Example
    /// ```cpp
    /// {
    ///     auto* foo = some_c_code();
    ///     auto _ = defer([&] { some_c_code_cleanup(foo); })
    ///
    ///     // use `foo`
    /// }
    /// ```
    ///
    /// # Parameters
    /// - `functor`: The callable to invoke whenever the returned object is destructed
    ///
    /// # Returns
    /// Returns an object encapsulating the action, that calls the action in its destructor.
    template <std::invocable Fn> DeferredAction<Fn> defer(Fn&& functor)
    {
        return DeferredAction<Fn>(std::forward<Fn>(functor));
    }
} // namespace zinc

#endif
