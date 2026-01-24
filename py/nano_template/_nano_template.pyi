from collections.abc import Mapping
from typing import Callable
from typing import Type
from typing import TypeAlias
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

class CompiledTemplate:
    def render(self, data: Mapping[str, object]) -> str: ...

def parse(
    source: str,
    serializer: Callable[[object], str],
    undefined: Type[Undefined],
) -> Template: ...
def compile(
    source: str,
    serializer: Callable[[object], str],
    undefined: Type[Undefined],
) -> CompiledTemplate: ...

class BytecodeView:
    """Bytecode instructions and constant pool."""

    @property
    def instructions(self) -> bytes: ...
    @property
    def constants(self) -> list[object]: ...

def bytecode(source: str) -> BytecodeView: ...

OpDef: TypeAlias = tuple[str, tuple[int, int], int, int]

def bytecode_definitions() -> list[OpDef]: ...
