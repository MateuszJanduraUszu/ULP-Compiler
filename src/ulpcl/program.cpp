// program.cpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <mjfs/directory.hpp>
#include <mjfs/status.hpp>
#include <mjsync/thread.hpp>
#include <ulpcl/logger.hpp>
#include <ulpcl/program.hpp>
#include <type_traits>

namespace mjx {
    program_options& program_options::current() noexcept {
        static program_options _Options;
        return _Options;
    }

    bool _Threads_option_traits::_Is_thread_count_supported(const size_t _Count) noexcept {
        switch (_Count) {
        case 1:
        case 2:
        case 4:
        case 8:
            return true;
        default:
            return false;
        }
    }

    bool _Is_input_file_included(const path& _Path) noexcept {
        for (const path& _Input_file : program_options::current().input_files) {
            if (_Input_file == _Path) { // searched file found, break
                return true;
            }
        }

        return false; // not included
    }

    size_t _Clamp_thread_count(size_t _Count) noexcept {
        // clamp _Count to the highest valid value that is not greater than the total number of threads
        const size_t _Max = ::mjx::hardware_concurrency();
        while (_Count > _Max) {
            // continue dividing _Count by two (right-shifting by one) until it is less than or equal to
            // the total number of threads; the valid thread count options are powers of two:
            // 1, 2, 4 and 8, each being twice the previous count
            _Count >>= 1;
        }

        return _Count;
    }

    size_t _Choose_thread_count(const size_t _Input_files) noexcept {
        // choose the number of threads based on the number of input files and the total number of threads
        size_t _Count;
        if (_Input_files <= 4) {
            _Count = 1;
        } else if (_Input_files <= 8) {
            _Count = 2;
        } else if (_Input_files <= 16) {
            _Count = 4;
        } else {
            _Count = 8;
        }

        return _Clamp_thread_count(_Count);
    }

    void _Options_parser::_Parse_input_file(const unicode_string_view _Value) {
        vector<path>& _Input_files = program_options::current().input_files;
        path _Path                 = _Value;
        if (_Is_input_file_included(_Path)) { // already specified
            rtlog(L"Warning: The input file '%s' specified more than once.", _Value.data());
            return;
        }

        if (!::mjx::exists(_Path)) { // specified non-existent file
            rtlog(L"Warning: The input file '%s' does not exist, ignored.", _Value.data());
            return;
        }

        if (_Path.extension() != L".ulp") { // specified not recognized file
            rtlog(L"Warning: The input file '%s' has invalid extension, ignored.", _Value.data());
            return;
        }

        _Input_files.push_back(::std::move(_Path));
    }

    void _Options_parser::_Parse_input_directory(const unicode_string_view _Value) {
        vector<path>& _Input_files = program_options::current().input_files;
        const path _Path           = _Value;
        if (!::mjx::exists(_Path)) { // specified non-existent directory
            rtlog(L"Warning: The input directory '%s' does not exist, ignored.", _Value.data());
            return;
        }

        if (!::mjx::is_directory(_Path)) { // specified non-directory
            rtlog(L"Warning: The input directory '%s' is not a directory, ignored.", _Value.data());
            return;
        }

        for (const directory_entry& _Entry : directory_iterator(_Path)) {
            const path& _Input_file = _Entry.absolute_path();
            if (_Input_file.extension() == L".ulp") { // found input file
                if (!_Is_input_file_included(_Input_file)) { // input file not included, include it
                    _Input_files.push_back(_Input_file);
                } else { // input file already 
                    rtlog(L"Warning: The input file '%s' already specified, ignored.", _Input_file.c_str());
                }
            }
        }
    }

    void _Options_parser::_Parse_output_directory(const unicode_string_view _Value) {
        path& _Outdir = program_options::current().output_directory;
        if (_Outdir.empty()) { // set the output directory
            path _Path = _Value;
            if (::mjx::exists(_Path)) { // specified existing directory, validate it
                if (!::mjx::is_directory(_Path)) { // specified non-directory, break
                    rtlog(L"Warning: The output directory '%s' is not a directory, ignored", _Value.data());
                    return;
                }
            } else { // specified non-existent directory, try to create it
                if (!::mjx::create_directory(_Path)) { // failed to create a directory, break
                    rtlog(L"Warning: Failed to create the output directory '%s', ignored", _Value.data());
                    return;
                }
            }

            _Outdir = ::std::move(_Path);
        } else { // the output directory already specified
            rtlog(L"Warning: Output directory specified more than once, ignored.");
        }
    }

    void _Options_parser::_Parse_threads(const unicode_string_view _Value) noexcept {
        size_t& _Threads = program_options::current().threads;
        if (_Threads == _Threads_option_traits::_Unknown) {
            if (_Value == L"disable") { // disable multithreading
                _Threads = _Threads_option_traits::_Disabled;
            } else if (_Value == L"auto") { // set default number of threads
                _Threads = _Threads_option_traits::_Auto;
            } else { // set requested number of threads
                if (_Value.size() != 1) { // invalid number of threads, report a warning and break
                    rtlog(L"Warning: Invalid number of threads, ignored");
                    return;
                }

                size_t _Count = static_cast<size_t>(_Value.front() - L'0');
                if (_Threads_option_traits::_Is_thread_count_supported(_Count)) {
                    if (_Count > ::mjx::hardware_concurrency()) { // requested too many threads, trim
                        _Count = _Clamp_thread_count(_Count);
                        rtlog(L"Warning: Requested too many threads, trimmed to %zu", _Count);
                    }

                    _Threads = _Count;
                } else { // not supported number of threads
                    rtlog(L"Warning: Requested number of threads is not supported, ignored");
                }
            }
        } else { // the number of threads already specified
            rtlog(L"Warning: Number of threads specified more than once, ignored.");
        }
    }

    void _Options_parser::_Parse_error_model(const unicode_string_view _Value) noexcept {
        error_model& _Model = program_options::current().model;
        if (_Model == error_model::unknown) { // set the error model
            if (_Value == L"soft" || _Value == L"default") {
                _Model = error_model::soft;
            } else if (_Value == L"strict") {
                _Model = error_model::strict;
            } else {
                rtlog(L"Warning: Unsupported error model, ignored.");
            }
        } else { // the error model already specified
            rtlog(L"Warning: Error model specified more than once, ignored.");
        }
    }

    void parse_program_args(int _Count, wchar_t** _Args) {
        program_options& _Options = program_options::current();
        bool _Verbose             = false;
        unicode_string_view _Arg;
        size_t _Eq_pos;
        for (; _Count > 0; --_Count, ++_Args) {
            _Arg    = *_Args;
            _Eq_pos = _Arg.find(L'=');
            if (_Eq_pos == unicode_string_view::npos) { // parse an argument without value
                if (_Arg == L"--discard-empty" || _Arg == L"-d") { // discard empty messages
                    _Options.discard_empty_messages = true;
                } else if (_Arg == L"--symbol-file" || _Arg == L"-s") { // generate symbol file
                    _Options.generate_symbol_file = true;
                } else if (_Arg == L"--verbose" || _Arg == L"-V") { // enable detailed logging
                    _Verbose = true;
                } else { // unrecognized option
                    rtlog(L"Warning: Unrecognized option '%s', ignored.", _Arg.data());
                }
            } else { // parse an argument with value
                if (_Eq_pos == 0 || _Eq_pos == _Arg.size() - 1) { // invalid option
                    rtlog(L"Warning: Invalid option '%s', ignored.", _Arg.data());
                    continue;
                }

                const unicode_string_view _Option = _Arg.substr(0, _Eq_pos);
                const unicode_string_view _Value  = _Arg.substr(_Eq_pos + 1);
                if (_Option == L"--input") { // include an input file
                    _Options_parser::_Parse_input_file(_Value);
                } else if (_Option == L"--input-dir") { // include an input directory
                    _Options_parser::_Parse_input_directory(_Value);
                } else if (_Option == L"--output-dir") { // set the output directory
                    _Options_parser::_Parse_output_directory(_Value);
                } else if (_Option == L"--threads") { // set the number of threads
                    _Options_parser::_Parse_threads(_Value);
                } else if (_Option == L"--error-model") { // set the error model
                    _Options_parser::_Parse_error_model(_Value);
                } else {
                    rtlog(L"Warning: Unrecognized option '%s', ignored.", _Arg.data());
                }
            }
        }

        if (_Options.output_directory.empty()) { // set the default output directory
            _Options.output_directory = ::mjx::current_path();
        }

        if (_Options.threads == _Threads_option_traits::_Unknown) { // set the default number of threads
            _Options.threads = _Threads_option_traits::_Disabled;
        } else if (_Options.threads == _Threads_option_traits::_Auto) { // choose the number of threads
            _Options.threads = _Choose_thread_count(_Options.input_files.size());
        }

        if (_Options.model == error_model::unknown) { // set the default error model
            _Options.model = error_model::soft;
        }

        if (_Verbose) { // startup compilation logger
            compilation_logger::current().startup();
        }
    }
} // namespace mjx