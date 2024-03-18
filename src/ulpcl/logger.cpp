// logger.cpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <mjmem/object_allocator.hpp>
#include <ulpcl/logger.hpp>
#include <ulpcl/program.hpp>
#include <ulpcl/tinywin.hpp>

namespace mjx {
    void _Write_unicode_console(const unicode_string_view _Str) noexcept {
        void* const _Handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
        ::WriteConsoleW(_Handle, _Str.data(),
#ifdef _M_X64
            static_cast<unsigned long>(_Str.size()),
#else // ^^^ _M_X64 ^^^ / vvv _M_IX86 vvv
            _Str.size(),
#endif // _M_X64
                nullptr, nullptr);
        ::WriteConsoleW(_Handle, L"\n", 1, nullptr, nullptr); // break the line
    }

    _Logger_base::_Logger_base() noexcept {}

    _Logger_base::~_Logger_base() noexcept {}

    _Direct_logger::_Direct_logger() noexcept {}

    _Direct_logger::~_Direct_logger() noexcept {}

    void _Direct_logger::_Write(const unicode_string_view _Msg) {
        // write message directly to the log (no buffering)
        _Write_unicode_console(_Msg);
    }

    void _Direct_logger::_Request_flush() noexcept {
        // buffering not supported, do nothing
    }

    _Buffered_logger::_Buffered_logger() noexcept : _Mybufs() {}

    _Buffered_logger::~_Buffered_logger() noexcept {}

    _Buffered_logger::_Thread_buffer::_Thread_buffer(const thread::id _Id) noexcept
        : _Owner(_Id), _Queue() {}

    size_t _Buffered_logger::_Find_buffer(const thread::id _Id) const noexcept {
        // assumes that the lock has been acquired
        for (size_t _Idx = 0; _Idx < _Mybufs.size(); ++_Idx) {
            if (_Mybufs[_Idx]._Owner == _Id) { // searched buffer found
                return _Idx;
            }
        }

        return static_cast<size_t>(-1); // not found
    }

    void _Buffered_logger::_Flush_buffer(const size_t _Idx) noexcept {
        // assumes that the lock has been acquired and _Idx points to an existing buffer
        for (const unicode_string& _Msg : _Mybufs[_Idx]._Queue) {
            _Write_unicode_console(_Msg);
        }

        _Mybufs.erase(_Mybufs.begin() + _Idx); // unregister buffer
    }

    void _Buffered_logger::_Write(const unicode_string_view _Msg) {
        // write message to the log (enqueue to the internal buffer)
        lock_guard _Guard(_Mylock);
        const thread::id _Id = ::mjx::current_thread_id();
        const size_t _Idx    = _Find_buffer(_Id);
        if (_Idx != static_cast<size_t>(-1)) { // buffer is registered, use it
            _Mybufs[_Idx]._Queue.push_back(_Msg);
        } else { // buffer isn't registered, register it now
            _Mybufs.push_back(_Thread_buffer(_Id));
            _Mybufs.back()._Queue.push_back(_Msg); // enqueue message
        }
    }

    void _Buffered_logger::_Request_flush() noexcept {
        lock_guard _Guard(_Mylock);
        const size_t _Idx = _Find_buffer(::mjx::current_thread_id());
        if (_Idx != static_cast<size_t>(-1)) { // buffer is registered, flush it
            _Flush_buffer(_Idx);
        }
    }

    compilation_logger::compilation_logger() noexcept : _Myimpl(nullptr) {}

    compilation_logger::~compilation_logger() noexcept {
        shutdown();
    }

    compilation_logger& compilation_logger::current() noexcept {
        static compilation_logger _Logger;
        return _Logger;
    }

    bool compilation_logger::is_active() const noexcept {
        return _Myimpl != nullptr;
    }

    void compilation_logger::startup() {
        if (!_Myimpl) {
            if (program_options::current().threads > 0) { // startup buffered logger
                _Myimpl.reset(::mjx::create_object<_Buffered_logger>());
            } else { // startup direct logger
                _Myimpl.reset(::mjx::create_object<_Direct_logger>());
            }
        }
    }

    void compilation_logger::shutdown() noexcept {
        _Myimpl.reset();
    }

    void notify_compilation_finish() noexcept {
        // request a buffer flush if the logger is active and buffered
        compilation_logger& _Logger = compilation_logger::current();
        if (_Logger.is_active()) { // logger is active, request flush
            _Logger._Myimpl->_Request_flush();
        }
    }
} // namespace mjx