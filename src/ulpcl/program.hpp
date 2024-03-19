// program.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _ULPCL_PROGRAM_HPP_
#define _ULPCL_PROGRAM_HPP_
#include <cstddef>
#include <mjfs/path.hpp>
#include <mjstr/string_view.hpp>
#include <ulpcl/utils.hpp>

namespace mjx {
    enum class error_model : unsigned char {
        unknown,
        soft,
        strict
    };

    struct _Threads_option_traits { // traits for the '--threads' option
        static constexpr size_t _Unknown  = static_cast<size_t>(-1);
        static constexpr size_t _Auto     = static_cast<size_t>(-2);
        static constexpr size_t _Disabled = 0;

        // checks if the specified number of threads is supported
        static bool _Is_thread_count_supported(const size_t _Count) noexcept;
    };

    class program_options {
    public:
        vector<path> input_files;
        path output_directory;
        size_t threads              = _Threads_option_traits::_Unknown;
        error_model model           = error_model::unknown;
        bool discard_empty_messages = false;
        bool generate_symbol_file   = false;
    
        // returns the global instance of the program options
        static program_options& current() noexcept;
    };

    bool _Is_input_file_included(const path& _Path) noexcept;
    path _Absolute_path(const path& _Path);
    size_t _Clamp_thread_count(size_t _Count) noexcept;
    size_t _Choose_thread_count(const size_t _Input_files) noexcept;

    struct _Options_parser {
        // parses '--input' option
        static void _Parse_input_file(const unicode_string_view _Value);

        // parses '--input-dir' option
        static void _Parse_input_directory(const unicode_string_view _Value);

        // parses '--ouput-dir' option
        static void _Parse_output_directory(const unicode_string_view _Value);

        // parses '--threads' option
        static void _Parse_threads(const unicode_string_view _Value) noexcept;

        // parses '--error-model' option
        static void _Parse_error_model(const unicode_string_view _Value) noexcept;
    };

    void parse_program_args(int _Count, wchar_t** _Args);
} // namespace mjx

#endif // _ULPCL_PROGRAM_HPP_