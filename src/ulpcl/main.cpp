// main.cpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <mjstr/char_traits.hpp>
#include <ulpcl/dispatcher.hpp>
#include <ulpcl/logger.hpp>
#include <ulpcl/program.hpp>
#include <ulpcl/runtime.hpp>
#include <ulpcl/version.hpp>

namespace mjx {
    enum class _Help_or_version : unsigned char {
        _Neither,
        _Help,
        _Version
    };

    inline _Help_or_version _Scan_args_for_help_or_version(int _Count, wchar_t** _Args) noexcept {
        using _Traits = char_traits<wchar_t>;
        for (; _Count > 0; --_Count, ++_Args) {
            if (_Traits::eq(*_Args, L"--help", 6) || _Traits::eq(*_Args, L"-h", 2)) { // help flag found
                return _Help_or_version::_Help;
            } else if (_Traits::eq(*_Args, L"--version", 9) || _Traits::eq(*_Args, L"-v", 2)) { // version flag found
                return _Help_or_version::_Version;
            }
        }

        return _Help_or_version::_Neither;
    }

    inline void _Print_help() noexcept {
        rtlog(
            L"ULPCL usage:\n"
            L"\n"
            L"ulpcl.exe [options...]\n"
            L"ulpcl.exe --help\n"
            L"\n"
            L"Options:\n"
            L"    --help (or -h)            display this help message and exit\n"
            L"    --version (or -v)         print the ULPCL version and exit\n"
            L"    --verbose (or -V)         enable detailed logging\n"
            L"\n"
            L"    --input=\"[...]\"           compile the specified file (absolute path)\n"
            L"    --input-dir=\"[...]\"       compile files from the specified directory (absolute path)\n"
            L"    --output-dir=\"[...]\"      set the output directory for compiled files (absolute path)\n"
            L"\n"
            L"    --error-model=[...]       define how errors and warnings impact the compilation process\n"
            L"        soft                      halt on errors but allow warnings\n"
            L"        strict                    halt on both errors and warnings\n"
            L"        default                   alias for 'soft'\n"
            L"\n"
            L"    --threads=[...]           specify multithreading during compilation\n"
            L"        disable                   disable multithreading\n"
            L"        auto                      allow multithreading (automatically adjusted number of threads)\n"
            L"        <number>                  allow multithreading (user-specified number of threads,"
            L" limited to: 1, 2, 4, 8)\n"
            L"\n"
            L"    --discard-empty (or -d)   discard messages that have no values\n"
            L"    --symbol-file (or -s)     generates a symbol file for each input file"
        );
    }

    inline void _Print_version() noexcept {
        rtlog(L"ULPCL version %s compiled on %s", _UNICODE_ULPCL_VERSION, _UNICODE_ULPCL_COMPILATION_DATE);
    }

    inline bool _Should_invoke_entry_point(int _Count, wchar_t** _Args) noexcept {
        if (_Count <= 1) { // no user-provided arguments, print help and exit
            _Print_help();
            return false;
        }

        switch (_Scan_args_for_help_or_version(_Count, _Args)) {
        case _Help_or_version::_Help:
            _Print_help();
            break;
        case _Help_or_version::_Version:
            _Print_version();
            break;
        default:
            return true;
        }

        return false;
    }

    inline void _Start_build() {
        rtlog(L"Build started at %s...", get_current_time().c_str());
        compilation_counters _Counters;
        const float _Elapsed = measure_invoke_duration(
            [&_Counters] {
                // dispatch compilation for each input file and wait until the entire compilation is completed
                compilation_dispatcher _Dispatcher;
                for (const path& _Input_file : program_options::current().input_files) {
                    _Dispatcher.dispatch(_Input_file);
                }

                _Dispatcher.wait_for_completion();
                _Counters = _Dispatcher.counters();
            }
        );
        rtlog(L"\n----- Build: %zu succeeded, %zu failed", _Counters.succeeded, _Counters.failed);
        rtlog(L"----- Build completed at %s (took %.5fs)", get_current_time().c_str(), _Elapsed);
    }

    inline void _Unsafe_entry_point() {
        program_options& _Options = program_options::current();
        if (_Options.input_files.empty()) { // no input files specified
            rtlog(L"Warning: No input files specified");
            return;
        }

        _Start_build();
    }
} // namespace mjx

int wmain(int _Count, wchar_t** _Args) {
    try {
        if (!::mjx::_Should_invoke_entry_point(_Count, _Args)) {
            return 0;
        }

        // skip the first argument, which is always the path or name of the executable file
        ::mjx::parse_program_args(_Count - 1, _Args + 1);
        ::mjx::_Unsafe_entry_point();
        return 0;
    } catch (const ::mjx::allocation_failure&) {
        ::mjx::rtlog(L"Error: Insufficient memory to complete the operation");
        return -1;
    } catch (...) {
        ::mjx::rtlog(L"Error: An unknown error occurred");
        return -2;
    }
}