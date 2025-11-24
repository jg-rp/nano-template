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
parse ext                     : best = 0.012726s | avg = 0.012807s
parse native                  : best = 0.243873s | avg = 0.244109s
parse and render ext          : best = 0.019537s | avg = 0.019592s
parse and render native       : best = 0.278124s | avg = 0.278674s
parse and render minijinja    : best = 0.070754s | avg = 0.071700s
just render ext               : best = 0.006169s | avg = 0.006182s
just render native            : best = 0.030539s | avg = 0.030618s
```
