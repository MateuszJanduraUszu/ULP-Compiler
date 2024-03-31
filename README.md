# ULP Compiler

The ULP Compiler (ULPCL) is a C++20 tool designed to compile **.ulp** files into **.umc** files.

## Setup

1. Download the appropriate package based on your system architecture:

    * For 64-bit systems, download `Bin-x64.zip`.
    * For 32-bit systems, download `Bin-x86.zip`.

2. Extract the dowloaded package. Upon extraction, you should find the following files:

    * `ulpcl.exe`
    * `mjfs.dll`
    * `mjmem.dll`
    * `mjstr.dll`
    * `mjsync.dll`
    * `xxhash.dll`

## How to build

To successfully build the project, follow these steps:

1. Ensure that you have CMake and a compiler known to CMake properly installed.
2. Clone the repository using the following command

```
git clone https://github.com/MateuszJanduraUszu/ULP-Compiler.git
```

3. Build the `ulpcl` executable:

```
build.bat {x64|Win32} "{Compiler}"
```

These steps will help you compile the project's executable using the specified platform architecture and compiler.

## Usage

To learn how to use ULPCL, refer to the [documentation](https://github.com/MateuszJanduraUszu/ULP-Compiler/tree/main/docs).

## Compatibility

The ULPCL is designed to operate exclusively on Windows systems. It's essential that the system version is compatible with all modules
utilized in the project, which include:

* [MJFS](https://github.com/MateuszJanduraUszu/mjfs)
* [MJMEM](https://github.com/MateuszJanduraUszu/mjmem)
* [MJSTR](https://github.com/MateuszJanduraUszu/mjstr)
* [MJSYNC](https://github.com/MateuszJanduraUszu/mjsync)
* [xxHash](https://github.com/Cyan4973/xxHash)

## Questions and support

If you have any questions, encounter issues, or need assistance with the MJSYNC, feel free to reach out. You can reach me through the
`Issues` section or email ([mjandura03@gmail.com](mailto:mjandura03@gmail.com)).

## License

Copyright © Mateusz Jandura.

SPDX-License-Identifier: Apache-2.0