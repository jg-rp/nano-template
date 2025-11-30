import json
from dataclasses import asdict
from dataclasses import dataclass
from dataclasses import is_dataclass

import pytest

from nano_template import render


@dataclass
class MockData:
    foo: str
    bar: int


def json_default(obj: object) -> object:
    if is_dataclass(obj) and not isinstance(obj, type):
        return asdict(obj)
    raise TypeError(f"Object of type {obj.__class__.__name__} is not JSON serializable")


def my_serializer(obj: object) -> str:
    return (
        json.dumps(obj, default=json_default)
        if isinstance(obj, (list, dict, tuple))
        else str(obj)
    )


def test_custom_serializer() -> None:
    source = "{{ a }}"
    data = {"a": [MockData("hello", 42)]}

    # Without custom serializer
    with pytest.raises(
        TypeError, match="Object of type MockData is not JSON serializable"
    ):
        render(source, data)

    # With custom serializer
    assert (
        render(source, data, serializer=my_serializer)
        == '[{"foo": "hello", "bar": 42}]'
    )
