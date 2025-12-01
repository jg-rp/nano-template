import json
from nano_template import render


with open("tests/fixtures/002/template.txt") as fd:
    source = fd.read()

with open("tests/fixtures/002/template_f.txt") as fd:
    source_f = fd.read()

with open("tests/fixtures/002/data.json") as fd:
    data = json.load(fd)

print(render(source, data))

print(source_f.format(**data))


# print(render(source, data))

# TODO: setup ruff
# TODO: replace uv with hatch?
# TODO: accept `serializer` and `undefined` when calling `parse` and store in `Template` instance
# TODO:   - make Template.render() arguments optional
# TODO:   - benchmark _Template subclass?

# TODO: compare valgrind output before and after arena

# ==15356== LEAK SUMMARY:
# ==15356==    definitely lost: 0 bytes in 0 blocks
# ==15356==    indirectly lost: 0 bytes in 0 blocks
# ==15356==      possibly lost: 395,096 bytes in 4 blocks
# ==15356==    still reachable: 2,979 bytes in 7 blocks
# ==15356==         suppressed: 0 bytes in 0 blocks
# ==15356==
# ==15356== ERROR SUMMARY: 4 errors from 4 contexts (suppressed: 0 from 0)
