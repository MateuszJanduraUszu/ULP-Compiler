// symbol_file.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _ULPCL_SYMBOL_FILE_HPP_
#define _ULPCL_SYMBOL_FILE_HPP_
#include <cstdint>
#include <mjfs/file.hpp>
#include <mjfs/file_stream.hpp>
#include <mjfs/api.hpp>
#include <mjstr/string.hpp>
#include <ulpcl/utils.hpp>

namespace mjx {
    struct _Date_serializer {
        // writes serialized day into the buffer
        static void _Serialize_day(byte_t* const _Buf, const uint8_t _Day) noexcept;

        // writes serialized month into the buffer
        static void _Serialize_month(byte_t* const _Buf, const uint8_t _Month) noexcept;

        // writes serialized year into the buffer
        static void _Serialize_year(byte_t* const _Buf, const uint16_t _Year) noexcept;
    };

    struct symbol_location {
        uint64_t id    = 0; // location of the symbol ID
        uint64_t value = 0; // location of the symbol value
    };
 
    struct symbol {
        symbol_location location;
        utf8_string id;
    };

    struct _Symbol_serializer {
        // converts the symbol location into a string
        static byte_string _Serialize_location(const symbol_location _Location);

        // converts the symbol ID into a string
        static byte_string _Serialize_id(const utf8_string_view _Id);

        // converts the symbol into a string
        static byte_string _Serialize(const symbol& _Symbol);
    };

    class _Local_date_provider {
    public:
        _Local_date_provider()                                       = delete;
        _Local_date_provider(const _Local_date_provider&)            = delete;
        _Local_date_provider& operator=(const _Local_date_provider&) = delete;

        // returns the current local date as a string
        static byte_string _Query();

    private:
        struct _Local_date {
            uint8_t _Day;
            uint8_t _Month;
            uint16_t _Year;
        };

        // returns the current local date
        static _Local_date _Query_raw() noexcept;
    };
     
    struct report_counters;

    class _Symbol_file {
    public:
        _Symbol_file(const path& _Target, report_counters& _Counters);
        ~_Symbol_file() noexcept;

        _Symbol_file(const _Symbol_file&)            = delete;
        _Symbol_file& operator=(const _Symbol_file&) = delete;

        // checks if the symbol file is open
        bool _Is_open() const noexcept;

        // writes automatically-generated comment to the symbol file
        bool _Write_comment();

        // writes a symbol to the symbol file
        bool _Write_symbol(const symbol& _Symbol, const bool _Break_line);

    private:
        // creates a new symbol file
        void _Create(const path& _Target);

        // opens an existing symbol file
        void _Open(const path& _Target);

        file _Myfile;
        file_stream _Mystream;
        report_counters& _Myctrs;
    };

    path _Get_symbol_file_path(const unicode_string_view _Pack);

    bool generate_symbol_file(
        const unicode_string_view _Pack, const vector<symbol>& _Symbols, report_counters& _Counters);
} // namespace mjx

#endif // _ULPCL_SYMBOL_FILE_HPP_