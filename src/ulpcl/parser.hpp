// parser.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _ULPCL_PARSER_HPP_
#define _ULPCL_PARSER_HPP_
#include <cstdint>
#include <mjstr/string.hpp>
#include <mjstr/string_view.hpp>
#include <ulpcl/keyword.hpp>
#include <ulpcl/lexer.hpp>
#include <ulpcl/utils.hpp>

namespace mjx {
    struct message {
        utf8_string id;
        unicode_string value;
    };

    struct group {
        utf8_string name;
        vector<message> messages;
        vector<group> groups;
    };

    struct root_group {
        vector<message> messages;
        vector<group> groups;
    };

    struct parse_tree {
        unicode_string language;
        uint32_t lcid = 0;
        root_group content;
    };

    struct report_counters;

    class _Parser_base {
    public:
        size_t& _Off;
        parse_tree& _Tree;
        const token_stream& _Stream;
        report_counters& _Counters;

        // returns the number of remaining tokens
        size_t _Remaining_tokens() const noexcept;

        // checks if the token is the specified keyword
        bool _Is_matching_keyword(const token& _Token, const keyword _Keyword) const noexcept;

        // returns the current token
        const token& _Get_current_token() const;

        // returns the current token and advances to the next one
        const token& _Get_current_token_and_advance() const;

        // returns the next token
        const token& _Get_next_token() const;
    };

    struct _Lcid_parser {
        static constexpr uint32_t _Invalid = static_cast<uint32_t>(-1);
        static constexpr uint32_t _Max     = 0x7FFF'FFFF;

        // parses an LCID
        static uint32_t _Parse(const byte_string_view _Bytes) noexcept;
    };

    class _Static_parser : public _Parser_base {
    public:
        // parses static tokens
        bool _Parse();

    private:
        // parses '@language' directive
        bool _Parse_language();

        // parses '@lcid' directive
        bool _Parse_lcid();

        // skips '@meta' section
        bool _Skip_meta() noexcept;

        // validates '@content' section
        bool _Validate_content() noexcept;
    };

    struct _Name_validator {
        // checks if the specified identifier name is valid
        static bool _Is_valid_identifier_name(const byte_string_view _Name) noexcept;

        // checks if the specified group name is valid
        static bool _Is_valid_group_name(const byte_string_view _Name) noexcept;

        // checks if the specified identifier is unique in _Messages
        static bool _Is_identifier_name_unique(
            const vector<message>& _Messages, const utf8_string_view _Name) noexcept;

        // checks if the specified
        static bool _Is_group_name_unique(const vector<group>& _Groups, const utf8_string_view _Name) noexcept;

    private:
        // checks if _Ch can be used to represent a name
        static bool _Is_valid_char(const byte_t _Ch) noexcept;
    };

    class _Dynamic_parser : public _Parser_base {
    public:
        // parses dynamic tokens
        bool _Parse();

    private:
        // appends a new group to the already existing group
        template <class _Group_type>
        bool _Append_group(_Group_type& _Group, const utf8_string_view _Name);

        // appends a new message to the already existing group
        template <class _Group_type>
        bool _Append_message(_Group_type& _Group, const utf8_string_view _Id, const unicode_string_view _Value);

        // parses a group
        template <class _Group_type>
        bool _Parse_group(_Group_type& _Group, const token_location _Location);

        // parses a message
        template <class _Group_type>
        bool _Parse_message(_Group_type& _Group);
    };

    struct parse_result {
        bool success;
        parse_tree tree;
    };

    parse_result parse_token_stream(
        const token_stream& _Stream, const unicode_string_view _Pack, report_counters& _Counters);
} // namespace mjx

#endif // _ULPCL_PARSER_HPP_