from __future__ import annotations

import json
import timeit
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from nano_template import parse
from nano_template import render
from nano_template._pure import Template as PyTemplate
from nano_template._pure import render as py_render

# NOTE: At the time of writing, minijinja's `render_str` does not seem to do
# any caching, which is what we want for this benchmark.
# from minijinja import render_str

# from jinja2 import Template as JinjaTemplate
# from jinja2 import Environment as JinjaEnvironment


@dataclass
class Fixture:
    """Benchmark fixture template and data."""

    name: str
    source: str
    data: dict[str, Any]

    @staticmethod
    def load(path: Path) -> Fixture:
        source = (path / "template.txt").read_text()

        with (path / "data.json").open() as fd:
            data = json.load(fd)

        return Fixture(name=path.parts[-1], data=data, source=source)


_TESTS = {
    "parse c ext": "parse(source)",
    "parse pure py": "PyTemplate(source)",
    "just render c ext": "t.render(data)",
    "just render pure py": "nt.render(data)",
    # "just render jinja2": "jinja_template.render(**data)",
    "parse and render ext": "render(source, data)",
    "parse and render pure py": "py_render(source, data)",
    # "parse and render jinja2": "jinja_env.from_string(source).render(**data)",
    # "parse and render minijinja": "render_str(source, name=None, **data)",
}


def benchmark(path: str, number: int = 10000, repeat: int = 5) -> None:
    """Run the benchmark against fixture `path`. Print results to stdout."""
    fixture = Fixture.load(Path(path))
    t = parse(fixture.source)
    nt = PyTemplate(fixture.source)

    _globals: dict[str, Any] = {
        "data": fixture.data,
        "py_render": py_render,
        "PyTemplate": PyTemplate,
        "nt": nt,
        "parse": parse,
        "render": render,
        "source": fixture.source,
        "t": t,
        # "render_str": render_str,
        # "JinjaTemplate": JinjaTemplate,
        # "jinja_env": JinjaEnvironment(cache_size=0, bytecode_cache=None),
        # "jinja_template": JinjaTemplate(fixture.source),
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
