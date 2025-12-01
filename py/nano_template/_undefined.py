# SPDX-License-Identifier: MIT

from __future__ import annotations

import re
from typing import TYPE_CHECKING
from typing import Iterable

from ._exceptions import UndefinedVariableError

if TYPE_CHECKING:
    from ._nano_template import TokenView

_RE_WORD = re.compile(r"[\u0080-\uFFFFa-zA-Z_][\u0080-\uFFFFa-zA-Z0-9_-]*")


class Undefined:
    """The object used when a template variable can not be resolved."""

    def __init__(self, source: str, path: list[int | str], token: TokenView):
        self.source = source
        self.path = path
        self.token = token

    def __str__(self) -> str:
        return ""

    def __bool__(self) -> bool:
        return False

    def __iter__(self) -> Iterable[object]:
        yield from ()


class StrictUndefined(Undefined):
    def __str__(self) -> str:
        raise UndefinedVariableError(
            f"{_path_to_str(self.path)!r} is undefined",
            source=self.source,
            start_index=self.token.start,
            stop_index=self.token.end,
        )

    def __bool__(self) -> bool:
        return False

    def __iter__(self) -> Iterable[object]:
        raise UndefinedVariableError(
            f"{_path_to_str(self.path)!r} is undefined",
            source=self.source,
            start_index=self.token.start,
            stop_index=self.token.end,
        )


def _path_to_str(path: list[str | int]) -> str:
    it = iter(path)
    buf: list[str] = [str(next(it, ""))]
    for segment in it:
        if isinstance(segment, str):
            if _RE_WORD.fullmatch(segment):
                buf.append(f".{segment}")
            else:
                buf.append(f"[{segment!r}]")
        else:
            buf.append(f"[{segment}]")
    return "".join(buf)
