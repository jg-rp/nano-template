import json
from nano_template import parse

import gc

# gc.set_debug(gc.DEBUG_LEAK)
# gc.set_debug(gc.DEBUG_SAVEALL)

with open("tests/fixtures/001/data.json") as fd:
    data = json.load(fd)

with open("tests/fixtures/001/template.txt") as fd:
    source = fd.read()

gc.disable()
gc.collect()
before = len(gc.get_objects())

parse(source).render(data)

gc.collect()
after = len(gc.get_objects())
print("Object delta:", after - before)
gc.enable()

# leaked = [o for o in gc.garbage if type(o).__module__ == "_nano_template"]
# for o in leaked:
#     print(o, type(o))
