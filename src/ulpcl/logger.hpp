// logger.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _ULPCL_LOGGER_HPP_
#define _ULPCL_LOGGER_HPP_
#include <cstdio>
#include <mjmem/smart_pointer.hpp>
#include <mjstr/string.hpp>
#include <mjstr/string_view.hpp>
#include <mjsync/shared_resource.hpp>
#include <mjsync/thread.hpp>
#include <ulpcl/utils.hpp>

namespace mjx {
    void _Write_unicode_console(const unicode_string_view _Str) noexcept;

    template <class... _Types>
    inline unicode_string _Format_string(const unicode_string_view _Fmt, const _Types&... _Args) {
        if constexpr (sizeof...(_Types) > 0) { // format the string with the given arguments
            const size_t _Buf_size = static_cast<size_t>(::swprintf(nullptr, 0, _Fmt.data(), _Args...));
            unicode_string _Buf(_Buf_size, L'\0');
            ::swprintf(_Buf.data(), _Buf_size + 1, _Fmt.data(), _Args...);
            return ::std::move(_Buf);
        } else { // arguments not specified, don't format the string
            return _Fmt;
        }
    }

    template <class... _Types>
    inline void rtlog(const unicode_string_view _Fmt, const _Types&... _Args) {
        // write formatted message to the runtime log
        _Write_unicode_console(_Format_string(_Fmt, _Args...));
    }
    
    class __declspec(novtable) _Logger_base { // base class for all loggers
    public:
        _Logger_base() noexcept;
        virtual ~_Logger_base() noexcept;

        _Logger_base(const _Logger_base&)            = delete;
        _Logger_base& operator=(const _Logger_base&) = delete;
    
        // writes message to the logger
        virtual void _Write(const unicode_string_view _Msg) = 0;
        
        // requests buffer flush
        virtual void _Request_flush() noexcept = 0;
    };

    class _Direct_logger : public _Logger_base {
    public:
        _Direct_logger() noexcept;
        ~_Direct_logger() noexcept override;

        _Direct_logger(const _Direct_logger&)            = delete;
        _Direct_logger& operator=(const _Direct_logger&) = delete;

        // writes message to the logger
        void _Write(const unicode_string_view _Msg) override;

        // requests buffer flush (does nothing)
        void _Request_flush() noexcept override;
    };

    class _Buffered_logger : public _Logger_base {
    public:
        _Buffered_logger() noexcept;
        ~_Buffered_logger() noexcept override;

        _Buffered_logger(const _Buffered_logger&)            = delete;
        _Buffered_logger& operator=(const _Buffered_logger&) = delete;
    
        // writes message to the logger
        void _Write(const unicode_string_view _Msg) override;

        // requests buffer flush
        void _Request_flush() noexcept override;

    private:
        struct _Thread_buffer {
            thread::id _Owner;
            vector<unicode_string> _Queue;
        
            explicit _Thread_buffer(const thread::id _Id) noexcept;
        };

        // returns the index of the buffer associated with the specified owner
        static size_t _Find_buffer(vector<_Thread_buffer>& _Bufs, const thread::id _Id) noexcept;

        // fluses the specified buffer
        static void _Flush_buffer(vector<_Thread_buffer>& _Bufs, const size_t _Idx) noexcept;

        shared_resource<vector<_Thread_buffer>> _Mybufs;
    };

    class compilation_logger { // logger used at compilation time
    public:
        ~compilation_logger() noexcept;

        compilation_logger(const compilation_logger&)            = delete;
        compilation_logger& operator=(const compilation_logger&) = delete;

        // returns the global instance of the compilation logger
        static compilation_logger& current() noexcept;

        // checks if the compilation logger is active
        bool is_active() const noexcept;

        // startups the compilation logger
        void startup();

        // shutdowns the compilation logger
        void shutdown() noexcept;

        template <class... _Types>
        void write(const unicode_string_view _Fmt, const _Types&... _Args) {
            if (_Myimpl) {
                _Myimpl->_Write(_Format_string(_Fmt, _Args...));
            }
        }

    private:
        compilation_logger() noexcept;

        friend void notify_compilation_finish() noexcept;

        unique_smart_ptr<_Logger_base> _Myimpl;
    };

    template <class... _Types>
    inline void clog(const unicode_string_view _Fmt, const _Types&... _Args) {
        // write formatted message to the compilation log
        compilation_logger& _Logger = compilation_logger::current();
        if (_Logger.is_active()) {
            _Logger.write(_Fmt, _Args...);
        }
    }

    void notify_compilation_finish() noexcept;
} // namespace mjx

#endif // _ULPCL_LOGGER_HPP_