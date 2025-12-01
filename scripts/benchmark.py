from __future__ import annotations

import json
import timeit
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from nano_template import parse
from nano_template import render
from nano_template import Undefined
from nano_template import serialize
from nano_template._pure import Template as NativeTemplate
from nano_template._pure import render as native_render

# XXX: I'm assuming `render_str` does not cache parsed templates.
# TODO: Force parse by generating lots of distinct strings.
# from minijinja import render_str


@dataclass
class Fixture:
    """Benchmark fixture template and data."""

    name: str
    source: str
    data: dict[str, Any]

    @staticmethod
    def load(path: Path) -> Fixture:
        source = (path / "index.template").read_text()

        with (path / "data.json").open() as fd:
            data = json.load(fd)

        return Fixture(name=path.parts[-1], data=data, source=source)


_TESTS = {
    "parse ext": "parse(source)",
    "parse native": "NativeTemplate(source)",
    "parse and render ext": "render(source, data)",
    "parse and render native": "native_render(source, data)",
    # "parse and render minijinja": "Environment().render_str(source, name=None, **data)",
    "just render ext": "t.render(data, serialize, Undefined)",
    "just render native": "nt.render(data)",
}


def benchmark(path: str, number: int = 10000, repeat: int = 5) -> None:
    """Run the benchmark against fixture `path`. Print results to stdout."""
    fixture = Fixture.load(Path(path))
    t = parse(fixture.source)
    nt = NativeTemplate(fixture.source)

    _globals: dict[str, Any] = {
        "data": fixture.data,
        "native_render": native_render,
        "NativeTemplate": NativeTemplate,
        "nt": nt,
        "parse": parse,
        "render": render,
        "source": fixture.source,
        "t": t,
        "Undefined": Undefined,
        "serialize": serialize,
        # "render_str": render_str,
    }

    print(
        f"({fixture.name}) Best of {repeat} rounds with {number} iterations per round."
    )

    for name, stmt in _TESTS.items():
        times = timeit.repeat(stmt, globals=_globals, repeat=repeat, number=number)
        print(
            f"{name:<30}: best = {min(times):.6f}s | avg = {sum(times) / len(times):.6f}s"
        )


if __name__ == "__main__":
    # TODO: CLI to choose fixture
    benchmark("tests/fixtures/001")
