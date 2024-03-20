# CMakeLists.txt

# Copyright (c) Mateusz Jandura. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.21)
project(ulpcl
    VERSION 1.0.0
    DESCRIPTION "ULP Compiler"
    LANGUAGES CXX
)

set(CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)

# translate x64/Win32 into x64/x86
if(CMAKE_GENERATOR_PLATFORM STREQUAL x64)
    set(ULPCL_PLATFORM_ARCH x64)
elseif(CMAKE_GENERATOR_PLATFORM STREQUAL Win32)
    set(ULPCL_PLATFORM_ARCH x86)
else()
    set(ULPCL_PLATFORM_ARCH Invalid)
    message(FATAL_ERROR "Requires either x64 or Win32 platform architecture.")
endif()

set(CMAKE_SUPPRESS_REGENERATION TRUE)
if(MSVC)
    set(VS_SOURCE_GROUPS src)
endif()

set(ULPCL_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(ULPCL_SOURCES
    "${ULPCL_SRC_DIR}/ulpcl/compiler.cpp"
    "${ULPCL_SRC_DIR}/ulpcl/compiler.hpp"
    "${ULPCL_SRC_DIR}/ulpcl/dispatcher.cpp"
    "${ULPCL_SRC_DIR}/ulpcl/dispatcher.hpp"
    "${ULPCL_SRC_DIR}/ulpcl/keyword.cpp"
    "${ULPCL_SRC_DIR}/ulpcl/keyword.hpp"
    "${ULPCL_SRC_DIR}/ulpcl/lexer.cpp"
    "${ULPCL_SRC_DIR}/ulpcl/lexer.hpp"
    "${ULPCL_SRC_DIR}/ulpcl/logger.cpp"
    "${ULPCL_SRC_DIR}/ulpcl/logger.hpp"
    "${ULPCL_SRC_DIR}/ulpcl/main.cpp"
    "${ULPCL_SRC_DIR}/ulpcl/parser.cpp"
    "${ULPCL_SRC_DIR}/ulpcl/parser.hpp"
    "${ULPCL_SRC_DIR}/ulpcl/program.cpp"
    "${ULPCL_SRC_DIR}/ulpcl/program.hpp"
    "${ULPCL_SRC_DIR}/ulpcl/runtime.cpp"
    "${ULPCL_SRC_DIR}/ulpcl/runtime.hpp"
    "${ULPCL_SRC_DIR}/ulpcl/symbol_file.cpp"
    "${ULPCL_SRC_DIR}/ulpcl/symbol_file.hpp"
    "${ULPCL_SRC_DIR}/ulpcl/tinywin.hpp"
    "${ULPCL_SRC_DIR}/ulpcl/utils.hpp"
    "${ULPCL_SRC_DIR}/ulpcl/version.hpp"
)

# put all source files in 'src' directory
source_group("src" FILES ${ULPCL_SOURCES})

# put the compiled executable in either 'bin\Debug' or 'bin\Release' directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/")

add_executable(ulpcl ${ULPCL_SOURCES})

target_compile_features(ulpcl PRIVATE cxx_std_20)
target_include_directories(ulpcl PRIVATE
    "${ULPCL_SRC_DIR}"
    "${ULPCL_SRC_DIR}/thirdparty/MJFS/inc/"
    "${ULPCL_SRC_DIR}/thirdparty/MJMEM/inc/"
    "${ULPCL_SRC_DIR}/thirdparty/MJSTR/inc/"
    "${ULPCL_SRC_DIR}/thirdparty/MJSYNC/inc/"
    "${ULPCL_SRC_DIR}/thirdparty/xxHash/inc/${ULPCL_PLATFORM_ARCH}/"
)
target_link_libraries(ulpcl PRIVATE
    # link MJFS
    $<$<CONFIG:Debug>:${ULPCL_SRC_DIR}/thirdparty/MJFS/bin/${ULPCL_PLATFORM_ARCH}/Debug/mjfs.lib>
    $<$<CONFIG:Release>:${ULPCL_SRC_DIR}/thirdparty/MJFS/bin/${ULPCL_PLATFORM_ARCH}/Release/mjfs.lib>

    # link MJMEM
    $<$<CONFIG:Debug>:${ULPCL_SRC_DIR}/thirdparty/MJMEM/bin/${ULPCL_PLATFORM_ARCH}/Debug/mjmem.lib>
    $<$<CONFIG:Release>:${ULPCL_SRC_DIR}/thirdparty/MJMEM/bin/${ULPCL_PLATFORM_ARCH}/Release/mjmem.lib>

    # link MJSTR
    $<$<CONFIG:Debug>:${ULPCL_SRC_DIR}/thirdparty/MJSTR/bin/${ULPCL_PLATFORM_ARCH}/Debug/mjstr.lib>
    $<$<CONFIG:Release>:${ULPCL_SRC_DIR}/thirdparty/MJSTR/bin/${ULPCL_PLATFORM_ARCH}/Release/mjstr.lib>

    # link MJSYNC
    $<$<CONFIG:Debug>:${ULPCL_SRC_DIR}/thirdparty/MJSYNC/bin/${ULPCL_PLATFORM_ARCH}/Debug/mjsync.lib>
    $<$<CONFIG:Release>:${ULPCL_SRC_DIR}/thirdparty/MJSYNC/bin/${ULPCL_PLATFORM_ARCH}/Release/mjsync.lib>

    # link xxHash
    $<$<CONFIG:Debug>:${ULPCL_SRC_DIR}/thirdparty/xxHash/bin/${ULPCL_PLATFORM_ARCH}/Debug/xxhash.lib>
    $<$<CONFIG:Release>:${ULPCL_SRC_DIR}/thirdparty/xxHash/bin/${ULPCL_PLATFORM_ARCH}/Release/xxhash.lib>
)