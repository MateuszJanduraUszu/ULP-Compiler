// dispatcher.cpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <mjsync/async.hpp>
#include <ulpcl/compiler.hpp>
#include <ulpcl/dispatcher.hpp>
#include <ulpcl/program.hpp>

namespace mjx {
    _Dispatcher_base::_Dispatcher_base() noexcept {}

    _Dispatcher_base::~_Dispatcher_base() noexcept {}

    _Sequential_dispatcher::_Sequential_dispatcher() noexcept : _Myctrs() {}

    _Sequential_dispatcher::~_Sequential_dispatcher() noexcept {}

    void _Sequential_dispatcher::_Dispatch(const path& _Target) {
        // run the compilation on this thread
        if (compile_input_file(_Target)) { // capture success
            ++_Myctrs.succeeded;
        } else { // capture failure
            ++_Myctrs.failed;
        }
    }

    void _Sequential_dispatcher::_Wait_for_completion() noexcept {
        // wait not supported in sequential dispatcher, do nothing
    }

    compilation_counters _Sequential_dispatcher::_Counters() const noexcept {
        return _Myctrs;
    }

    _Parallel_dispatcher::_Parallel_dispatcher()
        : _Mypool(program_options::current().threads), _Myevent(), _Myctrs() {}

    _Parallel_dispatcher::~_Parallel_dispatcher() noexcept {}

    void _Parallel_dispatcher::_Dispatch(const path& _Target) {
        // schedule compilation, it will be executed by one of the thread-pool's threads
        ::mjx::async(_Mypool,
            [this, _Target] {
                if (compile_input_file(_Target)) { // capture success
                    ++_Myctrs._Succeeded;
                } else { // capture failure
                    ++_Myctrs._Failed;
                }

                ++_Myctrs._Total;
                _Myevent.notify();
            }
        );
    }

    void _Parallel_dispatcher::_Wait_for_completion() noexcept {
        const size_t _Total = program_options::current().input_files.size();
        while (_Myctrs._Total.load(::std::memory_order_relaxed) < _Total) {
            _Myevent.wait_and_reset();
        }
    }

    compilation_counters _Parallel_dispatcher::_Counters() const noexcept {
        return compilation_counters{
            _Myctrs._Succeeded.load(::std::memory_order_relaxed),
            _Myctrs._Failed.load(::std::memory_order_relaxed)
        };
    }

    compilation_dispatcher::compilation_dispatcher() : _Myimpl(_Create()) {}

    compilation_dispatcher::~compilation_dispatcher() noexcept {}

    _Dispatcher_base* compilation_dispatcher::_Create() {
        if (program_options::current().threads > 0) { // create parallel dispatcher
            return ::mjx::create_object<_Parallel_dispatcher>();
        } else { // create sequential dispatcher
            return ::mjx::create_object<_Sequential_dispatcher>();
        }
    }

    void compilation_dispatcher::dispatch(const path& _Target) {
        if (_Myimpl) { // dispatcher active, dispatch the compilation
            _Myimpl->_Dispatch(_Target);
        }
    }

    void compilation_dispatcher::wait_for_completion() noexcept {
        if (_Myimpl) { // dispatcher active, wait for the completion
            _Myimpl->_Wait_for_completion();
        }
    }

    compilation_counters compilation_dispatcher::counters() const noexcept {
        return _Myimpl ? _Myimpl->_Counters() : compilation_counters{};
    }
} // namespace mjx