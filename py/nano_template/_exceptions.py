# SPDX-License-Identifier: MIT

import os


class TemplateError(Exception):
    """Base class for all template exceptions."""

    def __init__(self, *args: object, source: str, start_index: int, stop_index: int):
        super().__init__(*args)
        self.source: str = source
        self.start_index: int = start_index
        self.stop_index: int = stop_index

    def __str__(self) -> str:
        if self.start_index < 0 or self.stop_index < 0:
            return super().__str__()

        value = self.source[self.start_index : self.stop_index]
        index = self.start_index
        context = self.error_context(self.source, index)

        if not context:
            return super().__str__()

        line, col, current = context
        position = f"{current!r}:{line}:{col}"
        pad = " " * len(str(line))
        pointer = (" " * col) + ("^" * (len(value) or 1))

        return os.linesep.join(
            [
                self.args[0],
                f"{pad} -> {position}",
                f"{pad} |",
                f"{line} | {current}",
                f"{pad} | {pointer} {self.args[0]}",
            ]
        )

    def error_context(self, text: str, index: int) -> tuple[int, int, str] | None:
        """Return the line number, column number and current line of text."""
        lines = text.splitlines(keepends=True)
        cumulative_length = 0
        target_line_index = -1

        for i, line in enumerate(lines):
            cumulative_length += len(line)
            if index < cumulative_length:
                target_line_index = i
                break

        if target_line_index == -1:
            return None

        line_number = target_line_index + 1  # 1-based
        column_number = index - (cumulative_length - len(lines[target_line_index]))
        current_line = lines[target_line_index].rstrip()
        return (line_number, column_number, current_line)


class TemplateSyntaxError(TemplateError):
    """An exception raised during template parsing due to unexpected template syntax."""


class UndefinedVariableError(TemplateError):
    """An exception raised by the strict undefined type."""
