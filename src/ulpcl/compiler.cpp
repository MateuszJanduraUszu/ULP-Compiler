// compiler.cpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <mjfs/status.hpp>
#include <mjstr/conversion.hpp>
#include <type_traits>
#include <ulpcl/compiler.hpp>
#include <ulpcl/lexer.hpp>
#include <ulpcl/logger.hpp>
#include <ulpcl/program.hpp>
#include <ulpcl/runtime.hpp>
#include <xxhash/xxhash.h>

namespace mjx {
    path _Get_output_file_path(const unicode_string_view _Pack) {
        // replace '.ulp' extension with '.umc' and concat it with the output directory
        return path{program_options::current().output_directory / _Pack}.replace_extension(L".umc");
    }

    vector<message> _Get_messages_from_group(const group& _Group, const utf8_string& _Namespace) {
        vector<message> _Messages;
        _Messages.reserve(_Group.messages.size()); // pre-allocate space for the messages from this group
        for (const message& _Message : _Group.messages) { // get messages from the current group
            _Messages.push_back(message{_Namespace + _Message.id, _Message.value});
        }

        for (const group& _Child_group : _Group.groups) {
            const vector<message>& _Child_messages = _Get_messages_from_group(
                _Child_group, _Namespace + '.' + _Child_group.name); // recurse into child group
            _Messages.insert(_Messages.end(), _Child_messages.begin(), _Child_messages.end());
        }

        return ::std::move(_Messages);
    }

    vector<message> _Get_messages_from_content(const root_group& _Content) {
        vector<message> _Messages = _Content.messages;
        for (const group& _Child_group : _Content.groups) {
            const vector<message>& _Child_messages = _Get_messages_from_group(_Child_group, _Child_group.name);
            _Messages.insert(_Messages.end(), _Child_messages.begin(), _Child_messages.end());
        }

        return ::std::move(_Messages);
    }

    uint64_t _Compute_hash(const utf8_string_view _Id) noexcept {
        return ::XXH3_64bits(_Id.data(), _Id.size());
    }

    _Umc_file::_Umc_file(const path& _Target, report_counters& _Counters)
        : _Myfile(), _Mystream(), _Myctrs(_Counters) {
        if (::mjx::exists(_Target)) { // open an existing file
            _Open(_Target);
        } else { // create a new file
            _Create(_Target);
        }
    }

    _Umc_file::~_Umc_file() noexcept {}

    void _Umc_file::_Create(const path& _Target) {
        if (::mjx::create_file(_Target, ::std::addressof(_Myfile))) {
            _Mystream.bind_file(_Myfile);
        } else { // report an error
            _Report_error(_Myctrs, L"(?, ?): error E3000: cannot create the UMC file '%s'", _Target.c_str());
        }
    }

    void _Umc_file::_Open(const path& _Target) {
        if (_Myfile.open(_Target, file_access::write) && _Myfile.resize(0)) {
            _Mystream.bind_file(_Myfile);
        } else { // report an error
            if (_Myfile.is_open()) { // file may have been opened but not resized, close it now
                _Myfile.close();
            }

            _Report_error(_Myctrs, L"(?, ?): error E3001: cannot open the UMC file '%s'", _Target.c_str());
        }
    }

    bool _Umc_file::_Is_open() const noexcept {
        return _Mystream.is_open();
    }

    uint64_t _Umc_file::_Current_offset() const noexcept {
        return _Mystream.is_open() ? _Mystream.tell() : 0;
    }

    bool _Umc_file::_Write_signature() noexcept {
        if (!_Mystream.is_open()) { // stream must be open, break
            return false;
        }

        constexpr size_t _Signature_length             = 4;
        constexpr byte_t _Signature[_Signature_length] = {'U', 'M', 'C', '\0'};
        return _Mystream.write(_Signature, _Signature_length);
    }

    bool _Umc_file::_Write_language(const unicode_string_view _Language) {
        if (!_Mystream.is_open()) { // stream must be open, break
            return false;
        }
        
        // assumes that the length of _Language in UTF-8 encoding fit in 8-bit integer
        const byte_string& _Bytes = ::mjx::to_byte_string(_Language);
        const byte_t _Length      = static_cast<byte_t>(_Bytes.size());
        return _Mystream.write(&_Length, 1) && _Mystream.write(_Bytes);
    }

    bool _Umc_file::_Write_lcid(const uint32_t _Lcid) noexcept {
        if (!_Mystream.is_open()) { // stream must be open, break
            return false;
        }

        return _Mystream.write(reinterpret_cast<const byte_t*>(&_Lcid), sizeof(uint32_t));
    }

    bool _Umc_file::_Write_message_count(const uint32_t _Count) noexcept {
        if (!_Mystream.is_open()) { // stream must be open, break
            return false;
        }

        return _Mystream.write(reinterpret_cast<const byte_t*>(&_Count), sizeof(uint32_t));
    }

    bool _Umc_file::_Write_lookup_table_entry(const _Lookup_table_entry _Entry) noexcept {
        if (!_Mystream.is_open()) { // stream must be open, break
            return false;
        }

        // Note: Given that _Lookup_table_entry is aligned to 4-byte boundary without padding,
        //       it is safe to reinterpret_cast _Entry to a byte sequence. This is because
        //       _Lookup_table_entry stores integers, which can be directly converted to raw bytes.
        return _Mystream.write(reinterpret_cast<const byte_t*>(&_Entry), sizeof(_Lookup_table_entry));
    }

    bool _Umc_file::_Write_message_value(const byte_string_view _Value) noexcept {
        if (!_Mystream.is_open()) { // stream must be open, break
            return false;
        }
        
        return _Mystream.write(_Value);
    }

    _Section_writer::_Section_writer(_Umc_file& _File, const vector<message>& _Messages)
        : _Myfile(_File), _Mymsgs(_Convert_messages(_Messages)) {}

    _Section_writer::~_Section_writer() noexcept {}

    vector<_Section_writer::_Writable_message> _Section_writer::_Convert_messages(const vector<message>& _Messages) {
        vector<_Writable_message> _Writable_messages;
        _Writable_messages.reserve(_Messages.size()); // pre-allocate space for converted messages
        for (const message& _Message : _Messages) {
            _Writable_messages.push_back(
                _Writable_message{_Compute_hash(_Message.id), ::mjx::to_byte_string(_Message.value)});
        }

        return ::std::move(_Writable_messages);
    }

    bool _Section_writer::_Write_lookup_table() noexcept {
        _Lookup_table_entry _Entry;
        for (const _Writable_message& _Message : _Mymsgs) {
            _Entry._Hash   = _Message._Hash;
#ifdef _M_X64
            _Entry._Length = static_cast<uint32_t>(_Message._Value.size());
#else // ^^^ _M_X64 ^^^ / vvv _M_IX86 vvv
            _Entry._Length = _Message._Value.size();
#endif // _M_X64
            if (!_Myfile._Write_lookup_table_entry(_Entry)) { // failed to write lookup table entry, break
                return false;
            }

            _Entry._Offset += _Entry._Length;
        }

        return true;
    }

    bool _Section_writer::_Write_lookup_table(vector<symbol>& _Symbols) noexcept {
        if (_Symbols.size() < _Mymsgs.size()) { // not enough symbols, break
            return false;
        }
        
        constexpr uint64_t _Bytes_per_entry = sizeof(_Lookup_table_entry);
        uint64_t _Abs_off                   = _Myfile._Current_offset();
        _Lookup_table_entry _Entry;
        for (size_t _Idx = 0; _Idx < _Mymsgs.size(); ++_Idx) {
            const _Writable_message& _Message = _Mymsgs[_Idx];
            _Entry._Hash                      = _Message._Hash;
#ifdef _M_X64
            _Entry._Length                    = static_cast<uint32_t>(_Message._Value.size());
#else // ^^^ _M_X64 ^^^ / vvv _M_IX86 vvv
            _Entry._Length                    = _Message._Value.size();
#endif // _M_X64
            if (!_Myfile._Write_lookup_table_entry(_Entry)) { // failed to write lookup table entry, break
                return false;
            }

            _Symbols[_Idx].location.id = _Abs_off;
            _Abs_off                  += _Bytes_per_entry;
            _Entry._Offset            += _Entry._Length;
        }

        // Note: The message blob begins immediately after the lookup table, and since we have previse
        //       information about the length of each message, we can accurately calculate the location
        //       of the message values within this function.
        _Abs_off = _Myfile._Current_offset();
        for (size_t _Idx = 0; _Idx < _Mymsgs.size(); ++_Idx) {
            _Symbols[_Idx].location.value = _Abs_off;
            _Abs_off                     += _Mymsgs[_Idx]._Value.size();
        }

        return true;
    }

    bool _Section_writer::_Write_blob() noexcept {
        for (const _Writable_message& _Message : _Mymsgs) {
            if (!_Myfile._Write_message_value(_Message._Value)) { // failed to write blob entry, break
                return false;
            }
        }

        return true;
    }

    void _Allocate_symbols_and_copy_ids(vector<symbol>& _Symbols, const vector<message>& _Messages) {
        _Symbols.reserve(_Messages.size());
        for (const message& _Message : _Messages) {
            _Symbols.push_back(symbol{symbol_location{}, _Message.id}); // location isn't known yet
        }
    }

    bool _Compile_parse_tree(_Umc_file& _File, const parse_tree& _Tree, report_counters& _Counters) {
        clog(L"> Starting compilation");
        bool _Success        = true;
        const float _Elapsed = measure_invoke_duration(
            [&] {
                const vector<message>& _Messages = _Get_messages_from_content(_Tree.content);
#ifdef _M_X64
                const uint32_t _Count            = static_cast<uint32_t>(_Messages.size());
#else // ^^^ _M_X64 ^^^ / vvv _M_IX86 vvv
                const uint32_t _Count            = _Messages.size();
#endif // _M_X64
                if (!_File._Write_signature() || !_File._Write_language(_Tree.language)
                    || !_File._Write_lcid(_Tree.lcid) || !_File._Write_message_count(_Count)) {
                    // failed to write header, report an error
                    _Success = false;
                    _Report_error(_Counters, L"(?, ?): error E3002: cannot generate the UMC file header");
                    return;
                }

                _Section_writer _Writer(_File, _Messages);
                if (!_Writer._Write_lookup_table()) { // failed to write lookup table, report an error
                    _Success = false;
                    _Report_error(_Counters, L"(?, ?): error E3003: cannot generate the UMC file lookup table");
                    return;
                }

                if (!_Writer._Write_blob()) { // failed to write blob, report an error
                    _Success = false;
                    _Report_error(_Counters, L"(?, ?): error E3004: cannot generate the UMC file blob");
                }
            }
        );
        if (!_Success) { // something went wrong, break
            return false;
        }

        clog(L"> Completed compilation (took %.5fs)", _Elapsed);
        return true;
    }

    bool _Compile_parse_tree_and_generate_symbols(
        _Umc_file& _File, const parse_tree& _Tree, vector<symbol>& _Symbols, report_counters& _Counters) {
        clog(L"> Starting compilation");
        bool _Success        = true;
        const float _Elapsed = measure_invoke_duration(
            [&] {
                const vector<message>& _Messages = _Get_messages_from_content(_Tree.content);
#ifdef _M_X64
                const uint32_t _Count            = static_cast<uint32_t>(_Messages.size());
#else // ^^^ _M_X64 ^^^ / vvv _M_IX86 vvv
                const uint32_t _Count            = _Messages.size();
#endif // _M_X64
                if (!_File._Write_signature() || !_File._Write_language(_Tree.language)
                    || !_File._Write_lcid(_Tree.lcid) || !_File._Write_message_count(_Count)) {
                    // failed to write header, report an error
                    _Success = false;
                    _Report_error(_Counters, L"(?, ?): error E3002: cannot generate the UMC file header");
                    return;
                }

                _Allocate_symbols_and_copy_ids(_Symbols, _Messages);
                _Section_writer _Writer(_File, _Messages);
                if (!_Writer._Write_lookup_table(_Symbols)) { // failed to write lookup table, report an error
                    _Success = false;
                    _Report_error(_Counters, L"(?, ?): error E3003: cannot generate the UMC file lookup table");
                    return;
                }

                if (!_Writer._Write_blob()) { // failed to write blob, report an error
                    _Success = false;
                    _Report_error(_Counters, L"(?, ?): error E3004: cannot generate the UMC file blob");
                }
            }
        );
        if (!_Success) { // something went wrong, break
            return false;
        }

        clog(L"> Completed compilation (took %.5fs)", _Elapsed);
        return true;
    }

    bool compile_input_file(const path& _Target) {
        clog(L"\nPack: '%s'", _Target.native().c_str());
        report_counters _Counters;
        const unicode_string _Pack = _Target.filename().native();
        const path& _Path          = _Get_output_file_path(_Pack);
        bool _Success              = true;
        const float _Elapsed       = measure_invoke_duration(
            [&] {
                const auto& [_Analyzed, _Stream] = analyze_input_file(_Target, _Counters);
                if (!_Analyzed) { // lexical analysis failed, break
                    _Success = false;
                    return;
                }

                const auto& [_Parsed, _Tree] = parse_token_stream(_Stream, _Pack, _Counters);
                if (!_Parsed) { // parse failed, break
                    _Success = false;
                    return;
                }

                _Umc_file _File(_Path, _Counters);
                if (!_File._Is_open()) { // failed to open or create the UMC file, break
                    _Success = false;
                    return;
                }

                if (program_options::current().generate_symbol_file) { // generate symbols
                    vector<symbol> _Symbols;
                    if (!_Compile_parse_tree_and_generate_symbols(_File, _Tree, _Symbols, _Counters)) {
                        // failed to compile parse tree and generate symbols, break
                        _Success = false;
                        return;
                    }

                    if (!generate_symbol_file(_Pack, _Symbols, _Counters)) { // failed to generate symbol file
                        _Success = false;
                    }
                } else { // don't generate symbols
                    if (!_Compile_parse_tree(_File, _Tree, _Counters)) { // failed to compile the parse tree
                        _Success = false;
                    }
                }
            }
        );
        if (_Success) { // report success
            clog(L"----- Generated '%s'", _Path.c_str());
            clog(L"----- Compilation succeeded (took %.5fs)", _Elapsed);
        } else { // report failure
            // choose singular or plural depending on the number of errors and warnings
            const wchar_t* const _Ex = _Counters.errors == 1 ? L"error" : L"errors";
            const wchar_t* const _Wx = _Counters.warnings == 1 ? L"warning" : L"warnings";
            clog(L"----- Compilation failed, %zu %s, %zu %s", _Counters.errors, _Ex, _Counters.warnings, _Wx);
        }

        notify_compilation_finish();
        return _Success;
    }
} // namespace mjx