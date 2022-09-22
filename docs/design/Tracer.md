# Tracer

The tracers watches native build process and generates [build spec](docs/design/Build-Spec.md).

The individual tracers are separate binaries that are platform-dependent.
A `Tracer` component inside `xcalbuild` is responsible to interact with these tracers.

## Unix Dynamic Tracer

This is based on a previous version of (BEAR)(https://github.com/rizsotto/Bear) with MIT license.

The improvements include:

* allow specifying a comma-separated list of interesting binary names
* output `arguments` list instead of space-separated `command`
* reads (transient) response file content while tracing, and output to build spec
* 32/64bit `LD_PRELOAD` payload to allow tracing mixed build environment (e.g. 64bit `make` and 32bit `arm-none-eabi-gcc`)

The dynamtic tracer only works for the build system that

1. allows specifying `LD_PRELOAD`, and
2. use dynamically linked library to invoke system calls

The tracer is implemented in a separate binary `unix-tracer` which as the following command line:
```bash
Usage: bin/unix-tracer [-o output] [-l libpaths] [-s socket] [-d] [-c clist] -- command

   -o output   output file (default: compile_commands.json)
   -l libpaths library paths to search for libtracer-preload.so (default: install/lib32:install/lib64)
   -s socket   multiplexing socket (default: randomly generated)
   -d          debug output (default: disabled)
   -c clist    override the known compilers with a comma-separated list (default: ar,cc,ld,gcc,llvm-gcc,clang,c++,g++,llvm-g++,clang++,arm-none-eabi-gcc,arm-none-eabi-g++)
   -p          print out known compilers
   -h          this message
```

## Unix Static Tracer

This is based on [strace](https://strace.io/) which in turn is based on the kernel feature [ptrace](https://man7.org/linux/man-pages/man2/ptrace.2.html).

The `Tracer` component calls `strace` and parsed the output into build spec.

The static tracer works in case of statically linked build tool, but still has the following limitations:

1. needs the build tool to invoke subcommands with system call
2. can only handle persistent response files

## Windows

This is based on the Windows Debug API. It uses processes from the [Process Threads API](https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/) header to run and inspect the process being traced in debug mode.

This allows the tracer to detect when a new process is created (include subprocesses) and inspect their details using the functions in the [winternal header](https://docs.microsoft.com/en-us/windows/win32/api/winternl/). In particular, the [NtQueryInformationProcess](https://docs.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntqueryinformationprocess) function returns the [process environment block](https://docs.microsoft.com/en-us/windows/win32/api/winternl/ns-winternl-peb).

The offsets described [here](https://www.geoffchappell.com/studies/windows/win32/ntdll/structs/peb/index.htm) are used to look up the process environment block, to get the directory and command line args. Response file expansion is done during the trace.

The tracer is implemented in a separate binary `win-tracer` which as the following command line:
```bash
Usage: bin/win-tracer [/o output] [/c config] command

   /o output   output file
   /c config   config file
```

The config file specifies all interesting binary names (in lower cases, but compares in case insensitive way) to be included in the compile database output.
It is of the format:

```typescript
ConfigFile = ToolConfig[];

ToolConfig = {
    binary: string,
    responseFileArgs: RespFileConfig[]
}

RespFileConfig = {
    argument: string,
    argFormat: ('attached' | 'space' | 'equal')[]
}
```

**Note** when the config file is an empty list, the tracer dumps all process invocations. This is for debug purpose only.

Example:

```json
[
  {
    "binary": "armclang",
    "responseFileArgs": [
      {
        "argument": "@",
        "argFormat": [ "attached" ]
      }
    ]
  },
  {
    "binary": "armlink",
    "responseFileArgs": [
      {
        "argument": "--Via",
        "argFormat": [ "equal", "space" ]
      }
    ]
  }
]
```