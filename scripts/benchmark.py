from __future__ import annotations

import json
import timeit
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from micro_liquid import parse
from micro_liquid import render
from micro_liquid import Undefined
from micro_liquid import serialize
from micro_liquid._native import Template as NativeTemplate
from micro_liquid._native import render as native_render


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
    "just render ext": "t.render(data, serialize, Undefined)",
    "just render native": "nt.render(data)",
}


def benchmark(path: str, number: int = 1000, repeat: int = 5) -> None:
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
    }

    print(
        f"({fixture.name}) Best of {repeat} rounds with {number} iterations per round."
    )

    for name, stmt in _TESTS.items():
        times = timeit.repeat(stmt, globals=_globals, repeat=repeat, number=number)
        print(
            f"{name:<25}: best = {min(times):.6f}s | avg = {sum(times) / len(times):.6f}s"
        )


if __name__ == "__main__":
    # TODO: CLI to choose fixture
    benchmark("tests/fixtures/001")
