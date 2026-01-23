// SPDX-License-Identifier: MIT

#include "nano_template/vm.h"

typedef int (*OpFn)(NT_VM *vm, size_t *ip, PyObject *data);

static int vm_noop(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_text(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_render(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_not(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_constant(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_global(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_selector(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_pop(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_jump(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_jump_if_falsy(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_jump_if_truthy(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_set_local(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_get_local(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_iter_init(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_iter_next(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_enter_frame(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_leave_frame(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_true(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_false(NT_VM *vm, size_t *ip, PyObject *data);

static OpFn vm_op_table[] = {
    [NT_OP_NULL] = vm_noop,
    [NT_OP_TEXT] = vm_op_text,
    [NT_OP_RENDER] = vm_op_render,
    [NT_OP_NOT] = vm_op_not,
    [NT_OP_CONSTANT] = vm_op_constant,
    [NT_OP_GLOBAL] = vm_op_global,
    [NT_OP_SELECTOR] = vm_op_selector,
    [NT_OP_POP] = vm_op_pop,
    [NT_OP_JUMP] = vm_op_jump,
    [NT_OP_JUMP_IF_FALSY] = vm_op_jump_if_falsy,
    [NT_OP_JUMP_IF_TRUTHY] = vm_op_jump_if_truthy,
    [NT_OP_SET_LOCAL] = vm_op_set_local,
    [NT_OP_GET_LOCAL] = vm_op_get_local,
    [NT_OP_ITER_INIT] = vm_op_iter_init,
    [NT_OP_ITER_NEXT] = vm_op_iter_next,
    [NT_OP_ENTER_FRAME] = vm_op_enter_frame,
    [NT_OP_LEAVE_FRAME] = vm_op_leave_frame,
    [NT_OP_TRUE] = vm_op_true,
    [NT_OP_FALSE] = vm_op_false,
};

/// @brief Read a single operand at offset `offset`.
static int vm_read_operand(NT_VM *vm, uint8_t byte_count, size_t offset);

/// @brief Write `obj` to the output buffer.
static int vm_render(NT_VM *vm, PyObject *obj);

/// @brief Put `obj` on top of the stack and create a new reference to it.
static int vm_push(NT_VM *vm, PyObject *obj);

/// @brief Pop the top off the stack and return a reference owned by the
/// caller.
/// @return A pointer to the popped object, or NULL if the top of the stack was
/// empty with an exception set.
static PyObject *vm_pop(NT_VM *vm);

/// @brief Return a borrowed reference to stack top.
static PyObject *vm_peek(NT_VM *vm);

/// @brief Get an iterator for `obj`.
/// @return 0 on success, -1 on error with an exception set.
static int vm_iter(PyObject *obj, PyObject **out_iter);

static size_t vm_current_frame(NT_VM *vm);
static int vm_push_frame(NT_VM *vm, size_t frame);
static size_t vm_pop_frame(NT_VM *vm);

NT_VM *NT_VM_new(NT_Code *code, PyObject *serializer, PyObject *undefined)
{
    NT_VM *result = NULL;
    NT_VM *vm = NULL;
    PyObject **stack = NULL;
    size_t **frames = NULL;

    NT_VM *vm = PyMem_Malloc(sizeof(NT_VM));
    if (!vm)
    {
        PyErr_NoMemory();
        goto cleanup;
    }

    stack = PyMem_Malloc(sizeof(PyObject *) * NT_VM_STACK_SIZE);
    if (!stack)
    {
        goto cleanup;
    }

    frames = PyMem_Malloc(sizeof(size_t) * NT_VM_MAX_FRAMES);
    if (!frames)
    {
        goto cleanup;
    }

    // Move instructions out of `code`.
    vm->instructions = code->instructions;
    vm->instructions_size = code->instructions_size;
    code->instructions = NULL;
    code->instructions_size = 0;

    // Move constants out of `code`
    vm->constant_pool = code->constant_pool;
    vm->constant_pool_size = code->constant_pool_size;
    code->constant_pool = NULL;
    code->constant_pool_size = 0;

    // Stack top is `stack[sp-1]`
    vm->stack = stack;
    vm->sp = 0;
    stack = NULL;

    // Current frame is `frames[fp-1]`
    vm->frames = frames;
    vm->frames[0] = 0;
    vm->fp = 1;
    frames = NULL;

    Py_INCREF(serializer);
    vm->serializer = serializer;

    Py_INCREF(undefined);
    vm->undefined = undefined;

    // A new buffer is created for every call to `run`.
    vm->buf = NULL;

    result = vm;

cleanup:
    PyMem_Free(stack);
    PyMem_Free(frames);
    PyMem_Free(vm);

    return result;
}

void NT_VM_free(NT_VM *vm)
{
    if (!vm)
    {
        return;
    }

    for (size_t i = 0; i < vm->constant_pool_size; i++)
    {
        Py_XDECREF(vm->constant_pool[i]);
        vm->constant_pool[i] = NULL;
    }

    PyMem_Free(vm->constant_pool);
    vm->constant_pool = NULL;
    vm->constant_pool_size = 0;

    PyMem_Free(vm->instructions);
    vm->instructions = NULL;
    vm->instructions_size = 0;

    Py_XDECREF(vm->serializer);
    vm->serializer = NULL;

    Py_XDECREF(vm->undefined);
    vm->undefined = NULL;

    for (size_t i = 0; i < NT_VM_STACK_SIZE; i++)
    {
        // XXX: Potentially decref-ing many NULLs here.
        Py_XDECREF(vm->stack[i]);
        vm->stack[i] = NULL;
    }

    PyMem_Free(vm->stack);
    vm->stack = NULL;
    vm->sp = 0;

    PyMem_Free(vm->frames);
    vm->frames = NULL;
    vm->fp = 0;

    Py_XDECREF(vm->buf);
    vm->buf = NULL;

    PyMem_Free(vm);
}

int NT_VM_run(NT_VM *vm, PyObject *data)
{
    vm->sp = 0;
    vm->frames[0] = 0;
    vm->fp = 1;

    Py_XDECREF(vm->buf);
    vm->buf = StringBuffer_new();
    if (!vm->buf)
    {
        return -1;
    }

    uint8_t op = 0;
    size_t ip = 0;
    size_t length = vm->instructions_size;

    while (ip < length)
    {
        op = vm->instructions[ip];
        assert(op >= NT_OP_NULL && NT_OP_TRUE);

        OpFn fn = vm_op_table[op];
        if (!fn)
        {
            PyErr_Format(PyExc_RuntimeError, "unknown opcode %d", op);
            return NULL;
        }

        if (fn(vm, &ip, data) < 0)
        {
            return -1;
        }
    }
}

PyObject *NT_VM_join(NT_VM *vm)
{
    PyObject *result = StringBuffer_finish(vm->buf);
    vm->buf = NULL;
    return result;
}

int vm_read_operand(NT_VM *vm, uint8_t byte_count, size_t offset)
{
    assert(offset + byte_count - 1 < vm->instructions_size);

    int value = 0;
    for (int i = 0; i < byte_count; i++)
    {
        value = (value << 8) | vm->instructions[offset + i];
    }
    return value;
}

static int vm_render(NT_VM *vm, PyObject *obj)
{
    PyObject *str = PyObject_CallFunctionObjArgs(vm->serializer, obj, NULL);

    if (!str)
    {
        return -1;
    }

    if (StringBuffer_append(vm->buf, str) < 0)
    {
        Py_DECREF(str);
        return -1;
    }

    Py_DECREF(str);
    return 0;
}

static int vm_push(NT_VM *vm, PyObject *obj)
{
    vm->stack[vm->sp++] = obj;
    Py_INCREF(obj);
    // TODO: check for stack overflow
    return 0;
}

static PyObject *vm_pop(NT_VM *vm)
{
    vm->sp--;

    PyObject *obj = vm->stack[vm->sp];
    if (!obj)
    {
        PyErr_SetString(PyExc_RuntimeError, "unexpected pop of null pointer");
    }

    vm->stack[vm->sp] = NULL;
    return obj;
}

static PyObject *vm_peek(NT_VM *vm)
{
    vm->stack[vm->sp - 1];
}

static size_t vm_current_frame(NT_VM *vm)
{
    return vm->frames[vm->fp];
}

static int vm_push_frame(NT_VM *vm, size_t frame)
{
    vm->frames[vm->fp++] = frame;
    // TODO: check for frame overflow.
    return 0;
}

static size_t vm_pop_frame(NT_VM *vm)
{
    return vm->frames[--vm->fp];
}

static int vm_noop(NT_VM *vm, size_t *ip, PyObject *data)
{
    return 0;
}

static int vm_op_text(NT_VM *vm, size_t *ip, PyObject *data)
{
    int constant_index = vm_read_operand(vm, 2, *ip + 1);

    if (vm_render(vm, vm->constant_pool[constant_index]) < 0)
    {
        return -1;
    }

    *ip += 3;
    return 0;
}

static int vm_op_render(NT_VM *vm, size_t *ip, PyObject *data)
{
    PyObject *obj = vm_pop(vm);
    if (!obj)
    {
        return -1;
    }

    if (vm_render(vm, obj) < 0)
    {
        return -1;
    }

    *ip += 1;
    return 0;
}

static int vm_op_not(NT_VM *vm, size_t *ip, PyObject *data)
{
    PyObject *obj = vm_pop(vm);
    if (!obj)
    {
        return -1;
    }

    int not_obj = PyObject_IsFalse(obj);
    Py_DECREF(obj);

    if (vm_push(vm, not_obj ? Py_True : Py_False) < 0)
    {
        return -1;
    }

    *ip += 1;
    return 0;
}

static int vm_op_constant(NT_VM *vm, size_t *ip, PyObject *data)
{
    int constant_index = vm_read_operand(vm, 2, *ip + 1);

    if (vm_push(vm, vm->constant_pool[constant_index]) < 0)
    {
        return -1;
    }

    *ip += 3;
    return 0;
}

static int vm_op_global(NT_VM *vm, size_t *ip, PyObject *data)
{
    int constant_index = vm_read_operand(vm, 2, *ip + 1);

    PyObject *obj = PyObject_GetItem(data, vm->constant_pool[constant_index]);
    if (!obj)
    {
        // TODO: undefined constructor.
        NTPY_TODO_I();
    }

    if (vm_push(vm, obj) < 0)
    {
        return -1;
    }

    *ip += 3;
    return 0;
}

static int vm_op_selector(NT_VM *vm, size_t *ip, PyObject *data)
{
    int constant_index = vm_read_operand(vm, 2, *ip + 1);

    PyObject *obj = vm_pop(vm);
    if (!obj)
    {
        return -1;
    }

    Py_DECREF(obj);

    PyObject *new_obj = PyObject_GetItem(data, obj);
    if (!obj)
    {
        // TODO: undefined constructor.
        NTPY_TODO_I();
    }

    if (vm_push(vm, obj) < 0)
    {
        return -1;
    }

    *ip += 3;
    return 0;
}

static int vm_op_pop(NT_VM *vm, size_t *ip, PyObject *data)
{
    int result = vm_pop(vm);
    *ip++;
    return result;
}

static int vm_op_jump(NT_VM *vm, size_t *ip, PyObject *data)
{
    *ip = vm_read_operand(vm, 2, *ip + 1);
    return 0;
}

static int vm_op_jump_if_falsy(NT_VM *vm, size_t *ip, PyObject *data)
{
    size_t jump_pos = vm_read_operand(vm, 2, *ip + 1);
    PyObject *obj = vm_peek(vm);
    *ip = PyObject_IsFalse(obj) ? jump_pos : ip + 3;
    return 0;
}

static int vm_op_jump_if_truthy(NT_VM *vm, size_t *ip, PyObject *data)
{
    size_t jump_pos = vm_read_operand(vm, 2, *ip + 1);
    PyObject *obj = vm_peek(vm);
    *ip = PyObject_IsTrue(obj) ? jump_pos : ip + 3;
    return 0;
}

static int vm_op_set_local(NT_VM *vm, size_t *ip, PyObject *data)
{
    size_t local_index = vm_read_operand(vm, 1, *ip + 1);

    PyObject *obj = vm_pop(vm);
    if (!obj)
    {
        return -1;
    }

    // Don't decref obj as we're inserting it back into the stack.
    vm->stack[vm_current_frame(vm) + local_index] = obj;
    *ip += 2;
    return 0;
}

static int vm_op_get_local(NT_VM *vm, size_t *ip, PyObject *data)
{
    size_t depth = vm_read_operand(vm, 1, *ip + 1);
    size_t offset = vm_read_operand(vm, 1, *ip + 2);

    size_t stack_pointer = vm->frames[vm->fp - 1 - depth] + offset;
    if (vm_push(vm, vm->stack[stack_pointer]) < 0)
    {
        return -1;
    }

    *ip += 3;
    return 0;
}

static int vm_op_iter_init(NT_VM *vm, size_t *ip, PyObject *data)
{
    PyObject *it = NULL;

    PyObject *obj = vm_pop(vm);
    if (!obj)
    {
        return -1;
    }

    if (vm_iter(obj, &it) < 0)
    {
        Py_DECREF(obj);
        return -1;
    }

    Py_DECREF(obj);

    if (vm_push(vm, it) < 0)
    {
        Py_DECREF(it);
        return -1;
    }

    Py_DECREF(it);

    *ip++;
    return 0;
}

static int vm_op_iter_next(NT_VM *vm, size_t *ip, PyObject *data)
{
    PyObject *it = vm->stack[vm->sp - 1];

    PyObject *value = PyIter_Next(it);
    if (!value)
    {
        if (PyErr_Occurred())
        {
            return -1;
        }

        if (vm_push(vm, Py_False) < 0)
        {
            return -1;
        }
    }

    if (vm_push(vm, value) < 0)
    {
        Py_DECREF(value);
        return -1;
    }

    Py_DECREF(value);

    if (vm_push(vm, Py_True) < 0)
    {
        return -1;
    }

    return 0;
}

static int vm_op_enter_frame(NT_VM *vm, size_t *ip, PyObject *data)
{
    int n_locals = vm_read_operand(vm, 1, vm->sp + 1);

    if (vm_push_frame(vm, vm->sp) < 0)
    {
        return -1;
    }

    vm->sp += n_locals;
    *ip += 2;
}

static int vm_op_leave_frame(NT_VM *vm, size_t *ip, PyObject *data)
{
    size_t frame = vm_pop_frame(vm);
    vm->sp = frame;
    *ip++;
    return 0;
}

static int vm_op_true(NT_VM *vm, size_t *ip, PyObject *data)
{
    if (vm_push(vm, Py_True) < 0)
    {
        return -1;
    }

    *ip++;
    return 0;
}

static int vm_op_false(NT_VM *vm, size_t *ip, PyObject *data)
{
    if (vm_push(vm, Py_False) < 0)
    {
        return -1;
    }

    *ip++;
    return 0;
}

static int vm_iter(PyObject *op, PyObject **out_iter)
{
    PyObject *it = NULL;
    *out_iter = NULL;

    PyObject *items = PyMapping_Items(op);

    if (items)
    {
        it = PyObject_GetIter(items);
        Py_DECREF(items);

        if (!it)
        {
            return -1;
        }

        *out_iter = it;
        return 0;
    }

    if (PyErr_Occurred())
    {
        PyErr_Clear();
    }

    it = PyObject_GetIter(op);
    if (it)
    {
        *out_iter = it;
        return 0;
    }

    if (PyErr_Occurred())
    {
        PyErr_Clear();
    }

    *out_iter = PyList_New(0);
    return 0;
}