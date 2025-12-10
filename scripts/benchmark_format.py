import json
import timeit
from typing import Any

from nano_template import parse


with open("tests/fixtures/002/template.txt") as fd:
    source = fd.read()

with open("tests/fixtures/002/template_f.txt") as fd:
    source_f = fd.read()

with open("tests/fixtures/002/data.json") as fd:
    data = json.load(fd)

template = parse(source)

assert template.render(data) == source_f.format(**data)


_TESTS = {
    "render template": "template.render(data)",
    "format string": "source_f.format(**data)",
}


def benchmark(number: int = 1000000, repeat: int = 5) -> None:
    _globals: dict[str, Any] = {
        "source_f": source_f,
        "data": data,
        "template": template,
    }

    print(f"(002) {repeat} rounds with {number} iterations per round.")

    for name, stmt in _TESTS.items():
        times = timeit.repeat(stmt, globals=_globals, repeat=repeat, number=number)
        print(
            f"{name:<30}: best = {min(times):.6f}s | avg = {sum(times) / len(times):.6f}s"
        )


if __name__ == "__main__":
    benchmark()
