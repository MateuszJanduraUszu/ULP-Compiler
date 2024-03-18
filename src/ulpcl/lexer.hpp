// lexer.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _ULPCL_LEXER_HPP_
#define _ULPCL_LEXER_HPP_
#include <cstdint>
#include <mjfs/path.hpp>
#include <mjstr/string.hpp>
#include <mjstr/string_view.hpp>
#include <ulpcl/utils.hpp>

namespace mjx {
    struct token_location {
        uint32_t line   = 0;
        uint32_t column = 0;
    };

    enum class token_type : unsigned char {
        none,
        keyword,
        identifier,
        string_literal,
        left_curly_bracket,
        right_curly_bracket,
        colon
    };

    struct token {
        token_location location;
        token_type type = token_type::none;
        byte_string data;
    };

    class token_stream { // stores a sequence of tokens
    public:
        token_stream() noexcept               = default;
        token_stream(const token_stream&)     = default;
        token_stream(token_stream&&) noexcept = default;
        ~token_stream() noexcept              = default;
    
        token_stream& operator=(const token_stream&)     = default;
        token_stream& operator=(token_stream&&) noexcept = default;
    
        // returns the number of tokens
        size_t size() const noexcept;

        // returns the specified token
        const token& get_token(const size_t _Idx) const;

        // appends a new token
        void append(const token& _Token);
        void append(token&& _Token);

    private:
        vector<token> _Mytokens;
    };

    enum class _Analysis_block : unsigned char {
        _Normal, // normal analysis state
        _Comment, // ignore until the end of the line is found
        _String_literal // skip analysis until the closing quote is encountered
    };

    struct _Lexer_location {
        token_location _Current = {1, 1};
        token_location _Captured;
    };

    struct _Lexer_cache { // stores data that is shared between lexical analyzer and analysis handler
        token_stream _Stream;
        byte_string _Buf;
        _Lexer_location _Location;
        _Analysis_block _Block = _Analysis_block::_Normal;
    };

    struct _Lexer_iterator { // bounded iterator for lexical analysis
        const byte_t* _First;
        const byte_t* _Last;
        const byte_t* _Current;

        explicit _Lexer_iterator(const byte_string_view _Bytes) noexcept;
    };

    struct _Token_parser {
        // checks if the token is a keyword
        static bool _Is_keyword(const byte_string_view _Token) noexcept;

        // checks if the token is an identifier
        static bool _Is_identifier(const byte_string_view _Token) noexcept;

        // parses the token type
        static token_type _Parse_type(const byte_string_view _Token) noexcept;
    };

    class _Analysis_handler {
    public:
        _Analysis_handler(_Lexer_cache& _Cache, _Lexer_iterator& _Iter) noexcept;
        ~_Analysis_handler() noexcept;

        _Analysis_handler()                                    = delete;
        _Analysis_handler(const _Analysis_handler&)            = delete;
        _Analysis_handler& operator=(const _Analysis_handler&) = delete;

        // handles the occurrence of quote during lexical analysis
        void _On_quote();

        // handles the occurrence of end of line during lexical analysis
        void _On_eol();

        // handles the occurrence of space during lexical analysis
        bool _On_space();
        
        // handles the occurrence of whitespace during lexical analysis
        void _On_whitespace();

        // handles the occurrence of slash during lexical analysis
        void _On_slash();

        // handles the occurrence of colon during lexical analysis
        void _On_colon();

        // handles the occurrence of left-curly-bracket during lexical analysis
        void _On_left_curly_bracket();

        // handles the occurrence of right-curly-bracket during lexical analysis
        void _On_right_curly_bracket();

        // handles the occurrence of character during lexical analysis
        void _On_char();
    
    private:
        // advances to the next line
        void _Next_line() noexcept;

        // captures the current location
        void _Capture_current_location() noexcept;

        // captures the current location if needed
        void _Maybe_capture_current_location() noexcept;

        // appends a trivial token
        void _Append_token(const token_type _Type);

        // appends a new token from the buffer
        void _Flush_buffer();

        // appends a new token of string literal type from the buffer
        void _Flush_buffer_as_string_literal();

        _Lexer_cache& _Mycache;
        _Lexer_iterator& _Myiter;
    };

    class lexical_analyzer { // breaks an input data into tokens
    public:
        lexical_analyzer() noexcept;
        ~lexical_analyzer() noexcept;

        lexical_analyzer(const lexical_analyzer&)            = delete;
        lexical_analyzer& operator=(const lexical_analyzer&) = delete;

        // analyzes the input data
        void analyze(const byte_string_view _Input_data);

        // returns the associated token stream
        const token_stream& stream() const noexcept;

    private:
        _Lexer_cache _Mycache;
    };

    struct analysis_result {
        bool success;
        token_stream stream;
    };

    struct report_counters;

    analysis_result analyze_input_file(const path& _Target, report_counters& _Counters);
} // namespace mjx

#endif // _ULPCL_LEXER_HPP_