from collections.abc import Mapping
from typing import Callable
from typing import Type
from ._undefined import Undefined

class TokenView:
    """Lightweight token view into source text (read-only)."""

    # Read-only properties
    @property
    def start(self) -> int: ...
    @property
    def end(self) -> int: ...
    @property
    def text(self) -> str: ...
    @property
    def kind(self) -> int: ...

def tokenize(source: str) -> list[TokenView]: ...

class Template:
    def render(self, data: Mapping[str, object]) -> str: ...

def parse(
    source: str,
    serializer: Callable[[object], str],
    undefined: Type[Undefined],
) -> Template: ...
