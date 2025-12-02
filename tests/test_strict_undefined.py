import pytest

from nano_template import StrictUndefined
from nano_template import UndefinedVariableError
from nano_template import render


def test_output_strict_undefined() -> None:
    with pytest.raises(UndefinedVariableError):
        render("{{ nosuchthing }}", data={}, undefined=StrictUndefined)


def test_output_nested_strict_undefined() -> None:
    with pytest.raises(UndefinedVariableError):
        render("{{ foo.nosuchthing }}", data={"foo": {}}, undefined=StrictUndefined)


def test_strict_undefined_list_index() -> None:
    with pytest.raises(UndefinedVariableError):
        render("{{ foo[99] }}", data={"foo": [1, 2, 3]}, undefined=StrictUndefined)


def test_strict_undefined_truthiness() -> None:
    result = render(
        "{% if nosuchthing %}true{% else %}false{% endif %}",
        data={},
        undefined=StrictUndefined,
    )
    assert result == "false"


def test_loop_over_strict_undefined() -> None:
    with pytest.raises(UndefinedVariableError):
        render(
            "{% for item in nosuchthing %}..{% endfor %}",
            data={},
            undefined=StrictUndefined,
        )
