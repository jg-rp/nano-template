```
| Symbol type             | Naming                                 |
| ----------------------- | -------------------------------------- |
| Internal struct         | `ML_Span`, `ML_Buffer`                 |
| Internal function       | `ml_span_init()`, `ml_buffer_append()` |
| Python wrapper struct   | `MLPyBufferObject`                     |
| Python wrapper function | `mlpy_buffer_new()`                    |
| Macros / constants      | `ML_MAX_SIZE`                          |
```

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
