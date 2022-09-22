# Infrastructure

## Programming Language

C++14 (`-std=c++14`)

## Build System

Use `cmake` to drive the overall build.

### Platform-dependent

The build tracer would be platform dependent to use the tracing features.

#### Linux

`cmake` + `clang/llvm`

#### Windows

`msbuild` + `ms vc++`

### Platform-independent

The rest of the system would be platform-independent and use .

Visual Studio 2019 now supports developing `cmake/clang` projects.

## 3rd Party Libraries

3rd party libraries are managed by [vcpkg](https://github.com/microsoft/vcpkg).

The libraries used are:

### Boost

The main dependency are [Boost libraries](https://www.boost.org/). The libraries used:

* process
* filesystem
* log
* algorithm (string)
* regex
* optional
* variant
* property_tree (for ini file)
* uuid (sha1)
* assert
* test (for unit tests)

### Parallel job system

To run the work item processor in parallel.

https://github.com/taskflow/taskflow

### JSON I/O

For toolchain profiles, build specs, binary list input to tracers, and output files.

https://github.com/nlohmann/json

### Archive generator

For producing `preprocess.tar.gz`.

https://www.libarchive.org/
