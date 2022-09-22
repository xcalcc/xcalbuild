# Test System

The test system consists of:

* Unit tests
* Feature tests
* Profile tests
* Evaluation projects

The first three are counted towards test coverage target.
They are included using cmake's test support and can be triggered with `ctest`.
There is also `scripts/test/run-builtin-tests.sh` to trigger coverage run.

The repo contains other test scripts in `scripts/test` that drives the
evaluation projects in a CI setting.

## Unit Tests

These are tests implemented with Boost.Test and often locate by the side of source file,
e.g. `prober.test.cpp` for `prober.cpp`.

Generally these should be testing intra-component functions.

## Feature Tests

These are script-driven tests running the installed binary.
They generally test features involving multiple components or non-trivial I/Os.

They locate at `tests/features`.

## Evaluation Projects

These are OSS projects of different build system and toolchain to run end-to-end (preprocessing and scan)
tests over the xcal tools.

### Linux

| project | build system | toolchain | profile | tracing |
|---|---|---|---|---|
| bazel-examples | bazel | gcc | gnu-cc | static |
| cmake | cmake/make | gcc/g++ | gnu | dynamic |
| gdb | make | gcc/g++ | gnu | static |
| json-c | cmake/make | gcc | gnu | dynamic |
| linkit-sdk | sh/make | arm-none-eabi-gcc | gnu | dynamic |
| openssl | make | gcc | gnu | dynamic |
| rt-thread | scons | arm-none-eabi-gcc | gnu | dynamic |

### Windows

| project | build system | toolchain | profile | tracing |
|---|---|---|---|---|
| cmis | iarbuild | iccarm | iar | windbg |
| samsung-pio | iarbuild | iccarm | iar | windbg |
| rt-thread | UV4 | armcc | keil | windbg |
