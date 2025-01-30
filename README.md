# C++ library for application development

Includes:

- [fcgi] - FastCGI implementation (server);
- [http] - HTTP parts implementation (with basic client and server);
- [jrpc] - [JSON-RPC 2.0][json-rpc2] implementation;
- [pgfe] - [PostgreSQL] driver;
- [rajson] - [RapidJSON] thin wrapper;
- [sqlixx] - [SQLite] driver;
- [ws] - WebSocket server;
- [wscl] - WebSocket client;
- miscellaneous stuff (from basic utilities to URL processing to concurrency).

All of these libraries can be used as shared libraries, static libraries or
header-only libraries.

## Third-party dependencies

### Unbundled third-party dependencies

- [CMake] build system version 3.16+;
- C++17 compiler ([GCC] 7.4+ or [Microsoft Visual C++][Visual_Studio] 15.7+);
- [libpq] library for [pgfe];
- [libuv] library for [ws];
- [OpenSSL] library (optionally) for [ws];
- [SQLite] for [sqlixx];
- [zlib] library (optionally) for [ws].

### Bundled third-party dependencies

- [Portable endian];
- [RapidJSON];
- [uSockets];
- [uWebSockets];
- [uwsc].

## CMake options

The table below (one may need to use horizontal scrolling for full view)
contains variables which can be passed to [CMake] for customization.

|Variable                        |Range |Default    |
|:-------------------------------|:-----|:----------|
|**Common options**|||
|DMITIGR_LIBS_HEADER_ONLY        |Bool  |Off        |
|DMITIGR_LIBS_DOXYGEN            |Bool  |Off        |
|DMITIGR_LIBS_TESTS              |Bool  |Off        |
|DMITIGR_LIBS_OPENSSL            |Bool  |Off        |
|DMITIGR_LIBS_ZLIB               |Bool  |Off        |
|DMITIGR_LIBS_AIO                |"uv"  |"uv"       |
|DMITIGR_LIBS_CLANG_USE_LIBCPP   |Bool  |On         |
|CMAKE_BUILD_TYPE                |String|Release    |
|BUILD_SHARED_LIBS               |Bool  |Off        |
|**Installation options**|||
|DMITIGR_LIBS_INSTALL            |Bool  |On         |
|CMAKE_INSTALL_PREFIX            |PATH  |See remarks|
|DMITIGR_LIBS_SHARE_INSTALL_DIR  |PATH  |See remarks|
|DMITIGR_LIBS_CMAKE_INSTALL_DIR  |PATH  |See remarks|
|DMITIGR_LIBS_LIB_INSTALL_DIR    |PATH  |See remarks|
|DMITIGR_LIBS_INCLUDE_INSTALL_DIR|PATH  |See remarks|
|**Dependencies options**|||
|Pq_ROOT                         |PATH  |*undefined*|
|Uv_ROOT                         |PATH  |*undefined*|
|OpenSSL_ROOT                    |PATH  |*undefined*|
|SQLite3_ROOT                    |PATH  |*undefined*|
|zlib_ROOT                       |PATH  |*undefined*|

### Remarks

- `<Lib>_ROOT` specifies a prefix for both binary and headers of the
  `<Lib>`. For example, if [PostgreSQL] installed relocatably into
  `/usr/local/pgsql`, the value of `Pq_ROOT` should be set accordingly;
- when building with Microsoft Visual Studio the value of `CMAKE_BUILD_TYPE`
  doesn't selects the build configuration within the generated build environment.
  The [CMake] command line option `--config` should be used for that purpose.
- `CMAKE_INSTALL_PREFIX` defaulted to "/usr/local" on Unix and
  "%ProgramFiles%\dmitigr_libs" on Windows;
- `DMITIGR_LIBS_SHARE_INSTALL_DIR` defaulted to "share/dmitigr_libs" on
  Unix and "." on Windows. (A path relative to `CMAKE_INSTALL_PREFIX`;)
- `DMITIGR_LIBS_CMAKE_INSTALL_DIR` defaulted to
  "${DMITIGR_LIBS_SHARE_INSTALL_DIR}/cmake" on Unix
  and "cmake" on Windows. (A path relative to `CMAKE_INSTALL_PREFIX`;)
- `DMITIGR_LIBS_LIB_INSTALL_DIR` defaulted to "lib" on both Unix and Windows.
  (A path relative to `CMAKE_INSTALL_PREFIX`;)
- `DMITIGR_LIBS_INCLUDE_INSTALL_DIR` defaulted to "include" on both Unix and
  Windows. (A path relative to `CMAKE_INSTALL_PREFIX`.)

## Installation

Library package can be installed as a set of:

  - shared libraries if `-DBUILD_SHARED_LIBS=ON` option is specified;
  - static libraries if `-DBUILD_SHARED_LIBS=OFF` option is specified
    (the default);
  - header-only libraries if `-DDMITIGR_LIBS_HEADER_ONLY=ON` option
    is specified.

The default build type is *Debug*.

### Installation on Linux

    $ git clone https://github.com/dmitigr/libapp.git
    $ mkdir libapp/build
    $ cd libapp/build
    $ cmake ..
    $ cmake --build . --parallel
    $ cmake --install .

### Installation on Microsoft Windows

Run the Developer Command Prompt for Visual Studio and type:

    > git clone https://github.com/dmitigr/libapp.git
    > mkdir libapp\build
    > cd libapp\build
    > cmake -A x64 ..
    > cmake --build . --config Release --parallel

Next, run the elevated command prompt (i.e. the command prompt with
administrator privileges) and type:

    > cmake --install .

Alternatively, one of the following build commands may be used:
    > cmake --build . --config Release --target install
or
    > cmake -DBUILD_TYPE=Release -P cmake_install.cmake

**A bitness of the target architecture must corresponds to the bitness
of external dependencies!**

To make installed DLLs available for *any* application that depends on it,
symbolic links can be created:

  - in `%SYSTEMROOT%\System32` for a 64-bit DLL on a 64-bit host
    (or for the 32-bit DLL on the 32-bit host);
  - in `%SYSTEMROOT%\SysWOW64` for the 32-bit DLL on 64-bit host.

For example, to create the symbolic link to `dmitigr_pgfe.dll`, the `mklink`
command can be used in the elevated command prompt:

    > cd /d %SYSTEMROOT%\System32
    > mklink dmitigr_pgfe.dll "%ProgramFiles%\dmitigr_libs\lib\dmitigr_pgfe.dll"

## Usage

Assuming `foo` is the name of library, the following considerations should be
followed:

  - headers other than `dmitigr/foo/foo.hpp` should *not* be used
    since these headers are subject to reorganize;
  - namespace `dmitigr::foo::detail` should *not* be used directly
    since it consists of the implementation details.

### Usage with CMake

With [CMake] it's pretty easy to use the libraries (including derived versions)
in two ways: as a system-wide installed library(-es) or as a library(-es) dropped
into the project source directory.

The code below demonstrates how to import system-wide installed libraries by using
[CMake] (this snippet is also valid when using the derived library package(s)):

```cmake
cmake_minimum_required(VERSION 3.16)
project(foo)
find_package(dmitigr_libs REQUIRED COMPONENTS fcgi pgfe)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(foo foo.cpp)
target_link_libraries(foo dmitigr_fcgi dmitigr_pgfe)
```

The next code demonstrates how to import the derived [Pgfe][dmitigr_pgfe]
library package dropped directly into the project's source directory `3rdparty/pgfe`:

```cmake
set(DMITIGR_LIBS_HEADER_ONLY ON CACHE BOOL "Header-only?")
add_subdirectory(3rdparty/pgfe)
```

Note, that all CMake variables described in [CMake options](#cmake-options) are
also valid for derived library packages.

#### Specifying a library type to use

It's possible to explicitly specify a type of library to use. To do it,
the corresponding suffix of a component name should be specified:

  - the suffix "_shared" corresponds to shared libraries;
  - the suffix "_static" corresponds to static libraries;
  - the suffix "_interface" corresponds to header-only libraries.

For example, the code below demonstrates how to use the shared [fcgi] library
and the header-only [pgfe] library in a same project side by side:

```cmake
find_package(dmitigr_libs REQUIRED COMPONENTS fcgi_shared pgfe_interface)
# ...
target_link_libraries(foo dmitigr_fcgi dmitigr_pgfe)
```

**Note that libraries of the explicitly specified types must be installed
to be found!**

If the type of library is not specified (i.e. suffix of a component name is
omitted), [find_package()][CMake_find_package] will try to import the first
available library in the following order:

  1. a shared library;
  2. a static library;
  3. a header-only library.

### Usage without CMake

It's possible to use the libraries without [CMake]. In order to use an any
library of the package as header-only library, just copy the contents of the
`src` directory to a project directory which is under an include path of a
compiler, for example, `src/3rdparty/dmitigr`. Then, just include the header
of the required library, for example:

```cpp
#include "dmitigr/pgfe/pgfe.hpp"

int main()
{
  dmitigr::pgfe::Connection conn;
}
```

Please note, that external dependencies must be linked manually in this case. So,
the snippet above could be compiled with the following command:

```
g++ -std=c++17 -I/usr/local/pgsql/include -L/usr/local/pgsql/lib -lpq -ohello hello.cpp
```

## Licenses and copyrights

All the libraries of the package itself (except the third-party software) is
distributed under [APACHE LICENSE, VERSION 2.0][Apache_2_LICENSE].

Third-party software are distributed under:

  - [Portable endian] is in the public domain;
  - [RapidJSON] is distributed under the following [license][RapidJSON_LICENSE];
  - [uSockets] is distributed under the following [license][uSockets_LICENSE];
  - [uWebSockets] is distributed under the following [license][uWebSockets_LICENSE];
  - [uwsc] is distributed under the following [license][uwsc_LICENSE].

For conditions of distribution and use, please see the corresponding license.

[Apache_2_LICENSE]: https://www.apache.org/licenses/LICENSE-2.0

[fcgi]: https://github.com/dmitigr/libapp/tree/main/src/dmitigr/fcgi
[http]: https://github.com/dmitigr/libapp/tree/main/src/dmitigr/http
[jrpc]: https://github.com/dmitigr/libapp/tree/main/src/dmitigr/jrpc
[pgfe]: https://github.com/dmitigr/libapp/tree/main/src/dmitigr/pgfe
[rajson]: https://github.com/dmitigr/libapp/tree/main/src/dmitigr/rajson
[sqlixx]: https://github.com/dmitigr/libapp/tree/main/src/dmitigr/sqlixx
[ws]: https://github.com/dmitigr/libapp/tree/main/src/dmitigr/ws
[wscl]: https://github.com/dmitigr/libapp/tree/main/src/dmitigr/wscl

[CMake]: https://cmake.org/
[CMake_find_package]: https://cmake.org/cmake/help/latest/command/find_package.html
[GCC]: https://gcc.gnu.org/
[json-rpc2]: https://www.jsonrpc.org/specification
[libpq]: https://www.postgresql.org/docs/current/static/libpq.html
[libuv]: https://libuv.org/
[OpenSSL]: https://www.openssl.org/
[Portable endian]: https://github.com/dmitigr/libapp/blob/main/src/dmitigr/3rdparty/portable_endian
[PostgreSQL]: https://www.postgresql.org/
[RapidJSON]: http://rapidjson.org/
[RapidJSON_LICENSE]: https://github.com/dmitigr/cpp-3rd-rapidjson/blob/main/license.txt
[SQLite]: https://www.sqlite.org/
[uSockets]: https://github.com/uNetworking/uSockets
[uSockets_LICENSE]: https://github.com/dmitigr/cpp-3rd-usockets/blob/main/LICENSE
[uWebSockets]: https://github.com/uNetworking/uWebSockets
[uWebSockets_LICENSE]: https://github.com/dmitigr/cpp-3rd-uwebsockets/blob/main/LICENSE
[uwsc]: https://github.com/zhaojh329/libuwsc
[uwsc_LICENSE]: https://github.com/zhaojh329/libuwsc/blob/master/LICENSE
[Visual_Studio]: https://www.visualstudio.com/
[zlib]: https://zlib.net/
