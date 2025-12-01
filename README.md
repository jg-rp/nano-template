## lldb

```
command source .lldbinit
```

## ASan

```
# Linux
export LD_PRELOAD=$(gcc -print-file-name=libasan.so)
export ASAN_OPTIONS=detect_leaks=1:abort_on_error=1

# macOS
export DYLD_INSERT_LIBRARIES=$(clang -print-file-name=libclang_rt.asan_osx_dynamic.dylib)
export ASAN_OPTIONS=detect_leaks=1:abort_on_error=1
```

## Preliminary benchmark

```
$ python scripts/benchmark.py
(001) Best of 5 rounds with 10000 iterations per round.
parse ext                     : best = 0.092188s | avg = 0.092236s
parse pure                    : best = 2.408759s | avg = 2.416534s
parse and render ext          : best = 0.159726s | avg = 0.159882s
parse and render pure         : best = 2.816334s | avg = 2.822223s
just render ext               : best = 0.062731s | avg = 0.062923s
just render pure              : best = 0.308758s | avg = 0.309301s
```

```
(002) Best of 5 rounds with 1000000 iterations per round.
render template               : best = 0.413830s | avg = 0.419547s
format string                 : best = 0.375050s | avg = 0.375237s
```

## ABI 3 Audit

Note that abi3audit ignores target ABI version when auditing .so files.

- Build a wheel locally with `python setup.py bdist_wheel`
- Run `abi3audit dist/<NAME>.whl --verbose`

Example successful output:

```
[17:55:59] 💁 nano_template-0.1.0-cp39-abi3-linux_x86_64.whl: 1 extensions scanned; 0 ABI version mismatches and 0 ABI violations found
```
