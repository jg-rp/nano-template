import json
from collections.abc import Mapping
from typing import Any
from typing import Callable
from typing import Type

from ._micro_liquid import Template
from ._micro_liquid import TokenView
from ._micro_liquid import parse as _parse
from ._micro_liquid import tokenize
from ._token_kind import TokenKind
from ._undefined import Undefined
from ._undefined import StrictUndefined
from ._exceptions import TemplateError
from ._exceptions import TemplateSyntaxError
from ._exceptions import UndefinedVariableError

__all__ = (
    "StrictUndefined",
    "Template",
    "TemplateError",
    "TemplateSyntaxError",
    "TokenKind",
    "TokenView",
    "Undefined",
    "UndefinedVariableError",
    "parse",
    "render",
    "serialize",
    "tokenize",
)


def serialize(obj: object) -> str:
    return json.dumps(obj) if isinstance(obj, (list, dict, tuple)) else str(obj)


def parse(source: str) -> Template:
    """Parse `source` as a template."""
    try:
        return _parse(source)
    except RuntimeError as err:
        raise TemplateSyntaxError(
            str(err),
            source=source,
            start_index=getattr(err, "start_index", -1),
            stop_index=getattr(err, "stop_index", -1),
        ) from None


def render(
    source: str,
    data: Mapping[str, Any],
    *,
    serializer: Callable[[object], str] = serialize,
    undefined: Type[Undefined] = Undefined,
) -> str:
    """Render template `source` with variables from `data`."""
    try:
        return _parse(source).render(data, serializer, undefined)
    except RuntimeError as err:
        raise TemplateSyntaxError(
            str(err),
            source=source,
            start_index=getattr(err, "start_index", -1),
            stop_index=getattr(err, "stop_index", -1),
        ) from None
