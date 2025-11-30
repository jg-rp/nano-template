from collections.abc import Mapping
from typing import Any
from typing import Callable
from typing import Type

from ._nano_template import Template
from ._nano_template import TokenView
from ._nano_template import tokenize
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

def serialize(obj: object) -> str: ...
def parse(source: str) -> Template: ...
def render(
    source: str,
    data: Mapping[str, Any],
    *,
    serializer: Callable[[object], str] = serialize,
    undefined: Type[Undefined] = Undefined,
) -> str: ...
