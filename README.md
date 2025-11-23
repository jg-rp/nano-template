```
PySys_WriteStdout("ch: %c\n", ch);
```

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
(001) Best of 5 rounds with 1000 iterations per round.
parse ext                : best = 0.202319s | avg = 0.205246s
parse native             : best = 3.994172s | avg = 4.034880s
parse and render ext     : best = 0.336588s | avg = 0.344831s
parse and render native  : best = 4.666261s | avg = 4.715309s
just render ext          : best = 0.127127s | avg = 0.131248s
just render native       : best = 0.591786s | avg = 0.601283s
```
