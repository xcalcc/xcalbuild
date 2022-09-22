# Python xcalbuild Functionalities

## Command Line Arguments
* --builddir (-i)
	* The path to the directory where the build command should be executed
* --outputdir (-o)
	* The directory to put the xcalbuild artefacts
	* Given the way the checksum works, I think xcal assumes this is an empty directory, but nothing enforces this
* --prebuild (-p)
	* A string for a command to run before the actual build command is executed
	* If the prebuild command is `cmake` it will run `make clean && rm -rf CMakeFiles && rm -f CMakeCache*`
* --debug
	* Doesn't do anything
* --preprocesser
	* The name of the preprocessor to use
	* Used to determine if additional options need to be passed to the compiler when preprocessing
	* Valid options are default, xvsa or clang
		* Any other value will fall through an if/elif
* `<build>`
	* The build command to run
	* Supported build commands seem to be aos, catkin, scons, make and ninja. These commands will trigger a clean of the project
* --version (-v)
	* Print the version and exit

## Config File
* In addition to command line args, `xcalbuild` also uses a side file, `config` that lives next to the xcalbuild python script.
* The version for linux has the following keys:
	* VERSION - current version
	* CDB_NAME - name of bear's output file
	* PREPROCESS - name of directory to put preprocessed files in
	* SOURCE_FILES - name of file to output list of source files
	* CLANG_OPTIONS - options to pass when running clang
* It also includes fields to use in the `xcalibyte.properties` files generated for each link target (see below).

```
[linux]
VERSION=0.1.3
CDB_NAME=compile_commands.json
PREPROCESS=preprocess
SOURCE_FILES=source_files.json
CLANG_OPTIONS=-D__clang__ -D__clang_major__=7 -D__clang_minor__=0 -D__clang_patchlevel__=1 -D__has_feature\(modules\)=0 -D'__building_module(x)=0' -D'__has_extension(x)=0' -D'__builtin_is_constant_evaluated(x)=0' -D'__builtin_prefetch(x,y,z)=0' -DMB_LEN_MAX=16 -DPATH_MAX=4096 -U__OPTIMIZE__ -O0
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

## High Level Overview of Current Process
1. Use the build command to figure out how to clean the project
2. Clean the project
	* Runs a cleaning command based on the build command supplied
	* May also run additional cleaning if given a prebuild command
3. Run any pre-build command supplied by the `--prebuild` option
4. Run the build command wrapped with `bear`.
	* The version Xcalibyte supplied assumes bear is installed in `PATH`
	* I've made a modification to allow for it to use our repo's version for now
	* Output of the build command is captured to a log file and not shown on console
5. Process bear output to find link targets and compile commands
6. Use the bear output to find and preprocesses source files, makes an archive of them (.tar.gz) as well as a JSON list of files

## Xcalbuild Output
* The final output of xcalbuild is an archive of preprocessed source files.
	* This is generated in `generate_i()`
*  Preprocessed source files are organised into directories like so
	`preprocess/<binary>.dir/preprocess/<file>.i`
* Each binary directory also has an `xcalibyte.properties` file
	* This file in the format of `<key>=<value>`
	* Generated in `Properties.get_properties()`
	* Has information about how to build that binary (compiler options, link options)
* A log is created in the output directory (`xcalbuild.log`)
* A checksum file for the output directory is put into `checksum.sha1`
* `source_files.json` - list of source files

Example director structure of `preprocess`:
```
preprocess
├── checksum.sha1
├── json_parse.dir
│   ├── preprocess
│   │   └── json_parse.c.i
│   └── xcalibyte.properties
├── libjson-c.a.dir
│   ├── preprocess
│   │   ├── arraylist.c.i
│   │   ├── debug.c.i
│   │   ├── json_c_version.c.i
...
│   │   ├── random_seed.c.i
│   │   └── strerror_override.c.i
│   └── xcalibyte.properties
...
├── test_visit.dir
│   ├── preprocess
│   │   └── test_visit.c.i
│   └── xcalibyte.properties
└── xcalibyte.properties
```

`preprocess/checksum.sha1`

SHA1 for all text files inside the directory
```
da39a3ee5e6b4b0d3255bfef95601890afd80709 /work/scripts/test/projects/json-c/preprocess/xcalibyte.properties
b9c187418cd96414cc20061e80b6fd54961900c2 /work/scripts/test/projects/json-c/preprocess/test_int_add.dir/xcalibyte.properties
6926087a3fd5e64a881a826b7f985766966b4e66 /work/scripts/test/projects/json-c/preprocess/test_int_add.dir/preprocess/test_int_add.c.i
dabdd5e44bd00bdb455baa19f4661f4515a76eaa /work/scripts/test/projects/json-c/preprocess/test2Formatted.dir/xcalibyte.properties
bf8f31dc0ff1bc759081831041d1f8a0b9a1cb8a /work/scripts/test/projects/json-c/preprocess/test2Formatted.dir/preprocess/test2.c.i
```

`source_files.json`

An array of absolute paths of input files.

```
["/work/scripts/test/projects/json-c/repo/tests/test_printbuf.c", "/work/scripts/test/projects/json-c/repo/tests/test_null.c", ...]
```

## Appendix: `xcalibyte.properties` keys
The keys for `xcalibyte.properites` are defined in the config file described above.

Descriptions below are for the fields that are used in the script. The fields that  are defined but not used are left blank here. Fields can be empty in the `xcalibyte.properties` file.

* version
* project
* vcs_tool
* vcs_url
* vcs_token
* vcs_branch
* vcs_version
* src_root_dir
* build_root_dir
* configure_command
* build_command
	* supposed to be the prebuild command, but the function to set it is never called
* c_compiler
	* the compiler used (if language is C)
* c_extra_flags
	* additional flags used to the c compiler (if language is C)
* cxx_compiler
	* the compiler used (if language is C++)
* cxx_extra_flags
	*  additional flags used to the c compiler (if language is C++)
* assembler
* as_compiler (not set in config file, but referenced in python script)
	* the assembler used
* as_extra_flags
	* extra flags passed to the assembler
* archiver
	* the archive (ar) used
* ar_extra_flags
	* extra args passed to the archiver
* linker
	* the linker used
* ld_extra_flags
	* extra flags passed to the linker
* compile_only
* dependencies
	* dependencies on other linked items in the projects
	* found by going through link command and identifying other binaries being linked
* c_scan_options
	* options from the c compiler that have to be passed to the scanner (?) these are: C standard (`-std`) and architecture (`-m32`)
* cxx_scan_options
	* options from the c++ compiler that have to be passed to the scanner (?) these are: C standard (`-std`) and architecture (`-m32`)

For now there is only one `xcalibyte.properties` per link target, and most fields are empty, e.g.:

```
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
c_compiler=/usr/bin/cc
c_extra_flags=-g
cxx_compiler=
cxx_extra_flags=
assembler=
as_extra_flags=
archiver=
ar_extra_flags=
linker=/usr/bin/ld
ld_extra_flags=-plugin /usr/lib/gcc/x86_64-linux-gnu/7/liblto_plugin.so -plugin-opt=/usr/lib/gcc/x86_64-linux-gnu/7/lto-wrapper -plugin-opt=-fresolution=/tmp/ccIyjLoV.res -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s -plugin-opt=-pass-through=-lc -plugin-opt=-pass-through=-lgcc -plugin-opt=-pass-through=-lgcc_s --build-id --eh-frame-hdr -m elf_x86_64 --hash-style=gnu --as-needed -export-dynamic -dynamic-linker /lib64/ld-linux-x86-64.so.2 -pie -z now -z relro -o test_parse /usr/lib/gcc/x86_64-linux-gnu/7/../../../x86_64-linux-gnu/Scrt1.o /usr/lib/gcc/x86_64-linux-gnu/7/../../../x86_64-linux-gnu/crti.o /usr/lib/gcc/x86_64-linux-gnu/7/crtbeginS.o -L/usr/lib/gcc/x86_64-linux-gnu/7 -L/usr/lib/gcc/x86_64-linux-gnu/7/../../../x86_64-linux-gnu -L/usr/lib/gcc/x86_64-linux-gnu/7/../../../../lib -L/lib/x86_64-linux-gnu -L/lib/../lib -L/usr/lib/x86_64-linux-gnu -L/usr/lib/../lib -L/usr/lib/gcc/x86_64-linux-gnu/7/../../.. CMakeFiles/test_parse.dir/test_parse.c.o -rpath /work/scripts/test/projects/json-c/repo ../libjson-c.so.5.0.0 -lgcc --push-state --as-needed -lgcc_s --pop-state -lc -lgcc --push-state --as-needed -lgcc_s --pop-state /usr/lib/gcc/x86_64-linux-gnu/7/crtendS.o /usr/lib/gcc/x86_64-linux-gnu/7/../../../x86_64-linux-gnu/crtn.o
compile_only=
dependencies=libjson-c.so.5.0.0
c_scan_options=
cxx_scan_options=
```

The top-level `xcalbuild.properties` seems to be always empty.
