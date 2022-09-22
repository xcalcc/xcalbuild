# Input/Output

This describes the input and output of `xcalbuild`.

The I/O is designed to be backward compatible with [Python Xcalbuild Functionality](docs/design/Python-Xcalbuild-Functionality.html).

## Input

`xcalbuild` takes input from both command line and the `config` file in the installation directory.

### Command line format

```bash
Usage:
xcalbuild [options] -- build commands
Allowed options:
  -i [ --builddir ] arg                 build directory
  -o [ --outputdir ] arg                output directory
  -p [ --prebuild ] arg                 prebuild command, such as 'cmake
                                        .','./configure'...
  --process_link_using_compiler         process source code file whose target file is linked through use of compiler command, default is false.
                                        source code file whose target file is linked through use of linker command such as ld will be processed direclty by xcalbuild.
                                        (new option, Python Xcalbuild Functionality does not have this)
  --debug                               debug info
  --preprocessor arg (=default)         preprocessor command
  -t [ --tracing_method ] arg (=dynamic)
                                        the method for build tracing, one of
                                        'dynamic', 'static' or 'windbg'
  -j [ --parallel ] arg (=1)            the pre-processing parallelism, default
                                        is 1
  --profile arg (=gnu)                  the toolchain profile to be used,
                                        default is 'gnu'
  -v [ --version ]                      display version
  -h [ --help ]                         display help message
```

### Config file

`xcalbuild` also reads the `config` file in the installation directory, for

1. output file/dir names (`CDB_NAME`, `PREPROCESS`, and `SOURCE_FILES`)
2. `xcalibyte.properties` property keys (`dependencies`, `c_scan_options`, and `cxx_scan_options`)

Example:

```ini
VERSION=0.1.3
CDB_NAME=compile_commands.json
PREPROCESS=preprocess
SOURCE_FILES=source_files.json
CLANG_OPTIONS=
[PROPERTY_KEY]
version=
project=
vcs_tool=
vcs_url=
vcs_token=
vcs_branch=
vcs_version=
src_root_dir=
build_root_dir=
configure_command=
build_command=
c_compiler=
c_extra_flags=
cxx_compiler=
cxx_extra_flags=
assembler=
as_extra_flags=
archiver=
ar_extra_flags=
linker=
ld_extra_flags=
compile_only=
dependencies=
c_scan_options=
cxx_scan_options=
```

## Output

* a `preprocess.tar.gz`
* a `source_files.json`

The details are desribed in [`xcalbuild` interface spec](files/xcalbuild-interface-spec.pdf)
