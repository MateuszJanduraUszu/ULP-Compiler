// utils.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _ULPCL_UTILS_HPP_
#define _ULPCL_UTILS_HPP_
#include <mjmem/object_allocator.hpp>
#include <mjstr/string.hpp>
#include <mjstr/string_view.hpp>
#include <type_traits>
#include <vector>

// this macro converts a narrow string literal to a wide one without any conversion
#define _XWIDEN(_Str) L##_Str
#define _WIDEN(_Str)  _XWIDEN(_Str)

namespace mjx {
    template <class _Ty>
    using vector = ::std::vector<_Ty, object_allocator<_Ty>>;

    enum class _Bom_kind : unsigned char {
        _None,
        _Utf8,
        _Utf16_le,
        _Utf16_be,
        _Utf32_le,
        _Utf32_be
    };

    struct _Bom {
        _Bom_kind _Kind  = _Bom_kind::_None;
        byte_t _Bytes[4] = {'\0'}; // at most 4 bytes
        size_t _Size     = 0;
    };

    struct _Bom_detector {
        static constexpr _Bom _Utf8     = {_Bom_kind::_Utf8, {0xEF, 0xBB, 0xBF}, 3};
        static constexpr _Bom _Utf16_le = {_Bom_kind::_Utf16_le, {0xFF, 0xFE}, 2};
        static constexpr _Bom _Utf16_be = {_Bom_kind::_Utf16_be, {0xFE, 0xFF}, 2};
        static constexpr _Bom _Utf32_le = {_Bom_kind::_Utf32_le, {0xFF, 0xFE, 0x00, 0x00}, 4};
        static constexpr _Bom _Utf32_be = {_Bom_kind::_Utf32_be, {0x00, 0x00, 0xFE, 0xFF}, 4};
        
        // Note: The order of BOMs in the _Known_boms array is important. Placing _Utf32_le
        //       before _Utf16_le ensures correct detection, as both BOMs start with 'FF FE'.
        static constexpr _Bom _Known_boms[5] = {_Utf8, _Utf32_le, _Utf32_be, _Utf16_le, _Utf16_be};

        static _Bom _Detect(const byte_string_view _Bytes) noexcept {
            const _Bom* _First      = _Known_boms;
            const _Bom* const _Last = _First + ::std::size(_Known_boms);
            for (; _First != _Last; ++_First) {
                if (_Bytes.starts_with(byte_string_view{_First->_Bytes, _First->_Size})) {
                    return *_First;
                }
            }

            return _Bom{};
        }
    };

    template <class _To, class _From>
    inline string<_To> _Fast_str_cvt(const string_view<_From> _Str) {
        string<_To> _Cvt_str;
        _Cvt_str.reserve(_Str.size());
        for (const _From _Ch : _Str) {
            _Cvt_str.push_back(static_cast<_To>(_Ch)); // no encoding conversion is performed
        }

        return ::std::move(_Cvt_str);
    }

    template <class _To, class _From>
    inline string<_To> _Fast_str_cvt(const string<_From>& _Str) {
        string<_To> _Cvt_str;
        _Cvt_str.reserve(_Str.size());
        for (const _From _Ch : _Str) {
            _Cvt_str.push_back(static_cast<_To>(_Ch)); // no encoding conversion is performed
        }

        return ::std::move(_Cvt_str);
    }
} // namespace mjx

#endif // _ULPCL_UTILS_HPP_