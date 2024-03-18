// symbol_file.cpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <cstdio>
#include <mjfs/file.hpp>
#include <mjfs/file_stream.hpp>
#include <mjfs/status.hpp>
#include <ulpcl/logger.hpp>
#include <ulpcl/program.hpp>
#include <ulpcl/runtime.hpp>
#include <ulpcl/symbol_file.hpp>
#include <ulpcl/tinywin.hpp>
#include <ulpcl/version.hpp>
#include <type_traits>

namespace mjx {
    _Local_date _Get_local_date() noexcept {
        SYSTEMTIME _Time;
        ::GetLocalTime(&_Time);
        return _Local_date{
            static_cast<uint8_t>(_Time.wDay), static_cast<uint8_t>(_Time.wMonth), _Time.wYear};
    }

    void _Write_day_or_month_to_buffer(byte_t* const _Buf, const uint8_t _Day_or_month) noexcept {
        // write _Day_or_month to _Buf and end with a dot, assumes that _Buf will fit three characters
        if (_Day_or_month < 10) { // write one digit, prepend with a zero
            _Buf[0] = '0';
            _Buf[1] = _Day_or_month + '0';
        } else { // write two digits
            _Buf[0] = (_Day_or_month / 10) + '0';
            _Buf[1] = (_Day_or_month % 10) + '0';
        }

        _Buf[2] = '.'; // write a dot that will connect the two date components
    }

    void _Write_year_to_buffer(byte_t* const _Buf, const uint16_t _Year) noexcept {
        // write _Year to _Buf, assumes that _Buf will fit four characters
        if (_Year < 10) { // write one digit, prepend with three zeros
            _Buf[0] = '0';
            _Buf[1] = '0';
            _Buf[2] = '0';
            _Buf[3] = static_cast<byte_t>(_Year) + '0';
        } else if (_Year < 100) { // write two digits, prepend with two zeros
            _Buf[0] = '0';
            _Buf[1] = '0';
            _Buf[2] = static_cast<byte_t>((_Year % 100) / 10) + '0';
            _Buf[3] = static_cast<byte_t>(_Year % 10) + '0';
        } else if (_Year < 1000) { // write three digits, prepend with a zero
            _Buf[0] = '0';
            _Buf[1] = static_cast<byte_t>((_Year % 1000) / 100) + '0';
            _Buf[2] = static_cast<byte_t>((_Year % 100) / 10) + '0';
            _Buf[3] = static_cast<byte_t>(_Year % 10) + '0';
        } else { // write four digits
            _Buf[0] = static_cast<byte_t>(_Year / 1000) + '0';
            _Buf[1] = static_cast<byte_t>((_Year % 1000) / 100) + '0';
            _Buf[2] = static_cast<byte_t>((_Year % 100) / 10) + '0';
            _Buf[3] = static_cast<byte_t>(_Year % 10) + '0';
        }
    }

    byte_string _Get_current_date() {
        constexpr size_t _Str_size = 10; // always ten characters ('dd.mm.yyyy')
        byte_t _Buf[_Str_size + 1] = {'\0'}; // must fit serialized date + null-terminator
        const _Local_date _Date    = _Get_local_date();
        _Write_day_or_month_to_buffer(_Buf, _Date._Day);
        _Write_day_or_month_to_buffer(_Buf + 3, _Date._Month); // skip 'dd.'
        _Write_year_to_buffer(_Buf + 6, _Date._Year); // skip 'dd.mm.'
        return byte_string{_Buf, _Str_size};
    }

    byte_string _Symbol_serializer::_Serialize_location(const symbol_location _Location) {
        // Note: This function converts _Location into '(x, y)', where x and y are always
        //       16 digits in length. The buffer size is calculated as 2 * 16 (two 16-digit values)
        //       + 2 (left and right parenthesis) + 2 (comma and space between values)
        constexpr size_t _Buf_size = 37; // 36 characters for the pattern + 1 for null-terminator
        char _Buf[_Buf_size]       = {'\0'};
        ::snprintf(_Buf, _Buf_size, "(%016llX, %016llX)", _Location.id, _Location.value);
        return byte_string{reinterpret_cast<const byte_t*>(_Buf), _Buf_size - 1};
    }

    byte_string _Symbol_serializer::_Serialize_id(const utf8_string_view _Id) {
        return byte_string{reinterpret_cast<const byte_t*>(_Id.data()), _Id.size()};
    }
    
    byte_string _Symbol_serializer::_Serialize(const symbol& _Symbol) {
        static constexpr byte_t _Connector[] = {':', ' ', '\0'};
        return _Serialize_location(_Symbol.location) + _Connector + _Serialize_id(_Symbol.id);
    }

    _Symbol_file::_Symbol_file(const path& _Target, report_counters& _Counters)
        : _Myfile(), _Mystream(), _Myctrs(_Counters) {
        if (::mjx::exists(_Target)) { // open an existing file for overwrite
            _Open(_Target);
        } else { // create a new file
            _Create(_Target);
        }
    }

    _Symbol_file::~_Symbol_file() noexcept {}

    void _Symbol_file::_Create(const path& _Target) {
        if (::mjx::create_file(_Target, ::std::addressof(_Myfile))) {
            _Mystream.bind_file(_Myfile);
        } else { // report an error
            _Report_error(_Myctrs, L"(?, ?): error E4000: cannot create the symbol file '%s'", _Target.c_str());
        }
    }

    void _Symbol_file::_Open(const path& _Target) {
        if (_Myfile.open(_Target, file_access::write) && _Myfile.resize(0)) {
            _Mystream.bind_file(_Myfile);
        } else { // report an error
            if (_Myfile.is_open()) { // file may have been opened but not resized, close it now
                _Myfile.close();
            }

            _Report_error(_Myctrs, L"(?, ?): error E4001: cannot open the symbol file '%s'", _Target.c_str());
        }
    }

    bool _Symbol_file::_Is_open() const noexcept {
        return _Mystream.is_open();
    }

    bool _Symbol_file::_Write_comment() {
        if (!_Mystream.is_open()) { // invalid stream, break
            return false;
        }

        constexpr char _Fmt[]      = "// generated by ULPCL %s on %s\n\n";
        constexpr size_t _Buf_size = 128;
        byte_t _Buf[_Buf_size]     = {'\0'}; // should accommodate every possible comment
        const int _Written         = ::snprintf(reinterpret_cast<char*>(_Buf), // negative value on error
            _Buf_size, _Fmt, _ULPCL_VERSION, _Get_current_date().c_str());
        return _Written > 0 ? _Mystream.write(_Buf, static_cast<size_t>(_Written)) : false;
    }

    bool _Symbol_file::_Write_symbol(const symbol& _Symbol, const bool _Break_line) {
        if (!_Mystream.is_open()) { // invalid stream, break
            return false;
        }

        // write serialized symbol, optionally break the line
        byte_string _Bytes = _Symbol_serializer::_Serialize(_Symbol);
        if (_Break_line) { // break the line
            _Bytes.push_back('\n');
        }

        return _Mystream.write(_Bytes);
    }

    path _Get_symbol_file_path(const unicode_string_view _Pack) {
        // make the path to the symbol file by concatenating the global output directory
        // and the pack name, then replacing the '.ulp' extension with '.sym'
        return path{program_options::current().output_directory / _Pack}.replace_extension(L".sym");
    }

    bool generate_symbol_file(
        const unicode_string_view _Pack, const vector<symbol>& _Symbols, report_counters& _Counters) {
        const path& _Path = _Get_symbol_file_path(_Pack);
        _Symbol_file _File(_Path, _Counters);
        if (!_File._Is_open()) { // failed to open/create the symbol file, break
            return false;
        }

        if (!_File._Write_comment()) { // failed to write the comment, report a warning
            _Report_warning(_Counters,
                L"(?, ?): warning W4000: cannot write comment to the symbol file '%s'", _Path.c_str());
        }

        // use index-based loop to detect whether we should break the line
        const size_t _Size = _Symbols.size();
        for (size_t _Idx = 0; _Idx < _Size; ++_Idx) {
            if (!_File._Write_symbol(_Symbols[_Idx], _Idx < _Size - 1)) {
                _Report_error(_Counters,
                    L"(?, ?): error E4002: cannot write symbol to the symbol file '%s'", _Path.c_str());
                return false;
            }
        }

        return true;
    }
} // namespace mjx