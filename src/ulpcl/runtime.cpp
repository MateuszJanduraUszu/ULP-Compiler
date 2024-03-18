// runtime.cpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <ulpcl/runtime.hpp>
#include <ulpcl/tinywin.hpp>

namespace mjx {
    timer::timer() noexcept : _Mystart(0) {}

    timer::~timer() noexcept {}

    uint64_t timer::_Query_qpc() noexcept {
        LARGE_INTEGER _Large = {0};
        ::QueryPerformanceCounter(&_Large); // never fails on Windows XP and later
        return static_cast<uint64_t>(_Large.QuadPart);
    }

    uint64_t timer::frequency() noexcept {
        // Note: The frequency of the performance counter is fixed at system boot
        //       and is consistent across all processors.
        static const uint64_t _Freq = []() noexcept -> uint64_t {
            LARGE_INTEGER _Large = {0};
            ::QueryPerformanceFrequency(&_Large); // never fails on Windows XP and later
            return static_cast<uint64_t>(_Large.QuadPart);
        }();
        return _Freq;
    }

    void timer::restart() noexcept {
        _Mystart = _Query_qpc();
    }

    uint64_t timer::elapsed_time() const noexcept {
        return _Query_qpc() - _Mystart;
    }

    float timer::elapsed_seconds() const noexcept {
        static const float _Freq = static_cast<float>(frequency());
        return static_cast<float>(elapsed_time()) / _Freq;
    }

    _Local_time _Get_local_time() noexcept {
        SYSTEMTIME _Sys_time;
        ::GetLocalTime(&_Sys_time);
        return _Local_time{static_cast<uint8_t>(_Sys_time.wHour),
            static_cast<uint8_t>(_Sys_time.wMinute), static_cast<uint8_t>(_Sys_time.wSecond)};
    }

    void _Write_time_component_to_buffer(
        wchar_t* const _Buf, const uint8_t _Component, const bool _Write_colon) noexcept {
        // assumes that _Component always takes at most two digits (represents hour, minute or second)
        if (_Component < 10) { // write one digit, prepend with a zero
            _Buf[0] = L'0';
            _Buf[1] = static_cast<wchar_t>(_Component) + L'0';
        } else { // write two digits
            _Buf[0] = static_cast<wchar_t>(_Component / 10) + L'0';
            _Buf[1] = static_cast<wchar_t>(_Component % 10) + L'0';
        }

        if (_Write_colon) { // write a colon that will serve as a connector for the two time components
            _Buf[2] = L':';
        }
    }

    unicode_string get_current_time() {
        constexpr size_t _Str_size  = 8; // always eight characters ('hh:mm:ss')
        wchar_t _Buf[_Str_size + 1] = {L'\0'}; // must fit serialized time + null-terminator
        const _Local_time _Time     = _Get_local_time();
        _Write_time_component_to_buffer(_Buf, _Time._Hour, true);
        _Write_time_component_to_buffer(_Buf + 3, _Time._Minute, true); // skip 'hh:'
        _Write_time_component_to_buffer(_Buf + 6, _Time._Second, false); // skip 'hh:mm:'
        return unicode_string{_Buf, _Str_size};
    }
} // namespace mjx