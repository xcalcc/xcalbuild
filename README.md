# Xcalibyte Build Monitor

## Design documents

[Design](docs/design/Design.md)

## User documents

[User](docs/user/User.md)

## Build & unit test

### Linux

Use the build scripts inside `scripts/build/lin` for dockerised build with `clang-10`.

Local dev build with `gcc` or `clang` needs [vcpkg](https://github.com/microsoft/vcpkg) for dependencies.
```sh
git clone https://github.com/Microsoft/vcpkg.git && cd vcpkg &&./bootstrap-vcpkg.sh -disableMetrics
vcpkg/vcpkg --disable-metrics install boost nlohmann-json libarchive[core]
```

Then run the following:
```sh
mkdir build
cd build
cmake \
    -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE \
    -DCMAKE_BUILD_TYPE:STRING=Debug \
    -DCMAKE_TOOLCHAIN_FILE:FILEPATH=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake \
    ..
make
```

Debug build generates test coverage instrumentations.

Test coverage is collected with `gcovr`, which could be installed with `sudo apt install gcovr` on Ubuntu.

After a successful local build, run `scripts/test/run-builtin-tests.sh` for unit tests and test coverage reports.
When compiling with `clang-10`, add `--clang` to the command line.

### Windows

Use Visual Studio 2019 to open the root folder as cmake project.

Setup vcpkg similar to the Linux case (but add `--triplet x64-windows-static` to the `vcpkg install` command line to install x64 instead of x86 packages).

Change `CMakeSettings.json` if needed. The default setting produces statically linked binaries.

### OSX

Setup `clang-10` and related tools, vcpkg, and the rest follows similar to the Linux build.


## Project evaluation

Run `scripts/test/ci-run-all-projects.sh`. This requires:
* running xcalscan instance (IP/port defined in `scripts/test/run-xvsa.sh`) for license

## Doc build

For UNIX builds only.

Install dependencies `doxygen` and `graphviz`, then do the normal config/build before running `make docs` to build the API docs.
