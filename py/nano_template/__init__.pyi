from collections.abc import Mapping
from typing import Any
from typing import Callable
from typing import Type

from ._nano_template import Template
from ._nano_template import TokenView as _TokenView
from ._nano_template import tokenize as _tokenize
from ._token_kind import TokenKind as _TokenKind
from ._undefined import Undefined
from ._undefined import StrictUndefined
from ._exceptions import TemplateError
from ._exceptions import TemplateSyntaxError
from ._exceptions import UndefinedVariableError

__all__ = (
    "_tokenize",
    "_TokenKind",
    "_TokenView",
    "parse",
    "render",
    "serialize",
    "StrictUndefined",
    "Template",
    "TemplateError",
    "TemplateSyntaxError",
    "Undefined",
    "UndefinedVariableError",
)

def serialize(obj: object) -> str: ...
def parse(
    source: str,
    *,
    serializer: Callable[[object], str] = serialize,
    undefined: Type[Undefined] = Undefined,
) -> Template: ...
def render(
    source: str,
    data: Mapping[str, Any],
    *,
    serializer: Callable[[object], str] = serialize,
    undefined: Type[Undefined] = Undefined,
) -> str: ...
