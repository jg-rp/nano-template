from typing import Any

import pytest

from nano_template import render
from nano_template import TemplateSyntaxError


def test_else() -> None:
    source = "{% if a %}a{% elif b %}b{% else %}c{% endif %}"
    data = {"a": False, "b": False}
    assert render(source, data) == "c"


def test_else_block() -> None:
    source = "{% if a %}a{% elif b %}b{% else %}{{ c }}{% endif %}"
    data: dict[str, Any] = {"a": False, "b": False, "c": "d"}
    assert render(source, data) == "d"


def test_elif() -> None:
    source = "{% if a %}a{% elif b %}b{% else %}c{% endif %}"
    data = {"a": False, "b": True}
    assert render(source, data) == "b"


def test_nested_if() -> None:
    source = "{% if a %}a{% if b %}b{% else %}c{% endif %}{% else %}d{% endif %}"
    data = {"a": True, "b": False}
    assert render(source, data) == "ac"


# TODO: better error messages


def test_elsif() -> None:
    source = "{% if a %}a{% elsif b %}b{% else %}c{% endif %}"
    data = {"a": False, "b": True}
    with pytest.raises(TemplateSyntaxError, match="unknown tag"):
        render(source, data)


def test_too_many_else_tags() -> None:
    source = "{% if a %}a{% else %}b{% else %}c{% endif %}"
    data = {"a": False, "b": False}
    with pytest.raises(
        TemplateSyntaxError, match="expected TOK_ENDIF_TAG, found TOK_ELSE_TAG"
    ):
        render(source, data)
