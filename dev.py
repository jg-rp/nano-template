from typing import Iterator
import nano_template as nt


class MyUndefined(nt.Undefined):
    def __str__(self) -> str:
        return "<MISSING>"

    def __bool__(self) -> bool:
        return False

    def __iter__(self) -> Iterator[object]:
        yield from ()


t = nt.parse("{{ foo.nosuchthing }}", undefined=MyUndefined)

print(t.render({"foo": {}}))  # <MISSING>
