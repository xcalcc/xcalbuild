
# Overall Design

This document describes the overall design of xcalbuild.

## Requirements

The basic requirements are:

* run on hosts
  * Linux (Ubuntu)
  * Windows
* trigger by `xcalagent`
  * command line compatibility with Python `xcalbuild`
* input
  * host build command and config via xcalagent
* output
  * host compiler preprocessing output in specific structure
  * host-compiler-related info to guide `xvsa`
* logging
* error handling
* performance target

## External Work Flow

The default work flow of `xcalbuild`:

1. From web UI, the user
    1. specify build dir
    2. config pre-build command (e.g. `./configure`, or `cmake .`)
    3. config build command (e.g. `make -j4`)
    4. (optionally) select toolchain profile (one of `gnu`, `clang`, `iar`, `keil`)
    5. (optionally) select preprocessing parallelism
    6. (optionally) select tracing method (one of `dynamic`, `static`, `windbg`)
    7. trigger build

2. In `xcalagent`, the tool
    1. receive build instruction with user configs
    2. call `xcalbuild` with
        * build dir
        * pre-build command
        * build command
        * (optional) toolchain profile
        * (optional) preprocessing parallelism
        * (optional) tracing method

3. In `xcalbuild`, the tool
    1. parse command line
    2. load toolchain profile
    3. perform pre-build command
    4. [trace](docs/design/Tracer.md) the build command to generate [build spec](docs/design/Build-Spec.md)
    5. [process](docs/design/Build-Processor.md) build spec
        1. identify native compiler binaries, and [probe](docs/design/Prober.md) them according to tool profiles
        2. (in parallel) [process](docs/design/Work-Item-Processor.md) each work item
            1. parse the build command
            2. run preprocess if needed, or generated link information
        3. aggregate work item processing results to generate output

4. `xcalagent` uploads preprocessing artifacts and triggers scan

## Input/Output

See [here](docs/design/IO.md) for input/output format description.

## Internal Workflow and Architecture

![Workflow](files/xcalbuild-workflow.svg "Workflow")

The components involved:

* [Tracer](docs/design/Tracer.md)
* [Prober](docs/design/Prober.md)
* [Build Processor](docs/design/Build-Processor.md)
* [Work Item Processor](docs/design/Work-Item-Processor.md)
* [Profile Provider](docs/design/Profile-Provider.md)

## Toolchain Profile Specification and Usage

[Toolchain Profile](docs/user/Profile-Spec.md)
