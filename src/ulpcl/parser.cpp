// parser.cpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <mjstr/conversion.hpp>
#include <ulpcl/logger.hpp>
#include <ulpcl/parser.hpp>
#include <ulpcl/program.hpp>
#include <ulpcl/runtime.hpp>

namespace mjx {
    size_t _Parser_base::_Remaining_tokens() const noexcept {
        return _Stream.size() - _Off - 1;
    }

    bool _Parser_base::_Is_matching_keyword(const token& _Token, const keyword _Keyword) const noexcept {
        return _Token.type == token_type::keyword && parse_keyword(_Token.data) == _Keyword;
    }

    const token& _Parser_base::_Get_current_token() const {
        return _Stream.get_token(_Off);
    }

    const token& _Parser_base::_Get_current_token_and_advance() const {
        return _Stream.get_token(_Off++);
    }

    const token& _Parser_base::_Get_next_token() const {
        return _Stream.get_token(++_Off);
    }

    uint32_t _Lcid_parser::_Parse(const byte_string_view _Bytes) noexcept {
        const byte_t* _First      = _Bytes.data();
        const byte_t* const _Last = _First + _Bytes.size();
        uint32_t _Value           = 0;
        for (; _First != _Last; ++_First) {
            if (*_First < '0' || *_First > '9') { // must consists only of digits
                return _Invalid;
            }

            _Value = _Value * 10 + static_cast<uint32_t>(*_First - '0');
            if (_Value > _Max) { // too big value, break
                return _Invalid;
            }
        }

        return _Value;
    }

    bool _Static_parser::_Parse_language() {
        if (_Remaining_tokens() < 3) { // language name consists of three tokens
            const token_location _Location = _Get_current_token().location;
            _Report_error(_Counters, L"(%u, %u): error E2000: undefined symbol '@language' which is required",
                _Location.line, _Location.column);
            return false;
        }

        // expected token order: '@language', ':' and '<string-literal>'
        const token& _First = _Get_current_token_and_advance();
        if (!_Is_matching_keyword(_First, keyword::language)) {
            _Report_error(_Counters, L"(%u, %u): error E2000: undefined symbol '@language' which is required",
                _First.location.line, _First.location.column);
            return false;
        }

        const token& _Second = _Get_current_token_and_advance();
        const token& _Third  = _Get_current_token_and_advance();
        if (_Second.type != token_type::colon || _Third.type != token_type::string_literal) {
            _Report_error(_Counters, L"(%u, %u): error E2006: invalid usage of the '@language' keyword",
                _First.location.line, _First.location.column);
            return false;
        }

        _Tree.language = ::mjx::to_unicode_string(_Third.data);
        return true;
    }

    bool _Static_parser::_Parse_lcid() {
        if (_Remaining_tokens() < 3) { // LCID consists of three tokens
            const token_location _Location = _Get_current_token().location;
            _Report_error(_Counters, L"(%u, %u): error E2000: undefined symbol '@lcid' which is required",
                _Location.line, _Location.column);
            return false;
        }

        // expected token order: '@lcid', ':' and '<string-literal>'
        const token& _First = _Get_current_token_and_advance();
        if (!_Is_matching_keyword(_First, keyword::lcid)) {
            _Report_error(_Counters, L"(%u, %u): error E2000: undefined symbol '@lcid' which is required",
                _First.location.line, _First.location.column);
            return false;
        }

        const token& _Second = _Get_current_token_and_advance();
        const token& _Third  = _Get_current_token_and_advance();
        if (_Second.type != token_type::colon || _Third.type != token_type::string_literal) {
            _Report_error(_Counters, L"(%u, %u): error E2006: invalid usage of the '@lcid' keyword",
                _First.location.line, _First.location.column);
            return false;
        }

        _Tree.lcid = _Lcid_parser::_Parse(_Third.data);
        if (_Tree.lcid == _Lcid_parser::_Invalid) { // invalid LCID, break
            _Report_error(_Counters, L"(%u, %u): error E2011: invalid '@lcid' value",
                _Third.location.line, _Third.location.column);
            return false;
        }

        return true;
    }

    bool _Static_parser::_Skip_meta() noexcept {
        // expected token order: '@meta', '{', ..., '}'; note that '@meta' is already skipped
        const token_location _Location = _Get_current_token_and_advance().location; // capture '@meta' location
        if (_Get_current_token_and_advance().type != token_type::left_curly_bracket) {
            _Report_error(_Counters, L"(%u, %u): error E2003: missing opening bracket '{' for group '@meta'",
                _Location.line, _Location.column);
            return false;
        }

        const size_t _Max_off = _Stream.size() - 1; // omit the last token (already checked)
        while (_Off < _Max_off) { // search for the right curly bracket
            const token& _Token = _Get_current_token_and_advance();
            if (_Is_matching_keyword(_Token, keyword::content)) {
                // '@content' cannot appear before the '@meta' closing bracket
                break;
            } else if (_Token.type == token_type::right_curly_bracket) { // right curly bracket found, break
                return true;
            }
        }

        _Report_error(_Counters, L"(%u, %u): error E2004: missing closing bracket '}' for group '@meta'",
            _Location.line, _Location.column);
        return false;
    }

    bool _Static_parser::_Validate_content() noexcept {
        if (_Remaining_tokens() < 3) { // content consists of at least three tokens
            const token_location _Location = _Get_current_token().location;
            _Report_error(_Counters, L"(%u, %u): error E2000: undefined symbol '@language' which is required",
                _Location.line, _Location.column);
            return false;
        }

        // expected token order: '@content', '{', ..., '}'
        const token& _First = _Get_current_token_and_advance();
        if (!_Is_matching_keyword(_First, keyword::content)) {
            _Report_error(_Counters, L"(%u, %u): error E2000: undefined symbol '@content' which is required",
                _First.location.line, _First.location.column);
            return false;
        }

        if (_Get_current_token_and_advance().type != token_type::left_curly_bracket) {
            _Report_error(_Counters, L"(%u, %u): error E2003: missing opening bracket '{' for group '@content'",
                _First.location.line, _First.location.column);
            return false;
        }

        if (_Stream.get_token(_Stream.size() - 2).type != token_type::right_curly_bracket) {
            _Report_error(_Counters, L"(%u, %u): error E2004: missing closing bracket '}' for group '@content'",
                _First.location.line, _First.location.column);
            return false;
        }

        return true;
    }

    bool _Static_parser::_Parse() {
        if (!_Parse_language() || !_Parse_lcid()) { // something went wrong, break
            return false;
        }

        if (_Remaining_tokens() < 2) {
            const token& _Token = _Get_current_token();
            if (_Token.type != token_type::left_curly_bracket) { // left curly bracket missing
                _Report_error(_Counters, L"(%u, %u): error E2001: missing opening bracket '{' for the global section",
                    _Token.location.line, _Token.location.column);
            } else { // right curly bracket missing
                const token_location _Location = _Stream.get_token(_Stream.size() - 1).location;
                _Report_error(_Counters, L"(%u, %u): error E2002: missing closing bracket '}' for the global section",
                    _Location.line, _Location.column);
            }

            return false;
        }

        { // check if left curly bracket is present
            const token& _Token = _Get_current_token_and_advance();
            if (_Token.type != token_type::left_curly_bracket) {
                _Report_error(_Counters, L"(%u, %u): error E2001: missing opening bracket '{' for the global section",
                    _Token.location.line, _Token.location.column);
                return false;
            }
        }

        { // check if right curly bracket is present
            const token& _Token = _Stream.get_token(_Stream.size() - 1); // should be as the last token
            if (_Token.type != token_type::right_curly_bracket) {
                _Report_error(_Counters, L"(%u, %u): error E2002: missing closing bracket '}' for the global section",
                    _Token.location.line, _Token.location.column);
                return false;
            }
        }

        if (_Remaining_tokens() > 0 && _Is_matching_keyword(_Get_current_token(), keyword::meta)) { // skip metadata
            if (!_Skip_meta()) { // something went wrong, break
                return false;
            }
        }

        return _Validate_content();
    }

    bool _Name_validator::_Is_valid_char(const byte_t _Ch) noexcept {
        switch (_Ch) {
        case '-':
        case '_':
            return true;
        default:
            return (_Ch >= '0' && _Ch <= '9') || (_Ch >= 'A' && _Ch <= 'Z')
                || (_Ch >= 'a' && _Ch <= 'z');
        }
    }

    bool _Name_validator::_Is_valid_identifier_name(const byte_string_view _Name) noexcept {
        const byte_string_view _Real_name = _Name.substr(1); // skip '#' sign
        for (const byte_t _Ch : _Real_name) {
            if (!_Is_valid_char(_Ch)) { // illegal character found, break
                return false;
            }
        }

        return true;
    }

    bool _Name_validator::_Is_valid_group_name(const byte_string_view _Name) noexcept {
        for (const byte_t _Ch : _Name) {
            if (!_Is_valid_char(_Ch)) { // illegal character found, break
                return false;
            }
        }

        return true;
    }

    bool _Name_validator::_Is_identifier_name_unique(
        const vector<message>& _Messages, const utf8_string_view _Name) noexcept {
        // check if _Name is unique within the scope of _Messages
        for (const message& _Message : _Messages) {
            if (_Message.id == _Name) { // same name found, break
                return false;
            }
        }

        return true;
    }

    bool _Name_validator::_Is_group_name_unique(
        const vector<group>& _Groups, const utf8_string_view _Name) noexcept {
        // check if _Name is unique within the scope of _Groups
        for (const group& _Group : _Groups) {
            if (_Group.name == _Name) { // same name found, break
                return false;
            }
        }
    
        return true;
    }

    template <class _Group_type>
    bool _Dynamic_parser::_Append_group(_Group_type& _Group, const utf8_string_view _Name) {
        if (!_Name_validator::_Is_group_name_unique(_Group.groups, _Name)) { // ambiguous name, break
            return false;
        }

        _Group.groups.push_back(group{_Name});
        return true;
    }

    template <class _Group_type>
    bool _Dynamic_parser::_Append_message(
        _Group_type& _Group, const utf8_string_view _Id, const unicode_string_view _Value) {
        if (!_Name_validator::_Is_identifier_name_unique(_Group.messages, _Id)) { // ambiguous name, break
            return false;
        }

        _Group.messages.push_back(message{_Id, _Value});
        return true;
    }

    template <class _Group_type>
    bool _Dynamic_parser::_Parse_group(_Group_type& _Group, const token_location _Location) {
        if (_Remaining_tokens() < 4) { // group consists of at least five tokens (keyword already skipped)
            _Report_error(_Counters, L"(%u, %u): error E2006: invalid usage of the '@group' keyword",
                _Location.line, _Location.column);
            return false;
        }

        // expected token order: '@group', ':', '<string-literal>', '{', ..., '}';
        // note that '@group' is already skipped
        if (_Get_current_token_and_advance().type != token_type::colon) {
            _Report_error(_Counters, L"(%u, %u): error E2006: invalid usage of the '@group' keyword",
                _Location.line, _Location.column);
            return false;
        }

        const token& _First = _Get_current_token_and_advance();
        if (_First.type != token_type::string_literal) {
            _Report_error(_Counters, L"(%u, %u): error E2006: invalid usage of the '@group' keyword",
                _Location.line, _Location.column);
            return false;
        }

        if (!_Name_validator::_Is_valid_group_name(_First.data)) {
            _Report_error(_Counters, L"(%u, %u): error E2009: illegal group name '%s'",
                _First.location.line, _First.location.column, _Fast_str_cvt<wchar_t>(_First.data).c_str());
            return false;
        }

        if (_Get_current_token_and_advance().type != token_type::left_curly_bracket) {
            _Report_error(_Counters, L"(%u, %u): error E2003: missing opening bracket '{' for group '%s'",
                _Location.line, _Location.column, _Fast_str_cvt<wchar_t>(_First.data).c_str());
            return false;
        }

        if (!_Append_group(_Group, ::mjx::to_utf8_string(_First.data))) { // failed to append the group, break
            _Report_error(_Counters, L"(%u, %u): error E2007: ambiguous group name, '%s' is already defined",
                _First.location.line, _First.location.column, _Fast_str_cvt<wchar_t>(_First.data).c_str());
            return false;
        }

        const size_t _Max_off = _Stream.size() - 2; // omit the last two tokens (right curly brackets)
        while (_Off < _Max_off) {
            const token& _Token = _Get_current_token(); // don't advance
            switch (_Token.type) {
            case token_type::keyword: // parse a group
                if (parse_keyword(_Token.data) != keyword::group) { // invalid keyword usage
                    _Report_error(_Counters, L"(%u, %u): error E2006: invalid usage of the '%s' keyword",
                        _Token.location.line, _Token.location.column, _Fast_str_cvt<wchar_t>(_Token.data).c_str());
                    return false;
                }

                ++_Off; // skip '@group' keyword
                if (!_Parse_group(_Group.groups.back(), _Token.location)) { // failed to parse the group
                    return false;
                }

                break;
            case token_type::identifier: // parse a message
                if (!_Parse_message(_Group.groups.back())) { // failed to parse the message
                    return false;
                }

                break;
            case token_type::right_curly_bracket: // close the current group
            {
                const group& _This_group = _Group.groups.back();
                if (_This_group.messages.empty() && _This_group.groups.empty()) {
                    if (program_options::current().model == error_model::strict) { // report an error
                        _Report_error(_Counters, L"(%u, %u): error E2015: group '%s' has no members",
                            _Location.line, _Location.column, _Fast_str_cvt<wchar_t>(_First.data).c_str());
                        return false;
                    } else { // report a warning
                        _Report_warning(_Counters, L"(%u, %u): warning W2002: group '%s' has no members",
                            _Location.line, _Location.column, _Fast_str_cvt<wchar_t>(_First.data).c_str());
                    }
                }

                ++_Off;
                return true;
            }
            default:
                _Report_error(_Counters, L"(%u, %u): error E2012: unexpected token '%s'",
                    _Token.location.line, _Token.location.column, _Fast_str_cvt<wchar_t>(_Token.data).c_str());
                return false;
            }
        }

        _Report_error(_Counters, L"(%u, %u): error E2004: missing closing bracket '}' for group '%s'",
            _Location.line, _Location.column, _Fast_str_cvt<wchar_t>(_First.data).c_str());
        return false;
    }

    template <class _Group_type>
    bool _Dynamic_parser::_Parse_message(_Group_type& _Group) {
        if (_Remaining_tokens() < 3) { // message consists of three tokens
            const token& _Token = _Get_current_token();
            _Report_error(_Counters, L"(%u, %u): error E2005: incomplete message '%s'",
                _Token.location.line, _Token.location.column, _Fast_str_cvt<wchar_t>(_Token.data).c_str());
            return false;
        }

        // expected token order: '#<id>', ':' and '<value>'
        const token& _First = _Get_current_token_and_advance();
        if (!_Name_validator::_Is_valid_identifier_name(_First.data)) { // illegal identifier, break
            _Report_error(_Counters, L"(%u, %u): error E2010: illegal identifier name '%s'",
                _First.location.line, _First.location.column, _Fast_str_cvt<wchar_t>(_First.data).c_str());
            return false;
        }

        const token& _Second = _Get_current_token_and_advance();
        const token& _Third  = _Get_current_token_and_advance();
        if (_Second.type != token_type::colon || _Third.type != token_type::string_literal) {
            _Report_error(_Counters, L"(%u, %u): error E2005: incomplete message '%s'",
                _First.location.line, _First.location.column, _Fast_str_cvt<wchar_t>(_First.data).c_str());
            return false;
        }

        // Note: Due to support for multi-line messages, we must scan for consecutive string literals,
        //       each representing a single line of the message.
        const size_t _Max_off = _Stream.size() - 2;
        byte_string _Value    = _Third.data;
        while (_Off < _Max_off) {
            const token& _Token = _Get_current_token(); // don't advance
            if (_Token.type != token_type::string_literal) {
                break;
            }

            _Value.push_back('\n');
            _Value.append(_Token.data);
            ++_Off; // advance to the next token
        }

        program_options& _Options = program_options::current();
        const bool _Empty         = _Value.empty();
        if (_Empty) { // empty message found
            if (_Options.model == error_model::strict) { // report error and break
                _Report_error(_Counters, L"(%u, %u): error E2014: message '%s' has an empty value",
                    _Third.location.line, _Third.location.column, _Fast_str_cvt<wchar_t>(_First.data).c_str());
                return false;
            }

            _Report_warning(_Counters, L"(%u, %u): warning W2001: message '%s' has an empty value",
                _Third.location.line, _Third.location.column, _Fast_str_cvt<wchar_t>(_First.data).c_str());
            if (_Options.discard_empty_messages) { // discard empty message
                return true;
            }
        }

        if (!_Append_message( // ambiguous name found, break
            _Group, ::mjx::to_utf8_string(_First.data), _Empty ? L"" : ::mjx::to_unicode_string(_Value))) {
            _Report_error(_Counters, L"(%u, %u): error E2008: ambiguous identifier name, '%s' is already defined",
                _First.location.line, _First.location.column, _Fast_str_cvt<wchar_t>(_First.data).c_str());
            return false;
        }

        return true;
    }

    bool _Dynamic_parser::_Parse() {
        const size_t _Max_off = _Stream.size() - 2; // omit the last two tokens (right curly brackets)
        while (_Off < _Max_off) {
            const token& _Token = _Get_current_token(); // don't advance
            switch (_Token.type) {
            case token_type::keyword:
                if (parse_keyword(_Token.data) != keyword::group) {
                    // in this context only the '@group' keyword is valid
                    _Report_error(_Counters, L"(%u, %u): error E2006: invalid usage of the '%s' keyword",
                        _Token.location.line, _Token.location.column, _Fast_str_cvt<wchar_t>(_Token.data).c_str());
                    return false;
                }

                ++_Off; // omit '@group' keyword
                if (!_Parse_group(_Tree.content, _Token.location)) { // failed to parse a group, break
                    return false;
                }

                break;
            case token_type::identifier: // parse a message
                if (!_Parse_message(_Tree.content)) { // failed to parse a message, break
                    return false;
                }

                break;
            default:
                _Report_error(_Counters, L"(%u, %u): error E2012: unexpected token '%s'",
                    _Token.location.line, _Token.location.column, _Fast_str_cvt<wchar_t>(_Token.data).c_str());
                return false;
            }
        }

        return true;
    }

    parse_result parse_token_stream(
        const token_stream& _Stream, const unicode_string_view _Pack, report_counters& _Counters) {
        clog(L"> Starting parse");
        parse_tree _Tree;
        bool _Success        = true;
        const float _Elapsed = measure_invoke_duration(
            [&] {
                size_t _Off = 0;
                _Static_parser _Static{_Off, _Tree, _Stream, _Counters};
                if (!_Static._Parse()) { // could not parse static tokens, break
                    _Success = false;
                    return;
                }

                _Dynamic_parser _Dynamic{_Off, _Tree, _Stream, _Counters};
                if (!_Dynamic._Parse()) { // could not parse dynamic tokens
                    _Success = false;
                }
            }
        );
        if (!_Success) { // something went wrong, break
            return parse_result{false};
        }

        if (_Tree.content.messages.empty() && _Tree.content.groups.empty()) {
            if (program_options::current().model == error_model::strict) { // report an error
                _Report_error(
                    _Counters, L"(?, ?): error E2013: pack '%s' has no messages or groups", _Pack.data());
                return parse_result{false};
            } else { // report a warning
                _Report_warning(
                    _Counters, L"(?, ?): warning W2000: pack '%s' has no messages or groups", _Pack.data());
            }
        }

        clog(L"> Completed parse (took %.5fs)", _Elapsed);
        return parse_result{true, _Tree};
    }
} // namespace mjx