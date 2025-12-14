# Nano Templates

Minimal, fast, non-evaluating text templates for Python.

![PyPI - License](https://img.shields.io/pypi/l/nano-template?style=flat-square)
![PyPI - Python Version](https://img.shields.io/pypi/pyversions/nano-template?style=flat-square)
![PyPI - Version](https://img.shields.io/pypi/v/nano-template?style=flat-square)

## Install

Use [pip](https://docs.python.org/3/installing/index.html), or your favorite package manager:

```console
python -m pip install nano-template
```

## Example

```python
import nano_template as nt

template = nt.parse("Hello, {{ you }}!")
print(template.render({"you": "World"}))  # Hello, World!
```

## About

Nano Template is a small text templating engine, written as a Python C extension, with syntax familiar to anyone who's used Jinja/Minijinja, Django templates or Liquid.

Unlike those popular template engines, Nano Template forces you to keep application logic out of template text by implementing a reduced feature set. In this scenario, template authors and application developer are likely to be the same person (or team of people).

## Syntax

> [!NOTE]
> In Nano templates, there are no filters or tests, no relational or membership operators, and we don't have for loop helpers or `{% break %}` and `{% continue %}`.
>
> Instead, you should process your data in Python before rendering a template, or use Minijinja.
>
> Available tags are `{% if %}`, `{% elif %}`, `{% else %}` and `{% for %}`.

### Variables

```liquid
<div>{{ some.variable }}</div>
<p>{{ other["variable with spaces or special characters"] }}</p>
```

### Conditions

```liquid
{% if some.variable %}
  more markup
{% elif another.variable %}
  alternative markup
{% else %}
  default markup
{% endif %}
```

### Loops

```liquid
{% for x in y %}
  more markup with {{ x }}
{% else %}
  default markup (y was empty or not iterable)
{% endfor %}
```

### Logical operators

Logical operators `and`, `or` and `not` use Python truthiness and precedence rules, and terms can be grouped with parentheses.

```liquid
{% if not a and b %}
  markup with {{ b }} and {{ c }}.
{% endif %}
```

Logical `and` and `or` have _last value_ semantics.

```liquid
Hello, {{ user.name or "guest" }}!
```

### Strings

String literals and quoted variable path segments can use single or double quotes, and allow JSON-style escape sequences.

```liquid
{{ greeting or "Hi \uD83D\uDE00!" }}
```

**Output:**

```
Hi 😀!
```

### Whitespace control

Control whitespace before and after markup delimiters with `-` and `~`. `~` will remove newlines but retain space and tab characters. `-` strips all whitespace.

```liquid
<ul>
{% for x in y ~%}
  <li>{{ x }}</li>
{% endfor -%}
</ul>
```

## API

### render

`render(source, data)` is a convenience function that parses and immediately renders a template to a string. Use this for testing or when you know you'll be rendering the template just the once.

```python
import nano_template as nt

print(nt.render("Hello, {{ you }}!", {"you", "World"}))
```

`render` also accepts [`serializer`](#serializing-objects) and [`undefined`](#undefined-variables) keyword arguments.

### parse

`parse(source)` parses template text and returns an instance of `Template` for later rendering with the `render(self, data)` method.

```python
import nano_template as nt

template = nt.parse("Hello, {{ you }}!")
print(template.render({"you": "World"}))  # Hello, World!
print(template.render({"you": "Sue"}))  # Hello, Sue!
```

`parse` also accepts [`serializer`](#serializing-objects) and [`undefined`](#undefined-variables) keyword arguments.

### Serializing objects

By default, when outputting an object with `{{` and `}}`, lists, dictionaries and tuples are rendered in JSON format. For all other objects we render the result of `str(obj)`.

You can change this behavior by passing a callable to [`parse`](#parse) or [`render`](#render) as the `serializer` keyword argument. The callable should accept an object and return its string representation suitable for output.

This example shows how one might define a serializer that can dump data classes with `json.dumps`.

```python
import json
from dataclasses import asdict
from dataclasses import dataclass
from dataclasses import is_dataclass

import nano_template as nt

@dataclass
class SomeData:
    foo: str
    bar: int

def json_default(obj: object) -> object:
    if is_dataclass(obj) and not isinstance(obj, type):
        return asdict(obj)
    raise TypeError(f"Object of type {obj.__class__.__name__} is not JSON serializable")

def my_serializer(obj: object) -> str:
    return (
        json.dumps(obj, default=json_default)
        if isinstance(obj, (list, dict, tuple))
        else str(obj)
    )


template = nt.parse("{{ some_object }}", serializer=my_serializer)
data = {"some_object": [SomeData("hello", 42)]}

print(template.render(data))  # [{"foo": "hello", "bar": 42}]
```

### Undefined variables

When a template variable or property can't be resolved, an instance of the _undefined type_ is used instead. That is, an instance of `nano_template.Undefined` or a subclass of it.

The default _undefined type_ renders nothing when output, evaluates to `False` when tested for truthiness and is an empty iterable when looped over. You can pass an alternative _undefined type_ as the `undefined` keyword argument to [`parse`](#parse) or [`render`](#render) to change this behavior.

Here we use the built-in `StrictUndefined`.

```python
import nano_template as nt

t = nt.parse("{{ foo.nosuchthing }}", undefined=nt.StrictUndefined)

print(t.render({"foo": {}}))
# nano_template._exceptions.UndefinedVariableError: 'foo.nosuchthing' is undefined
#   -> '{{ foo.nosuchthing }}':1:3
#   |
# 1 | {{ foo.nosuchthing }}
#   |    ^^^ 'foo.nosuchthing' is undefined
```

Or you can implement you own.

```python
from typing import Iterator
import nano_template as nt


class MyUndefined(nt.Undefined):
    def __str__(self) -> str:
        return "<MISSING>"

    def __bool__(self) -> bool:
        return False

    def __iter__(self) -> Iterator[object]:
        yield from ()


t = nt.parse("{{ foo.nosuchthing }}", undefined=MyUndefined)

print(t.render({"foo": {}}))  # <MISSING>
```

## Preliminary benchmark

TODO: move this

(On an M2 Mac Mini with Python 3.13)

```
$ python scripts/benchmark.py
(001) 5 rounds with 10000 iterations per round.
parse c ext                   : best = 0.092587s | avg = 0.092743s
parse pure py                 : best = 2.378554s | avg = 2.385293s
just render c ext             : best = 0.061812s | avg = 0.061850s
just render pure py           : best = 0.314468s | avg = 0.315076s
just render jinja2            : best = 0.170373s | avg = 0.170706s
just render minijinja         : best = 0.454723s | avg = 0.457256s
parse and render ext          : best = 0.155797s | avg = 0.156455s
parse and render pure py      : best = 2.733121s | avg = 2.745028s
parse and render jinja2       : <with caching disabled, I got bored waiting>
parse and render minijinja    : best = 0.705995s | avg = 0.707589s
```

```
$ python scripts/benchmark_format.py
(002) 5 rounds with 1000000 iterations per round.
render template               : best = 0.413830s | avg = 0.419547s
format string                 : best = 0.375050s | avg = 0.375237s
```

## Contributing

TODO

## Notes to self

TODO: move this

### Python Debug Build

Build Python in debug mode from source in `~/python-debug`:

```
./configure --prefix=$HOME/python-debug --with-pydebug
```

Tell uv to use the debug build instead of system or downloaded Python version:

```
uv sync -p <path/to/python debug binary>
```

Rebuild the extension in debug mode:

```
uv run python setup.py build_ext --inplace --force --debug
```

### PYTHONMALLOC=debug

Use `PYTHONMALLOC=debug python dev.py` to activate Python's debug memory allocator, which inserts guard bytes, fills memory with known patterns, and performs validation to catch buffer overflows, use-after-free, and double frees when using Python memory APIs.

ASAN does not seem to catch these.

### Manual reference leak check

(With a debug Python build.)  
(A debug build will also report negative reference counts.)

If the delta is small and consistent, it's probably not a ref count leak.

```python
import sys
from nano_template import render

before = sys.gettotalrefcount()

for i in range(10000):
    render("{% if a %}a{% else %}c{% endif %}", {"a": False, "b": False})
    if i % 1000 == 0:
        print(f"Iteration {i}, total refcount={sys.gettotalrefcount()}")

after = sys.gettotalrefcount()
print("Refcount delta:", after - before)
```

### Detecting leaks from reference cycles

(We shouldn't be creating any reference cycles in this project.)

```python
import json
from nano_template import parse

import gc

# gc.set_debug(gc.DEBUG_LEAK)
# gc.set_debug(gc.DEBUG_SAVEALL)

with open("tests/fixtures/001/data.json") as fd:
    data = json.load(fd)

with open("tests/fixtures/001/template.txt") as fd:
    source = fd.read()

gc.disable()
gc.collect()
before = len(gc.get_objects())

parse(source).render(data)

gc.collect()
after = len(gc.get_objects())
print("Object delta:", after - before)
gc.enable()

# leaked = [o for o in gc.garbage if type(o).__module__ == "_nano_template"]
# for o in leaked:
#     print(o, type(o))
```

### ABI 3 Audit

Note that abi3audit ignores target ABI version when auditing .so files.

- Build a wheel locally with `python setup.py bdist_wheel`
- Run `abi3audit dist/<NAME>.whl --verbose`

Example successful output:

```
[17:55:59] 💁 nano_template-0.1.0-cp39-abi3-linux_x86_64.whl: 1 extensions scanned; 0 ABI version mismatches and 0 ABI violations found
```

### Release

1. Manually run the [Build wheels workflow](https://github.com/jg-rp/nano-template/actions/workflows/build.yaml).
1. Clean `./dist/`.
1. Download workflow artifacts.
1. Extract artifacts to `./dist`.
1. Build sdist with `uv run python -m build --sdist`
1. Upload `uv run python -m twine upload dist/*`

## License

`nano-template` is distributed under the terms of the [MIT](https://spdx.org/licenses/MIT.html) license.
