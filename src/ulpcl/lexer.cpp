// lexer.cpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <mjfs/file.hpp>
#include <mjfs/file_stream.hpp>
#include <mjmem/exception.hpp>
#include <ulpcl/keyword.hpp>
#include <ulpcl/lexer.hpp>
#include <ulpcl/logger.hpp>
#include <ulpcl/program.hpp>
#include <ulpcl/runtime.hpp>
#include <type_traits>

namespace mjx {
    size_t token_stream::size() const noexcept {
        return _Mytokens.size();
    }

    const token& token_stream::get_token(const size_t _Idx) const {
        if (_Idx >= _Mytokens.size()) {
            resource_overrun::raise();
        }

        return _Mytokens[_Idx];
    }

    void token_stream::append(const token& _Token) {
        _Mytokens.push_back(_Token);
    }

    void token_stream::append(token&& _Token) {
        _Mytokens.push_back(::std::move(_Token));
    }

    _Lexer_iterator::_Lexer_iterator(const byte_string_view _Bytes) noexcept
        : _First(_Bytes.data()), _Last(_First + _Bytes.size()), _Current(_First) {}

    bool _Token_parser::_Is_keyword(const byte_string_view _Token) noexcept {
        return parse_keyword(_Token) != keyword::none;
    }

    bool _Token_parser::_Is_identifier(const byte_string_view _Token) noexcept {
        // identifier must start with '#' and must not contain a colon
        return _Token.starts_with('#') && !_Token.contains(':');
    }

    token_type _Token_parser::_Parse_type(const byte_string_view _Token) noexcept {
        constexpr byte_t _LCBracket[] = {'{', '\0'};
        constexpr byte_t _RCBracket[] = {'}', '\0'};
        constexpr byte_t _Colon[]     = {':', '\0'};
        if (_Is_keyword(_Token)) {
            return token_type::keyword;
        } else if (_Is_identifier(_Token)) {
            return token_type::identifier;
        } else if (_Token == _LCBracket) {
            return token_type::left_curly_bracket;
        } else if (_Token == _RCBracket) {
            return token_type::right_curly_bracket;
        } else if (_Token == _Colon) {
            return token_type::colon;
        } else { // unknown token type
            return token_type::none;
        }
    }

    _Analysis_handler::_Analysis_handler(_Lexer_cache& _Cache, _Lexer_iterator& _Iter) noexcept
        : _Mycache(_Cache), _Myiter(_Iter) {}

    _Analysis_handler::~_Analysis_handler() noexcept {}

    void _Analysis_handler::_Next_line() noexcept {
        ++_Mycache._Location._Current.line;
        _Mycache._Location._Current.column = 1; // reset column number
    }

    void _Analysis_handler::_Capture_current_location() noexcept {
        _Mycache._Location._Captured = _Mycache._Location._Current;
    }

    void _Analysis_handler::_Maybe_capture_current_location() noexcept {
        if (_Mycache._Block == _Analysis_block::_Normal && _Mycache._Buf.empty()) {
            _Capture_current_location();
        }
    }

    void _Analysis_handler::_Append_token(const token_type _Type) {
        byte_t _Sign[2] = {'\0'};
        switch (_Type) {
        case token_type::left_curly_bracket:
            *_Sign = '{';
            break;
        case token_type::right_curly_bracket:
            *_Sign = '}';
            break;
        case token_type::colon:
            *_Sign = ':';
            break;
        default: // requested token is not trivial, break
            return;
        }

        _Mycache._Stream.append(token{_Mycache._Location._Captured, _Type, _Sign});
    }

    void _Analysis_handler::_Flush_buffer() {
        byte_string& _Buf = _Mycache._Buf;
        _Mycache._Stream.append(token{_Mycache._Location._Captured, _Token_parser::_Parse_type(_Buf), _Buf});
        _Buf.clear(); // clear the buffer
    }

    void _Analysis_handler::_Flush_buffer_as_string_literal() {
        byte_string& _Buf = _Mycache._Buf;
        _Mycache._Stream.append(token{_Mycache._Location._Captured, token_type::string_literal, _Buf});
        _Buf.clear(); // clear the buffer
    }

    void _Analysis_handler::_On_quote() {
        if (_Mycache._Block == _Analysis_block::_Normal) { // found opening quote, switch to string literal block
            _Mycache._Block = _Analysis_block::_String_literal;
            _Capture_current_location();
        } else if (_Mycache._Block == _Analysis_block::_String_literal) {
            if (_Myiter._Current > _Myiter._First && *(_Myiter._Current - 1) == '\\') {
                _Mycache._Buf.back() = '"'; // replace slash with quote
            } else { // switch to normal block
                _Flush_buffer_as_string_literal();
                _Mycache._Block = _Analysis_block::_Normal;
            }
        }

        ++_Mycache._Location._Current.column;
    }

    bool _Analysis_handler::_On_eol() {
        if (_Mycache._Block == _Analysis_block::_Normal) {
            if (!_Mycache._Buf.empty()) { // append next token to the stream
                _Flush_buffer();
            }
        } else if (_Mycache._Block == _Analysis_block::_Comment) { // switch to normal block
            _Mycache._Block = _Analysis_block::_Normal;
        } else { // missing closing bracket detected, break
            return false;
        }

        _Next_line(); // advance to the next line
        return true;
    }

    bool _Analysis_handler::_On_space() {
        if (_Mycache._Block != _Analysis_block::_String_literal) {
            return false;
        }

        _Mycache._Buf.push_back(' '); // append next character to the buffer
        ++_Mycache._Location._Current.column;
        return true;
    }

    void _Analysis_handler::_On_whitespace() {
        if (_Mycache._Block == _Analysis_block::_Normal && !_Mycache._Buf.empty()) {
            _Flush_buffer(); // append next token to the stream
        }

        ++_Mycache._Location._Current.column;
    }

    void _Analysis_handler::_On_slash() {
        if (_Mycache._Block == _Analysis_block::_Normal) {
            if (_Myiter._Last - _Myiter._Current > 0 && *(_Myiter._Current + 1) == '/') {
                _Mycache._Block = _Analysis_block::_Comment; // switch to comment block
                if (!_Mycache._Buf.empty()) { // append next token to the stream
                    _Flush_buffer();
                }
            }
        } else if (_Mycache._Block == _Analysis_block::_String_literal) { // append next character to the buffer
            _Mycache._Buf.push_back('/');
        }

        ++_Mycache._Location._Current.column;
    }

    void _Analysis_handler::_On_colon() {
        if (_Mycache._Block == _Analysis_block::_Normal) { // store colon as a separate token
            if (!_Mycache._Buf.empty()) { // append next token to the stream
                _Flush_buffer();
            }

            _Capture_current_location();
            _Append_token(token_type::colon);
        } else if (_Mycache._Block == _Analysis_block::_String_literal) { // append next character to the buffer
            _Mycache._Buf.push_back(':');
        }

        ++_Mycache._Location._Current.column;
    }

    void _Analysis_handler::_On_left_curly_bracket() {
        if (_Mycache._Block == _Analysis_block::_Normal) { // store left-curly-bracket as a separate token
            if (!_Mycache._Buf.empty()) { // append next token to the stream
                _Flush_buffer();
            }

            _Capture_current_location();
            _Append_token(token_type::left_curly_bracket);
        } else if (_Mycache._Block == _Analysis_block::_String_literal) { // append next character to the buffer
            _Mycache._Buf.push_back('{');
        }

        ++_Mycache._Location._Current.column;
    }

    void _Analysis_handler::_On_right_curly_bracket() {
        if (_Mycache._Block == _Analysis_block::_Normal) { // store right-curly-bracket as a separate token
            if (!_Mycache._Buf.empty()) { // append next token to the stream
                _Flush_buffer();
            }

            _Capture_current_location();
            _Append_token(token_type::right_curly_bracket);
        } else if (_Mycache._Block == _Analysis_block::_String_literal) { // append next character to the buffer
            _Mycache._Buf.push_back('}');
        }

        ++_Mycache._Location._Current.column;
    }

    void _Analysis_handler::_On_char() {
        if (_Mycache._Block != _Analysis_block::_Comment) { // append next character to the buffer
            _Maybe_capture_current_location();
            _Mycache._Buf.push_back(*_Myiter._Current);
        }

        ++_Mycache._Location._Current.column;
    }

    lexical_analyzer::lexical_analyzer(report_counters& _Counters) noexcept
        : _Mycache(), _Myctrs(_Counters) {}

    lexical_analyzer::~lexical_analyzer() noexcept {}

    bool lexical_analyzer::analyze(const byte_string_view _Input_data) {
        _Lexer_iterator _Iter(_Input_data);
        _Analysis_handler _Handler(_Mycache, _Iter);
        for (; _Iter._Current != _Iter._Last; ++_Iter._Current) {
            switch (*_Iter._Current) {
            case '"':
                _Handler._On_quote();
                break;
            case '\n':
                if (!_Handler._On_eol()) { // report an error
                    // Note: The missing quote error is detected only when the current analysis block is
                    //       a string literal. In such cases, we have captured the location of the current
                    //       string literal. Therefore, it's preferable to use the captured location
                    //       instead of the current one, as this provides a more accurate error message.
                    _Report_error(_Myctrs, L"(%u, %u): error E2016: missing closing quote '\"' for string literal",
                        _Mycache._Location._Captured.line, _Mycache._Location._Captured.column);
                    return false;
                }

                break;
            case ' ':
                if (_Handler._On_space()) {
                    break;
                }

                [[fallthrough]];
            case '\t':
            case '\v':
            case '\f':
            case '\r':
                _Handler._On_whitespace();
                break;
            case '/':
                _Handler._On_slash();
                break;
            case ':':
                _Handler._On_colon();
                break;
            case '{':
                _Handler._On_left_curly_bracket();
                break;
            case '}':
                _Handler._On_right_curly_bracket();
                break;
            default:
                _Handler._On_char();
                break;
            }

        }

        return true;
    }

    bool lexical_analyzer::complete_analysis() {
        // Note: This function is called after lexical analysis, with the main purpose of detecting
        //       opened string literals. The analyze() function may not detect a missing closing quote
        //       if the string literal is the last token in the input data, hence it must be checked
        //       manually. The analysis block after lexical analysis is expected to be either _Normal
        //       or _Comment, as comments are allowed and completely discarded during analysis.
        if (_Mycache._Block == _Analysis_block::_String_literal) { // report an error
            _Report_error(_Myctrs, L"(%u, %u): error E2016: missing closing quote '\"' for string literal",
                _Mycache._Location._Captured.line, _Mycache._Location._Captured.column);
            return false;
        }

        return true;
    }

    const token_stream& lexical_analyzer::stream() const noexcept {
        return _Mycache._Stream;
    }

    analysis_result analyze_input_file(const path& _Target, report_counters& _Counters) {
        clog(L"> Starting lexical analysis");
        file _File(_Target, file_access::read, file_share::read);
        file_stream _Stream(_File);
        if (!_Stream.is_open()) { // cannot open the input file
            _Report_error(_Counters, L"(?, ?): error E1000: cannot open input file '%s'", _Target.c_str());
            return analysis_result{false};
        }

        if (_File.size() == 0) { // the input file is empty
            if (program_options::current().model == error_model::strict) { // report an error and exit
                _Report_error(_Counters, L"(?, ?): error E1001: input file '%s' is empty", _Target.c_str());
                return analysis_result{false};
            } else { // report a warning and exit
                _Report_warning(_Counters, L"(?, ?): warning W1000: input file '%s' is empty", _Target.c_str());
                return analysis_result{true};
            }
        }

        lexical_analyzer _Lexer(_Counters);
        bool _Success        = true;
        const float _Elapsed = measure_invoke_duration(
            [&] {
                bool _Check_bom            = true;
                constexpr size_t _Buf_size = 4096;
                byte_t _Buf[_Buf_size];
                size_t _Read;
                for (;;) { // analyze the file chunk by chunk
                    _Read = _Stream.read(_Buf, _Buf_size);
                    if (_Read == 0) { // no more data, break
                        break;
                    }

                    if (_Check_bom) { // check for presence of any BOM
                        _Check_bom                = false; // check it once
                        const _Bom& _Detected_bom = _Bom_detector::_Detect(byte_string_view{_Buf, _Read});
                        switch (_Detected_bom._Kind) {
                        case _Bom_kind::_None: // BOM not present, do nothing
                            break;
                        case _Bom_kind::_Utf8: // detected UTF-8 BOM, discard and continue
                            using _Traits = char_traits<byte_t>;
                            _Read        -= _Detected_bom._Size;
                            _Traits::move(_Buf, _Buf + _Detected_bom._Size, _Read);
                            break;
                        default: // detected unsupported BOM, break
                            _Success = false;
                            _Report_error(
                                _Counters, L"(?, ?): error E1002: detected unsupported encoding");
                            return;
                        }
                    }

                    if (!_Lexer.analyze(byte_string_view{_Buf, _Read})) { // analysis failed, break
                        _Success = false;
                        return;
                    }
                }

                if (!_Lexer.complete_analysis()) { // analysis completion failed
                    _Success = false;
                }
            }
        );
        if (_Success) {
            clog(L"> Completed lexical analysis (took %.5fs)", _Elapsed);
            return analysis_result{true, _Lexer.stream()};
        } else { // something went wrong
            return analysis_result{false};
        }
    }
} // namespace mjx