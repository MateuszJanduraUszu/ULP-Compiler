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

    struct _Local_time {
        uint8_t _Hour;
        uint8_t _Minute;
        uint8_t _Second;
    };

    _Local_time _Get_local_time() noexcept;
    void _Write_time_component_to_buffer(
        wchar_t* const _Buf, const uint8_t _Component, const bool _Write_colon) noexcept;
    
    unicode_string get_current_time();
} // namespace mjx

#endif // _ULPCL_RUNTIME_HPP_