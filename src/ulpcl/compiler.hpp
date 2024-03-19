// compiler.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _ULPCL_COMPILER_HPP_
#define _ULPCL_COMPILER_HPP_
#include <mjfs/file.hpp>
#include <mjfs/file_stream.hpp>
#include <mjfs/path.hpp>
#include <mjstr/string.hpp>
#include <mjstr/string_view.hpp>
#include <ulpcl/parser.hpp>
#include <ulpcl/symbol_file.hpp>

namespace mjx {
    path _Get_output_file_path(const unicode_string_view _Pack);
    vector<message> _Get_messages_from_group(const group& _Group, const utf8_string& _Namespace);
    vector<message> _Get_messages_from_content(const root_group& _Content);
    uint64_t _Compute_hash(const utf8_string_view _Id) noexcept;

#pragma pack(push)
#pragma pack(4) // align structure members on 4-byte boundaries to avoid alignment issues
                // while copying raw data
    struct _Lookup_table_entry {
        uint64_t _Hash   = 0;
        uint64_t _Offset = 0;
        uint32_t _Length = 0;
    };
#pragma pack(pop)

    class _Umc_file { // UFUI Message Catalog (UMC) file writer
    public:
        _Umc_file(const path& _Target, report_counters& _Counters);
        ~_Umc_file() noexcept;

        _Umc_file()                            = delete;
        _Umc_file(const _Umc_file&)            = delete;
        _Umc_file& operator=(const _Umc_file&) = delete;

        // checks if the UMC file is open
        bool _Is_open() const noexcept;

        // returns the current offset
        uint64_t _Current_offset() const noexcept;

        // writes the signature to the UMC file
        bool _Write_signature() noexcept;

        // writes a language name to the UMC file
        bool _Write_language(const unicode_string_view _Language);

        // writes an LCID to the UMC file
        bool _Write_lcid(const uint32_t _Lcid) noexcept;

        // writes a number of messages to the UMC file
        bool _Write_message_count(const uint32_t _Count) noexcept;

        // writes a lookup table entry to the UMC file
        bool _Write_lookup_table_entry(const _Lookup_table_entry _Entry) noexcept;

        // writes a message's value to the UMC file
        bool _Write_message_value(const byte_string_view _Value) noexcept;

    private:
        // creates a new UMC file
        void _Create(const path& _Target);

        // opens an existing UMC file
        void _Open(const path& _Target);

        file _Myfile;
        file_stream _Mystream;
        report_counters& _Myctrs;
    };

    class _Section_writer { // writes lookup table and blob to the UMC file
    public:
        _Section_writer(_Umc_file& _File, const vector<message>& _Messages);
        ~_Section_writer() noexcept;

        _Section_writer()                                  = delete;
        _Section_writer(const _Section_writer&)            = delete;
        _Section_writer& operator=(const _Section_writer&) = delete;

        // writes lookup table to the UMC file
        bool _Write_lookup_table() noexcept;

        // writes lookup table to the UMC file (captures symbols location)
        bool _Write_lookup_table(vector<symbol>& _Symbols) noexcept;

        // writes blob to the UMC file
        bool _Write_blob() noexcept;

    private:
        struct _Writable_message {
            uint64_t _Hash = 0; // 8-byte hash of the message ID
            byte_string _Value; // message's value in UTF-8 encoding
        };

        // converts plain messages to writable
        vector<_Writable_message> _Convert_messages(const vector<message>& _Messages);

        _Umc_file& _Myfile;
        vector<_Writable_message> _Mymsgs;
    };

    void _Allocate_symbols_and_copy_ids(vector<symbol>& _Symbols, const vector<message>& _Messages);
    bool _Compile_parse_tree(_Umc_file& _File, const parse_tree& _Tree, report_counters& _Counters);
    bool _Compile_parse_tree_and_generate_symbols(
        _Umc_file& _File, const parse_tree& _Tree, vector<symbol>& _Symbols, report_counters& _Counters);

    bool compile_input_file(const path& _Target);
} // namespace mjx

#endif // _ULPCL_COMPILER_HPP_