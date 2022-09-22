# Build Specification

The build specification contains a list of interesting native tool invocations.

It generally follows the [JSON compilation database](https://clang.llvm.org/docs/JSONCompilationDatabase.html) format,
but only `directory` and `arguments` are the required properties in a work item representing a native tool invocation.

Definition:
```typescript

BuildSpecDef = WorkItemDef[]

WorkItemDef = {
    directory: string,
    arguments: string[]
    respfile?: string
}
```

The `directory` is the working directory of the invocation,
while `arguments` represents the full command line, including the binary path
as `argumnets[0]`.

The optional `respfile` specifies the content of a response file seen on command line, if any.
A tracer also just expands the command line with response file content directly.

**Note** that although some tracer produces the `file` property,
it is ignored by `xcalbuild`.