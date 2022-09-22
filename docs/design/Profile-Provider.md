# Profile Provider

Profile provider is the in-memory instantiation of the selected [toolchain profiles](md_docs_user_Profile-Spec.md).

A `ToolchianProfile` class instance represents the entire toolchain, while a `ToolProfile` class
instance represents a single tool.

It provides toolchain information to guide the entire `xcalbuild` process:

## Workflow

The workflow is divided into the following phase.

### Initial profile loading

In this phase, the following happen in order:

1. the [toolchain profile](md_docs_user_Profile-Spec.md) JSON is loaded in memory
2. according to the toolchain profile, the individual [tool profiles](md_docs_user_Profile-Spec.md) are also loaded in memory
3. the immutable (non-actionable) parts of the tool profiles are used to initiate internal data structures in the provider

### Tracing

In this phase, the toolchain provider provides a list of interesting binary names
and optionally response file option format to the tracer to identify native commands to be handled.

### Probing

In this phase, the following happen in order:

1. JSON probe directives are provided to the [prober](docs/design/Prober.md)
2. [prober](docs/design/Prober.md) runs probing process and identify actions to be applied
3. tool profile provider applies the actions on the JSON object to update [`ActionableConfigs`](md_docs_user_Profile-Spec.md#Prober-directives)

### Actionable config loading

This phase completes profile loading by initiating internal data structures
with the updated [`ActionableConfigs`](md_docs_user_Profile-Spec.md#Prober-directives)

After this phase, the profile provider is read-only and allows for parallel access by [work item processors](docs/design/Work-Item-Processor.md).

### Work item processing

[work item processors](docs/design/Work-Item-Processor.md) calls profile provider methods to

1. parse a work item in the build spec into `ParsedWorkItem`
2. for compile commands, generate the preprocessing command line

## Internal

The profile provider holds the JSON representation of the profile specs,
while also maintain internal data structures for better serving the profile information.

For each tool, it instantiate a list of `Option` class instances from the
[option specification](md_docs_user_Profile-Spec.md#Options) and use them to parse the native command line to determine:

* the command kind (compile, assemble, archive, or link)
* the native command line options applicable to preprocessing command line
* the scanner command line options mapped from the native command line options
* the output
* the source files and their format
* ...
