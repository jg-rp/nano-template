# SPDX-License-Identifier: MIT

import json
from collections.abc import Mapping
from typing import Any
from typing import Callable
from typing import Type

from ._nano_template import CompiledTemplate
from ._nano_template import Template
from ._nano_template import TokenView as _TokenView
from ._nano_template import BytecodeView as _BytecodeView
from ._nano_template import compile as _compile
from ._nano_template import parse as _parse
from ._nano_template import tokenize as _tokenize
from ._nano_template import bytecode as _bytecode
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
    "CompiledTemplate",
    "Template",
    "TemplateError",
    "TemplateSyntaxError",
    "Undefined",
    "UndefinedVariableError",
)


def serialize(obj: object) -> str:
    return json.dumps(obj) if isinstance(obj, (list, dict, tuple)) else str(obj)


def parse(
    source: str,
    *,
    serializer: Callable[[object], str] = serialize,
    undefined: Type[Undefined] = Undefined,
) -> Template:
    """Parse `source` as a template."""
    try:
        return _parse(source, serializer, undefined)
    except RuntimeError as err:
        raise TemplateSyntaxError(
            str(err),
            source=source,
            start_index=getattr(err, "start_index", -1),
            stop_index=getattr(err, "stop_index", -1),
        ) from None


def compile(
    source: str,
    *,
    serializer: Callable[[object], str] = serialize,
    undefined: Type[Undefined] = Undefined,
) -> CompiledTemplate:
    """Parse and compile `source` as a template."""
    try:
        return _compile(source, serializer, undefined)
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
    return parse(source, serializer=serializer, undefined=undefined).render(data)


def compile_and_render(
    source: str,
    data: Mapping[str, Any],
    *,
    serializer: Callable[[object], str] = serialize,
    undefined: Type[Undefined] = Undefined,
) -> str:
    """Render template `source` with variables from `data`."""
    return compile(source, serializer=serializer, undefined=undefined).render(data)
