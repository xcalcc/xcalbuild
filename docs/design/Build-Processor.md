# Build Processor

This module handles turns the [build spec](docs/design/Build-Spec.md) into [the output of `xcalbuild`](docs/design/IO.md#Output).

## Workflow

The build processor is triggered after [tracer](docs/design/Tracer.md) and works in the following steps:

1. loads the build spec
2. probe all the native tools mentioned in the build spec
3. (in parallel) invoke [work item processor](docs/design/Work-Item-Processor.md) to process the build spec work items
4. aggregate the results and generate output

## Internal

The build processor contains mutex-protected lists to hold work item results of different kinds.

The output generation is done in the following steps:

### Assemble results processing

All assemble results are processed to create a map `as_target_to_source` from assemble
target to assemble source.

### First round link results processing

All link/archive results are processed to create the output directory structure and the following maps:

* `link_targets` from link source file to a `LDInfo` instance with link information containing
  * preprocessing output path
  * dependencies
  * C/C++ scan options
* `dependency_names` from target full path to the file name in the dependencies list

**Note** the keys in `link_targets` are compile outputs, and `as_target_to_source` is used to
find the compile outputs in case assembler calls are traced.
E.g. for `cc1 -o t.s -c t.c`, `as -o t.o t.s`, `ld -o a.out t.o`,
`as_target_to_source` will have `t.o -> t.s`, and `link_targets` will have `t.s -> a.out`.

This process handles the case when different link target path shares the same file name
by renaming the later target with a number suffix.
E.g. `lib.a.dir` and `lib.a.1.dir`.

### Compile result processing

All compile results are processed to fill in the output directory with preprocess files.

This process uses `link_targets` to locate the preprocessing output path.

It also:

1. appends the checksum file for the preprocessing output.
2. sets C/C++ scan options in the corresponding `link_targets` entry, if not set already
3. removes the compile target from dependencies in the corresponding `link_targets` entry
4. appends the source file list

**Note** the scan option is set to the ones from the first compile result per language.

### Second round link results processing

All link results are visited again to finalize the dependencies (by replacing full path
with suffixed names) and generate `xcalibyte.properties`.
Checksum file is also updated.

### Final output

Achiever is called to generate `preprocess.tar.gz` and the content of source file list
is output as `source_files.json`.
