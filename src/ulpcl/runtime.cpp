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

    _Local_date _Get_local_date() noexcept {
        SYSTEMTIME _Sys_time;
        ::GetLocalTime(&_Sys_time);
        return _Local_date{
            static_cast<uint8_t>(_Sys_time.wDay), static_cast<uint8_t>(_Sys_time.wMonth), _Sys_time.wYear};
    }

    _Local_time _Get_local_time() noexcept {
        SYSTEMTIME _Sys_time;
        ::GetLocalTime(&_Sys_time);
        return _Local_time{static_cast<uint8_t>(_Sys_time.wHour),
            static_cast<uint8_t>(_Sys_time.wMinute), static_cast<uint8_t>(_Sys_time.wSecond)};
    }
} // namespace mjx