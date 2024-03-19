// keyword.cpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <ulpcl/keyword.hpp>

namespace mjx {
    keyword parse_keyword(const byte_string_view _Keyword) noexcept {
        return parse_keyword(utf8_string_view{reinterpret_cast<const char*>(_Keyword.data()), _Keyword.size()});
    }

    keyword parse_keyword(const utf8_string_view _Keyword) noexcept {
        if (_Keyword == "@language") {
            return keyword::language;
        } else if (_Keyword == "@lcid") {
            return keyword::lcid;
        } else if (_Keyword == "@meta") {
            return keyword::meta;
        } else if (_Keyword == "@content") {
            return keyword::content;
        } else if (_Keyword == "@group") {
            return keyword::group;
        } else {
            return keyword::none;
        }
    }
} // namespace mjx