# Toolchain Profile

A profile is a description of a native compiler toolchain to guide `xcalbuild` through the pre-build process.

The `xcalbuild` installation comes with a selection of default toolchain profiles.
But they can be changed to adjust for different native build environment as well.

## Format

A profile is a set of files including:

* a toolchain profile JSON file specifying the tools used by the toolchain and the path to their tool profiles
* several tool profile JSON files specifying the handling of individual tools
* pre-include header files for compiler intrinsic, etc
* several tool profile test JSON files for testing the tool profiles

## Toolchain Profile

This enumerates the build tools to be traced in this build environment, and links to their individual profiles.

Definitions:

```typescript
ToolchainProfile = {
    tools: ToolDef[]
}

ToolDef  = {
    aliases?: string[],
    profile: string
};
```

**Note** this document uses TypeScript-style type specifications in the definitions.

The `profile` property specifies the relative or absolute path to the tool profile.

When supplying the optional `aliases` property, the engine will only match the supplied
binary aliases instead of the default ones defined in the tool profile.

**Note** the behavior is undefined if a single alias appears in multiple tool definitions.

Example:

```json
{
    "tools": [
        {
            "aliases": ["cc1", "cc1plus"],
            "profile": "./gcc.profile.json"
        },
        {
            "profile": "./ld.profile.json"
        }
    ]
}
```

In this example, command lines for `cc1` and `cc1plus` will be handled by `gcc.profile.json`,
while the

## Tool profile

This is the second layer JSON file, e.g. `gcc.profile.json`, that details tool-specific options and handling.

Definition:

```typescript
ToolProfile = {
    aliases: string[],
    cAliases?: string[],
    cxxAliases?: string[],
    defaultCommandKind: CommandKind,
    optionPrefix?: string,
    options: OptionDef[],
    cPrependPreprocessingOptions?: string[],
    cxxPrependPreprocessingOptions?: string[],
    cAppendPreprocessingOptions?: string[],
    cxxAppendPreprocessingOptions?: string[],
    cPrependScanOptions?: string[],
    cxxPrependScanOptions?: string[],
    cSystemIncludePaths?: string[],
    cxxSystemIncludePaths?: string[],
    cPreIncludes?: string[],
    cxxPreIncludes?: string[],
    probeCMacros?: ProbeMacroDef,
    probeCxxMacros?: ProbeMacroDef,
    sourceExtensions?: FileExtDef,
    targetExtensions?: FileExtDef,
    textSubstitutions?: TextSubDef[]
}

CommandKind = 'compile' | 'assemble' | 'archive' | 'link' | 'ignore'
```

### Aliases

The property `aliases` is a list of default command aliases to be traced.
Any command line matching an alias in the list will be handled by this profile.

**Note** the list can be overridden by the `aliases` property in `ToolDef` in toolchain profile.

The optional `cAliases` and `cxxAliases` are used by source language determination.

When specified, any command line matching these will have the default source
file language set to C++.

Example:

```json
{
    "aliases": ["cc1", "cc1plus"],
    "cAliases": ["cc1"],
    "cxxAliases": ["cc1plus"],
}
```

In this example, any source file for `cc1plus`, if not further specified by any option,
is treated as C++ source.

**Note** some aliases in `aliases` would appear in neither `cxxAliases` and `cAliases`.
This is especially the case if the alias is a compiler driver like `gcc`.
For these, the source language is default to be determined by source file extension.

### Default command kind

This specifies the default kind of the command.

**Note** for some binaries, the command kind could be overridden by options.
An example is that `--help` normally turns the command into `ignore`, which will
not be further processed in the work-item-processor.

Example:

```json
{
    "defaultCommandKind": "compile"
}
```

### Option prefix

This specifies the common prefix for the options of this tool.
For unix-based tools, this normally is `-`.

Because the preprocessing workflow is using the native compiler, we are not
translating all the options like in a custom frontend workflow.
It is thus unnecessary to specify all the options in the `options` section.
Only the ones that need special handling, or the ones take space-separated argument,
so as to identify the source/target files on the command line.

The command line items not matched by option definitions but starts with `optionPrefix`
will also be ignored.

Example:

```json
{
    "optionPrefix": "-"
}
```

### Options

This details the option handling part of the profile.

The `options` property is an ordered list of option definitions `OptionDef`.
When handling each command line option, the option definitions are matched
in the order specified in this list. So consider moving frequent options to
the top of the list, and also mind of command line parsing ambiguities.

Definition:

```typescript
OptionDef = CmdOptionDef
          | LangOptionDef
          | RespFileOptionDef
          | DeleteOptionDef
          | ScanOptionDef
          | PreprocessOptionDef
          | OutputOptionDef
          | PreIncludeOptionDef
          | SystemIncludePathOptionDef
          | OtherOptionDef

BaseOptionDef = {
    aliases: string[],
    argFormat: ArgFormatType[],
    type: OptionType
}

ArgFormatType = 'attached' | 'space' | 'equal'

OptionType = 'cmd' | 'language' | 'response' | 'delete' | 'scan' | 'preprocess' | 'output' | 'include' | 'isystem' | 'other'
```

Any option definition will have an ordered list of aliases with prefixes to be matched specified by `aliases`.

The format of how argument is placed w.r.t. the option is specified by `argFormat`.
If `argFormat` is not specified, the option takes no argument.

**Note** many command line options support multiple ways to place arguments. E.g. `-DMACRO=1` or `-D MACRO=1`.

In case of `attached` argument format, the aliases are matched in the order specified.

An option definition also has a `type` property specifying the exact type of the option.

Example:

```json
"options": [
    {"aliases": ["@"], "type": "response", "argFormat" : ["attached"]},
    {"aliases": ["-o", "--output"], "type": "output", "argFormat": ["space", "equlas"]}
]
```

**Note** the `space` specifier also allows for options with optional space-separeted argument, such as `-MD` in `gcc`.
But `optionPrefix` property need to be set for the engine to handle ambiguity on the command line.

#### Command option

This type of option changes the command kind of the parsed work item to any of the `CommandKind`.

An `ignore` kind is to be ignored by work item processor for further processing
because it would not generate any meaningful outcome.

If no command option is on the command line, then `defaultCommandKind` will apply.

Definition:

```typescript
CmdOptionDef extends BaseOptionDef {
    kind: CommandKind
}
```

Example:

```json
"options": [
    {"aliases": ["-c"], "type": "cmd", "kind": "compile"},
    {"aliases": ["--help"], "type": "cmd", "kind": "ignore", "argFormat": ["attached"]},
]
```

#### Language option

Some compilers have command line options to change the language of source files.
This would influence the final decision of whether to produce `.i` or `.ii` file for
the scanner to handle. Other factors in the source language determination process
involves the binary name and the file extension.

Definition:

```typescript
LangOptionDef extends BaseOptionDef {
    argValues: {[arg: string]: FileFormatTypeWithExt}
}
```

The `argValues` property specifies how argument values are mapped to the source languages.
`ext` means this option explicitly specifies the language should be determined by source file extension.

Example:

```json
{
    "aliases": ["-x"],
    "argFormat": ["space"],
    "type": "language",
    "argValues": {
        "c": "c",
        "c++-header": "c",
        "cpp-output": "c",
        "c++": "c++",
        "c++-header": "c++",
        "c++-cpp-output": "c++",
        "none": "ext"
    }
}
```

**Note** the language option will be removed from the preprocessing command line.
Consider adding options to `cPrependPreprocessingOptions` etc to specify the language if needed.

#### Response file option

This specifies the response file to be handled.

It has no additional properties.

Example:

```json
{"aliases": ["@"], "type": "response", "argFormat" : ["attached"]}
```

**Note** the response file option will be unfolded before option process and removed from the preprocessing command line.

#### Delete option

This specifies the options to be deleted from the preprocessing command line.

It has no additional properties.

#### Scan option

This specifies the options to be passed to scanner as scan options.

Definition:

```typescript
ScanOptionDef extends BaseOptionDef {
    cScanOption?: string,
    cxxScanOption?: string,
    scanArgFormat?: ArgFormatType,
    cArgValues?: {[arg: string]: string},
    cxxArgValues?: {[arg: string]: string},
}
```

The `cScanOption` and `cxxScanOption` properties, if present, specifies the scan option including prefix.
The absence of any of these properties means the engine will pass the first alias specified in `aliases` as scan option.

The `scanArgFormat` option, if present, specifies the argument format of the scan option.
The absence of this property means scan option takes no argument.

The `cArgValues` and `cxxArgValues` properties, if present, how argument values are mapped to the scan option arguments.
The absence of any of these properties means the argument will be passed as is to scanner.

If none of the additional properties are specified, the option is passed as is to both C and C++ scan.

Example:

```json
{"aliases": ["-ansi"], "type": "scan"},
{"aliases": ["-std"], "type": "scan", "argFormat": ["equal"], "scanArgFormat": "equal"},
{"aliases": ["-m"], "type": "scan", "argFormat": ["attached"], "scanArgFormat": "attached", "cArgValues": {"32": "32"}}
```

#### Preprocess option

An preprocess option is used by the engine to add output option to the preprocessing
command line. The one used by the engine is the last one specified with tyep `preprocess` in
the `options` list.

It has no additional properties.

**Note** can also specify the preprocess option as an 'ignore' command option on the same time.

#### Output option

This specifies the options to be used to identify output files (arguments).
The output option's argument is used to determine the target of the parsed work item.

An output option is also used by the engine to add output option to the preprocessing
command line. The one used by the engine is the last one specified with tyep `output` in
the `options` list.

It has no additional properties.

For some compilers, such as `iccarm`, the preprocess option takes a target argument.
In this case, the preprocess option could be specified as an output option and leave
preprocess option unspecified.

**Note** the output option will also be removed from the preprocessing command line.

#### Preinclude option

This specifies the options to be used by the engine to add preinclude headers specified
in the profile (`preIncludes`). If there is no option of type `include`, then the profile
property `preIncludes` is ignored.

It has no additional properties.

**Note** the preinclude option has no effect in command line processing, and should be put
near the end of the `options` list.

#### System include path option

This specifies the options to be used by the engine to add system include path specified
in the profile (`systemIncludePaths`). If there is no option of type `isystem`, then the profile
property `systemIncludePaths` is ignored.

It has no additional properties.

**Note** the preinclude option has no effect in command line processing, and should be put
near the end of the `options` list.

#### Other option

This specifies the options not handled by the engine.

It has no additional properties.

The reason that we need this is for identifying source files on the command line.
I.E. the command line items not matched in any of the options.

Generally speaking we should specify all other options with a `space` argument format as `type='other'`,
so that we get the correct leftover on the command line for source files.

#### Unhandled command line items

There will be command line items that are not handled by any of the above option specifications.

In this case, the following process is used to figure out their meaning:

1. if the item starts with `optionPrefix`, it is passed on to preprocessing command line, otherwise
2. if the work item's target is not set and the item ends with any extensions specified in `targetExtensions` property, then set it as the work item's target, otherwise
3. if the item ends with any extensions specified in `sourceExtensions` property, then set it as the work item's source, otherwise
4. log a warning and pass the item on to preprocessing command line

### Additional options

The additional option specifications

```typescript
    cPrependPreprocessingOptions: string[],
    cxxPrependPreprocessingOptions: string[],
    cAppendPreprocessingOptions: string[],
    cxxAppendPreprocessingOptions: string[],
    cPrependScanOptions: string[],
    cxxPrependScanOptions: string[],
```

allows putting additional options to the preprocess and scan command line.

Example:

```json
"cxxPrependScanOptions": [ "-clang" ]
```

### Additional includes

The additional includes specifications

```typescript
    cSystemIncludePaths: string[],
    cxxSystemIncludePaths: string[],
    cPreIncludes: string[],
    cxxPreIncludes: string[]
```

allows adding additional preinclude headers and system include paths to the preprocessing command line per source language.

Example:

```json
"cSystemIncludePaths": [ "./include", "../include/common" ],
"cPreIncludes": [ "../include/common/xcalibyte.h" ]
```

### Preprocessing and scan command line ordering

The final preprocessing command line order for each of the source file is defined below, using C as an example.

```typescript
[binary] ++
cPrependPreprocessingOptions ++
[system include path options] ++
[preinclude header options] ++
[filtered native command line options] ++
cAppendPreprocessingOptions ++
[output options] ++
[source file]
```

The final scan options passed to xvsa is defined below, using C as an example.

```typescript
cPrependScanOptions ++ [Mapped native command line options]
```

### File extension specifications

Additional file extension specifications could be added to help the engine identify source and target files on the command line.

Definition:

```typescript
sourceExtensions?: FileExtDef,
targetExtensions?: FileExtDef,

FileExtDef = {[k: FileFormatType]: string[]}

FileFormatType = 'c' | 'c++' | 'preprocessed' | 'assembly' | 'object' | 'library' | 'executive'
FileFormatTypeWithExt = FileFormatType | 'ext'
```

This is a map from file kind to the kown extensions of this kind.

When a source file extension is matched, and the file kind of the parsed work item is `ext`,
then the file kind will be updated to `c` or `c++` according to the map.

Example:

```json
"sourceExtensions": {
    "preprocessed": [".i", ".ii"],
    "c": [".c"],
    "c++": [".cc", ".cp", ".cxx", ".cpp", ".CPP", ".c++", ".C"],
    "assembly": [".s"]
}
```

`targetExtensions` is needed in cases when the command don't have an output option, like `ar`.

### Prober directives

Definitions:

```typescript
ProbeMacroDef = { [macro: string], ProbeOutcomeDef }

ProbeOutcomeDef = { [expanded: string], ProbeActionDef[] }

ProbeActionDef = {
    config: ,
    action: 'prepend',
    value: string[]
}

ActionableConfigs = 'cPrependPreprocessingOptions'
                  | 'cxxPrependPreprocessingOptions'
                  | 'cAppendPreprocessingOptions'
                  | 'cxxAppendPreprocessingOptions'
                  | 'cPrependScanOptions'
                  | 'cxxPrependScanOptions'
                  | 'cPreIncludes'
                  | 'cxxPreIncludes'
                  | 'cSystemIncludePaths'
                  | 'cxxSystemIncludePaths'
```

Each `ProbeMacroDef` specifies the macro definitions to be probed by the [prober](Build-Tool-Prober).

The key to this definition is the macro name, and the value is a `ProbeOutcomeDef` object specifying
the expected macro expansion text and the corresponding actions.

An action is a triplet of the target config property, the action, and the value for the action.

Example:

```json
"probeCMacros":{
    "__STDC_VERSION__": {
        "__STDC_VERSION__": [{"config": "cPrependScanOptions", "action": "prepend", "value": ["-std=gnu89"]}],
        "199409L": [{"config": "cPrependScanOptions", "action": "prepend", "value": ["-std=gnu89"]}],
        "199901L": [{"config": "cPrependScanOptions", "action": "prepend", "value": ["-std=gnu99"]}],
        "201112L": [{"config": "cPrependScanOptions", "action": "prepend", "value": ["-std=gnu11"]}],
        "201710L": [{"config": "cPrependScanOptions", "action": "prepend", "value": ["-std=gnu11"]}]
    }
},
```

This example specifies the scan options needed for each default language standard for the compiler.

### Text substitutions

This section specifies the post-preprocessing text substitution in the source code.

This should only be sepcified to a `compile` tool.

Definition:

```typescript
textSubstitutions?: TextSubDef[]

TextSubDef = RegexSubDef | StringSubDef

RegexSubDef = {
    regex: string,
    replacement: string
}

StringSubDef = {
    string: string,
    replacement: string
}
```

Basically there are string or regex replacement to be performed.

Example:

```json
"textSubstitution": [
    {
        "regex": "_Float(32|64|128)x?",
        "replacement": "float",
    },
    {
        "string": "__gnu_printf__",
        "replacement": "printf"
    }
]
```

## Profile testing

The installation comes with a utility binary `profile-tester` which can be used to test
a tool profile.

The basic command line is:

```bash
profile-tester -- -p /path/to/tool-profile.json -t /path/to/tool-profile.test.json
```

**Note** the `--` is needed in the command line, which can also take Boost.Test options.

Three different kinds of tests could be performed:

* command line parsing tests
* preprocessing option tests
* source transformation tests
* profile validation

The test file is in JSON format and has the following definition:

```typescript
profileTestDef = {
    commandlineParsingTests: CommandlineParsingTestDef[],
    outputOptionTests: OutputOptionTestDef[],
    sourceTransformationTests: SourceTransformationTestDef[]
}
```

### Command line parsing tests

These are for testing command parsing in general, including

Definition:

```typescript
CommandlineParsingTestDef = {{
    name: string,
    command: CompileCommand,
    expected: ParsedWorkItemSpec
}

CompileCommand = {
    directory: string,
    arguments: string[]
}

ParsedWorkItemSpec = {
    kind?: CommandKind,
    binary?: string,
    fileFormat?: FileFormatTypeWithExt,
    sources?: SourceFileWithFormat[],
    target?: string,
    dir?: string,
    ppOptions?: string[],
    cScanOptions?: string[],
    cxxScanOptions?: string[]
}

SourceFileWithFormat = {
    file: string,
    format: FileFormatType
}
```

A single test consist of:

* a `name`,
* a compile command (result of tracing), and
* a specification of expected parsing result

All properties for the parsing result specification are optional,
so that one can only specify the interested options to test.

The specification represents the end state of command line paring,
with the effect of the options describe in above sections.

Example:

```json
{
    "name": "language-option",
    "command": {
        "directory": "/work",
        "arguments": ["g++","test1.c","-x","c","test2.c","-x","c++","test3.c", "-x","none","test4.c","test5.cc"]
    },
    "expected": {
        "sources": [
            {"file": "/work/test1.c", "format": "c++"},
            {"file": "/work/test2.c", "format": "c"},
            {"file": "/work/test3.c", "format": "c++"},
            {"file": "/work/test4.c", "format": "c"},
            {"file": "/work/test5.cc", "format": "c++"}
        ],
        "ppOptions": []
    }
}
```

This tests whether the profile for `g++` can correctly identify the source language of each input file on the command line.

**Note** The `ppOptions` does not represent the final preprocessing command line, which is per-source-file and include other
specified options as mentioned in above sections.

### Preprocessing option tests

These are for testing whether the profile can correctly generate preprocessing options to
be used on the preprocessing command line.

Definition:

```typescript
PreprocessingOptionTestDef = {{
    name: string,
    command: CompileCommand,
    target: string,
    format: FileFormatType,
    expected: string[]
}
```

The inputs are the native command line, the target to be produced, and the source file format.

**Note** the expected list of options does not include native binary and source file.

Example:

```json
{
    "name": "basic",
    "command": {
        "directory": "/work",
        "arguments": ["/usr/bin/cc1","-D_GNU_SOURCE","-o","CMakeFiles/json-c-static.dir/arraylist.s", "-ansi", "-c","/work/arraylist.c"]
    },
    "target": "temp",
    "format": "c",
    "expected": [
        "-include","../../include/__xvsa_common.h",
        "-include","../../include/__xvsa_ia32.h",
        "-D_GNU_SOURCE","-ansi", "-c",
        "-E",
        "-o","temp"
    ]
}
```

### Source transformation tests

These are for testing whether the profile can correctly transform a piece of source code.

Definition:

```typescript
SourceTransformationTestDef = {
    name: string,
    source: string,
    expected: string
}

```

Example:

```json
{
    "name": "asm-constraint",
    "source": "__asm__ __volatile__(\"=@ccnz\");",
    "expected": "__asm__ __volatile__(\"=q\");"
}
```

### Profile validation

Using the `-c` option, one can validate a profile.

```bash
profile-tester -- -p /path/to/tool-profile.json -c
```

If the profile is invalid, error messages will be provided to assist debugging.
