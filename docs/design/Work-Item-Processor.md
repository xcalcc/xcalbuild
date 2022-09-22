# Work Item Processor

This module handles a single work item in the build spec.

It is designed to be able to run multiple instances in parallel.

## Workflow

An `WorkItemProcessor` instance is instantiated by the [build processor](docs/design/Build-Processor.md)
for each work item in the build spec.

The work item is firstly parsed by the corresponding [tool profile provider](docs/design/Profile-Provider.md),
then handled separately according to the command kind.

For compile commands, the work item processor constructs **separate** command line
for each sources of the command basing on the source file format(C or C++),
and run the preprocessing commands.
The preprocessing outputs are stored in temp files and passed back to build processor.

The non-system dependencies are also extracted from the preprocessing output to be added to `source_files.json`.

In case of multiple source files on a command line, a pseudo link option is also added
to link up all the sources.

For other commands, the sources and target information is passed back to build processor.
