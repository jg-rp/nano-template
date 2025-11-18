from __future__ import annotations

from typing import TYPE_CHECKING
from typing import Iterable

if TYPE_CHECKING:
    from ._micro_liquid import TokenView


class Undefined:
    """The object used when a template variable can not be resolved."""

    def __init__(self, name: str, token: TokenView):
        self.name = name
        self.token = token

    def __str__(self) -> str:
        return ""

    def __bool__(self) -> bool:
        return False

    def __iter__(self) -> Iterable[object]:
        yield from ()
