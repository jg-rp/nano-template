import operator
from typing import NamedTuple

import pytest

from nano_template import TemplateSyntaxError
from nano_template import parse
from nano_template import render


class Case(NamedTuple):
    description: str
    template: str
    data: dict[str, object]
    want: str


TEST_CASES: list[Case] = [
    Case(
        description="escaped u0020",
        template="{{ a['\\u0020'] }}",
        data={"a": {" ": "hi"}},
        want="hi",
    ),
    Case(
        description="escaped code point",
        template="{{ a['\\u263A'] }}",
        data={"a": {"☺": "hi"}},
        want="hi",
    ),
    Case(
        description="escaped surrogate pair",
        template="{{ a['\\uD834\\uDD1E'] }}",
        data={"a": {"𝄞": "hi"}},
        want="hi",
    ),
    Case(
        description="escaped double quote",
        template='{{ a["\\""] }}',
        data={"a": {'"': "hi"}},
        want="hi",
    ),
    Case(
        description="escaped single quote",
        template="{{ a['\\''] }}",
        data={"a": {"'": "hi"}},
        want="hi",
    ),
    Case(
        description="escaped reverse solidus",
        template="{{ a['\\\\'] }}",
        data={"a": {"\\": "hi"}},
        want="hi",
    ),
]


@pytest.mark.parametrize("case", TEST_CASES, ids=operator.attrgetter("description"))
def test_unescape_strings(case: Case) -> None:
    assert render(case.template, case.data) == case.want


def test_escaped_double_quote_in_single_quote_string() -> None:
    with pytest.raises(TemplateSyntaxError, match=r"invalid '\\\"' escape sequence"):
        parse("{{ a['\\\"'] }}")


def test_invalid_code_point() -> None:
    with pytest.raises(
        TemplateSyntaxError, match="invalid hex digit `X` in escape sequence"
    ):
        parse("{{ a['ab\\u263Xc'] }}")
