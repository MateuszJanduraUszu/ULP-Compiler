// runtime.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _ULPCL_RUNTIME_HPP_
#define _ULPCL_RUNTIME_HPP_
#include <cstddef>
#include <cstdint>
#include <mjstr/string.hpp>
#include <mjstr/string_view.hpp>
#include <type_traits>

namespace mjx {
    class timer { // QPC-based timer
    public:
        timer() noexcept;
        ~timer() noexcept;

        timer(const timer&)            = delete;
        timer& operator=(const timer&) = delete;

        // returns the timer frequency
        static uint64_t frequency() noexcept;

        // restarts the timer
        void restart() noexcept;

        // calculates the time elapsed since the last restart
        uint64_t elapsed_time() const noexcept;

        // calculates the time in seconds elapsed since the last restart
        float elapsed_seconds() const noexcept;

    private:
        // returns the current QPC value
        static uint64_t _Query_qpc() noexcept;

        uint64_t _Mystart;
    };

    template <class _Fn, class... _Types>
    inline float measure_invoke_duration(_Fn&& _Func, _Types&&... _Args) {
        timer _Timer;
        _Timer.restart();
        (void) _Func(::std::forward<_Types>(_Args)...);
        return _Timer.elapsed_seconds();
    }

    struct report_counters {
        size_t errors   = 0;
        size_t warnings = 0;
    };

    template <class... _Types>
    inline void _Report_warning(report_counters& _Counters, const unicode_string_view _Fmt, const _Types&... _Args) {
        ++_Counters.warnings;
        clog(_Fmt, _Args...);
    }

    template <class... _Types>
    inline void _Report_error(report_counters& _Counters, const unicode_string_view _Fmt, const _Types&... _Args) {
        ++_Counters.errors;
        clog(_Fmt, _Args...);
    }

    struct _Local_date {
        uint8_t _Day;
        uint8_t _Month;
        uint16_t _Year;
    };

    struct _Local_time {
        uint8_t _Hour;
        uint8_t _Minute;
        uint8_t _Second;
    };

    _Local_date _Get_local_date() noexcept;
    _Local_time _Get_local_time() noexcept;

    template <class _Elem>
    inline void _Write_day_or_month_and_dot_to_buffer(_Elem* const _Buf, const uint8_t _Day_or_month) noexcept {
        // assumes that _Buf will fit two digits + dot
        if (_Day_or_month < 10) { // write one digit, prepend with a zero
            _Buf[0] = static_cast<_Elem>('0');
            _Buf[1] = static_cast<_Elem>(_Day_or_month) + '0';
        } else { // write two digits
            _Buf[0] = static_cast<_Elem>((_Day_or_month / 10) + '0');
            _Buf[1] = static_cast<_Elem>((_Day_or_month % 10) + '0');
        }

        _Buf[2] = static_cast<_Elem>('.'); // write
    }

    template <class _Elem>
    inline void _Write_year_to_buffer(_Elem* const _Buf, const uint16_t _Year) noexcept {
        // assumes that _Buf will fit four digits
        if (_Year < 10) { // write one digit, prepend with three zeros
            _Buf[0] = static_cast<_Elem>('0');
            _Buf[1] = static_cast<_Elem>('0');
            _Buf[2] = static_cast<_Elem>('0');
            _Buf[3] = static_cast<_Elem>(_Year + '0');
        } else if (_Year < 100) { // write two digits, prepend with two zeros
            _Buf[0] = static_cast<_Elem>('0');
            _Buf[1] = static_cast<_Elem>('0');
            _Buf[2] = static_cast<_Elem>(((_Year % 100) / 10) + '0');
            _Buf[3] = static_cast<_Elem>((_Year % 10) + '0');
        } else if (_Year < 1000) { // write three digits, prepend with a zero
            _Buf[0] = static_cast<_Elem>('0');
            _Buf[1] = static_cast<_Elem>(((_Year % 1000) / 100) + '0');
            _Buf[2] = static_cast<_Elem>(((_Year % 100) / 10) + '0');
            _Buf[3] = static_cast<_Elem>((_Year % 10) + '0');
        } else { // write four digits
            _Buf[0] = static_cast<_Elem>((_Year / 1000) + '0');
            _Buf[1] = static_cast<_Elem>(((_Year % 1000) / 100) + '0');
            _Buf[2] = static_cast<_Elem>(((_Year % 100) / 10) + '0');
            _Buf[3] = static_cast<_Elem>((_Year % 10) + '0');
        }
    }

    template <class _Elem>
    inline void _Write_time_component_to_buffer(
        _Elem* const _Buf, const uint8_t _Component, const bool _Write_colon) noexcept {
        // assumes that _Buf will fit two digits + optionally colon
        if (_Component < 10) { // write one digit, prepend with a zero
            _Buf[0] = static_cast<_Elem>('0');
            _Buf[1] = static_cast<_Elem>(_Component) + '0';
        } else { // write two digits
            _Buf[0] = static_cast<_Elem>((_Component / 10) + '0');
            _Buf[1] = static_cast<_Elem>((_Component % 10) + '0');
        }

        if (_Write_colon) { // write a colon that will serve as a connector for the two time components
            _Buf[2] = static_cast<_Elem>(':');
        }
    }

    template <class _Elem>
    inline string<_Elem> get_current_date() {
        constexpr size_t _Str_size = 10; // always ten characters ('dd.mm.yyyy')
        _Elem _Buf[_Str_size + 1]  = {_Elem{0}}; // must fit formatted date + null-terminator
        const _Local_date _Date    = _Get_local_date();
        _Write_day_or_month_and_dot_to_buffer(_Buf, _Date._Day);
        _Write_day_or_month_and_dot_to_buffer(_Buf + 3, _Date._Month); // skip 'dd.'
        _Write_year_to_buffer(_Buf + 6, _Date._Year); // skip 'dd.mm.'
        return string<_Elem>{_Buf, _Str_size};
    }

    template <class _Elem>
    inline string<_Elem> get_current_time() {
        constexpr size_t _Str_size = 8; // always eight characters ('hh:mm:ss')
        _Elem _Buf[_Str_size + 1]  = {_Elem{0}}; // must fit formatted time + null-terminator
        const _Local_time _Time    = _Get_local_time();
        _Write_time_component_to_buffer(_Buf, _Time._Hour, true);
        _Write_time_component_to_buffer(_Buf + 3, _Time._Minute, true); // skip 'hh:'
        _Write_time_component_to_buffer(_Buf + 6, _Time._Second, false); // skip 'hh:mm:'
        return string<_Elem>{_Buf, _Str_size};
    }
} // namespace mjx

#endif // _ULPCL_RUNTIME_HPP_