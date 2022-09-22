# Prober

Prober runs specific tests with the native toolchain to gain additional information
not available from the [build spec](docs/design/Build-Spec.md).

Currently the prober is used to test native compiler's expansion of macros and
adjust the tool profile accordingly.

## Invocation

Prober is called by [build processor](docs/design/Build-Processor.md) and interacts with [profile provider](docs/design/Profile-Provider.md).

## Workflow

Prober takes the prober directives from the tool profile and,

1. generates pseudo C/C++ code snippets
2. preprocess the code snippets with the native compiler
3. parse the preprocess output and trigger prober directive actions according to macro expansion values

Example:

For the prober directive:

```json
"probeCMacros":{
    "__STDC_VERSION__": {
        "__STDC_VERSION__": [{"config": "cPrependScanOptions", "action": "prepend", "value": ["-std=gnu89"]}],
        "199409L": [{"config": "cPrependScanOptions", "action": "prepend", "value": ["-std=gnu89"]}]
    }
},
```

The code snippet generated is

```C
__XCAL____STDC_VERSION__ __STDC_VERSION__
```

The preprocess result could be

```C
__XCAL____STDC_VERSION__ 199409L
```

And prober asks the tool profile provider to apply the following action to set
the tool's default language standard to `gnu89`.

```json
{"config": "cPrependScanOptions", "action": "prepend", "value": ["-std=gnu89"]}
```
