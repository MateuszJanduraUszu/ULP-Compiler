// dispatcher.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _ULPCL_DISPATCHER_HPP_
#define _ULPCL_DISPATCHER_HPP_
#include <atomic>
#include <cstddef>
#include <mjfs/path.hpp>
#include <mjmem/smart_pointer.hpp>
#include <mjsync/thread_pool.hpp>
#include <mjsync/waitable_event.hpp>

namespace mjx {
    struct compilation_counters {
        size_t succeeded = 0;
        size_t failed    = 0;
    };

    class __declspec(novtable) _Dispatcher_base { // base class for all dispatchers
    public:
        _Dispatcher_base() noexcept;
        virtual ~_Dispatcher_base() noexcept;

        _Dispatcher_base(const _Dispatcher_base&)            = delete;
        _Dispatcher_base& operator=(const _Dispatcher_base&) = delete;

        // dispatches compilation of the specified input file
        virtual void _Dispatch(const path& _Target) = 0;

        // waits until the compilation is done
        virtual void _Wait_for_completion() noexcept = 0;

        // returns the compilation counters
        virtual compilation_counters _Counters() const noexcept = 0;
    };

    class _Sequential_dispatcher : public _Dispatcher_base { // dispatches compilation on a single thread
    public:
        _Sequential_dispatcher() noexcept;
        ~_Sequential_dispatcher() noexcept override;

        _Sequential_dispatcher(const _Sequential_dispatcher&)            = delete;
        _Sequential_dispatcher& operator=(const _Sequential_dispatcher&) = delete;

        // dispatches compilation of the specified input file
        void _Dispatch(const path& _Target) override;

        // waits until the compilation is done
        void _Wait_for_completion() noexcept override;
    
        // returns the compilation counters
        compilation_counters _Counters() const noexcept override;

    private:
        compilation_counters _Myctrs;
    };

    class _Parallel_dispatcher : public _Dispatcher_base { // dispatches compilation on multiple threads
    public:
        _Parallel_dispatcher();
        ~_Parallel_dispatcher() noexcept override;

        _Parallel_dispatcher(const _Parallel_dispatcher&)            = delete;
        _Parallel_dispatcher& operator=(const _Parallel_dispatcher&) = delete;

        // dispatches compilation of the specified input file
        void _Dispatch(const path& _Target) override;

        // waits until the compilation is done
        void _Wait_for_completion() noexcept override;

        // returns the compilation counters
        compilation_counters _Counters() const noexcept override;

    private:
        struct _Atomic_counters { // atomic counters to ensure proper synchronization
            ::std::atomic<size_t> _Total     = 0;
            ::std::atomic<size_t> _Succeeded = 0;
            ::std::atomic<size_t> _Failed    = 0;
        };

        thread_pool _Mypool;
        waitable_event _Myevent;
        _Atomic_counters _Myctrs;
    };

    class compilation_dispatcher { // dispatches compilation of the input files
    public:
        compilation_dispatcher();
        ~compilation_dispatcher() noexcept;

        // dispatches compilation of the specified input file
        void dispatch(const path& _Target);

        // waits until the compilation is done
        void wait_for_completion() noexcept;

        // returns the compilation counters
        compilation_counters counters() const noexcept;

    private:
        // creates the compilation dispatcher
        static _Dispatcher_base* _Create();

        unique_smart_ptr<_Dispatcher_base> _Myimpl;
    };
} // namespace mjx

#endif // _ULPCL_DISPATCHER_HPP_