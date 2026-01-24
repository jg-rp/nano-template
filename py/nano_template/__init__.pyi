from collections.abc import Mapping
from typing import Any
from typing import Callable
from typing import Type

from ._nano_template import CompiledTemplate
from ._nano_template import Template
from ._nano_template import BytecodeView as _BytecodeView
from ._nano_template import TokenView as _TokenView
from ._nano_template import bytecode as _bytecode
from ._nano_template import tokenize as _tokenize
from ._nano_template import bytecode_definitions as _bytecode_definitions
from ._token_kind import TokenKind as _TokenKind
from ._undefined import Undefined
from ._undefined import StrictUndefined
from ._exceptions import TemplateError
from ._exceptions import TemplateSyntaxError
from ._exceptions import UndefinedVariableError

__all__ = (
    "_bytecode",
    "_bytecode_definitions",
    "_tokenize",
    "_TokenKind",
    "_TokenView",
    "compile",
    "compile_and_render",
    "parse",
    "render",
    "serialize",
    "_BytecodeView",
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
def compile(
    source: str,
    *,
    serializer: Callable[[object], str] = serialize,
    undefined: Type[Undefined] = Undefined,
) -> CompiledTemplate: ...
def render(
    source: str,
    data: Mapping[str, Any],
    *,
    serializer: Callable[[object], str] = serialize,
    undefined: Type[Undefined] = Undefined,
) -> str: ...
def compile_and_render(
    source: str,
    data: Mapping[str, Any],
    *,
    serializer: Callable[[object], str] = serialize,
    undefined: Type[Undefined] = Undefined,
) -> str: ...
