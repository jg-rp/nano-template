# My first C extension

For a typical parser workload, my reasoning goes like this:

1. If we care about performance, we want to avoid copying and/or encoding potentially large input strings.

2. If we're processing an opaque syntax tree (or other internal representation) with contextual data in the form of Python objects, we want to avoid data object wrappers or other indirect access to that data.

3. If the result of processing our internal representation is a potentially large string, we want to avoid copying and/or re-encoding the result before exposing it to Python users.

4. If we're exposing a potentially large syntax tree to Python users, we want to avoid wrapping every single node in the tree.

Reaching for Cython, pybind11, PyO3, etc. can, of course, yield impressive results. However, to me, for this use case, using cPython's C API directly feels more natural. We can work with Python's internal string representation directly and we can build complex trees of Python objects in-place.

This approach is not without its downsides. We have to deal with manual memory management and manual Python object reference counting. That is what I've been practicing in my latest project.

## Nano template

Nano Template is a fast, non-evaluating template engine with syntax familiar to anyone who's used Jinja, Minijinja or Django templates.

Unlike those popular template engines, Nano Template forces you to keep application logic out of template text by implementing a reduced feature set. Instead of manipulating template data with additional markup, you are expected to process your data in Python, before rendering a template.

This example demonstrates Nano Template's API and covers all available template syntax:

```python
import nano_template as nt

# TODO:
```

## Benchmarks

A provisional benchmark shows Nano Template to be about 17 times faster than a pure Python solution, and about 4 times faster than Minijinja.

Excluding parsing time and limiting our benchmark fixture to simple variable substitution only, Nano Template renders about 10% slower than `str.format()` (we're using cPython's limited C API, which comes with a performance cost).

TODO: benchmark table

## Conclusions

At the moment, confidence in correct memory management and reference counting it quite high. I've successfully used Python debug builds and Python's debug memory allocator to find bugs.

TODO: I've implemented a simple allocator that also keeps track of AST object references on top of Python's memory manager ()

In the search for a balance between performance and maintenance costs, ...

TODO: I've found limiting myself to Python's stable ABI and limited C API to strike a reasonable balance between performance and maintenance costs.

TODO: In the absence of any existing C, C++, Rust, etc. libraries that need wrapping, using `PyMem_*` functions and a simple arena allocator ...

TODO: If you're wrapping existing C, C++, Rust, etc. libraries ...
