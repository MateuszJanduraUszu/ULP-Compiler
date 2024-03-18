// version.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _ULPCL_VERSION_HPP_
#define _ULPCL_VERSION_HPP_

// these macros define version value in all encodings to avoid unnecessary conversion at runtime
#define _BYTE_ULPCL_VERSION    reinterpret_cast<const byte_t*>("1.0.0")
#define _UTF8_ULPCL_VERSION    "1.0.0"
#define _UNICODE_ULPCL_VERSION L"1.0.0"

#define _BYTE_ULPCL_COMPILATION_DATE    reinterpret_cast<const byte_t*>("18.03.2024")
#define _UTF8_ULPCL_COMPILATION_DATE    "18.03.2024"
#define _UNICODE_ULPCL_COMPILATION_DATE L"18.03.2024"
#endif // _ULPCL_VERSION_HPP_