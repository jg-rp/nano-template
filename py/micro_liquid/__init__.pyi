from collections.abc import Mapping
from typing import Any

from ._micro_liquid import Template
from ._micro_liquid import TokenView
from ._micro_liquid import parse
from ._micro_liquid import tokenize
from ._token_kind import TokenKind
from ._undefined import Undefined
from ._exceptions import TemplateError
from ._exceptions import TemplateSyntaxError

__all__ = (
    "Template",
    "TemplateError",
    "TemplateSyntaxError",
    "TokenKind",
    "TokenView",
    "Undefined",
    "parse",
    "render",
    "serialize",
    "tokenize",
)

def serialize(obj: object) -> str: ...
def render(source: str, data: Mapping[str, Any]) -> str: ...
