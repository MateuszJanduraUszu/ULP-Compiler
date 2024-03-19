// keyword.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _ULPCL_KEYWORD_HPP_
#define _ULPCL_KEYWORD_HPP_
#include <mjstr/string_view.hpp>

namespace mjx {
    enum class keyword : unsigned char {
        none,
        language,
        lcid,
        meta,
        content,
        group
    };

    keyword parse_keyword(const byte_string_view _Keyword) noexcept;
    keyword parse_keyword(const utf8_string_view _Keyword) noexcept;
} // namespace mjx

#endif // _ULPCL_KEYWORD_HPP_