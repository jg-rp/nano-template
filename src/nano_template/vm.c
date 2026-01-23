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

static int vm_push(NT_VM *vm, PyObject *obj);
static PyObject *vm_pop(NT_VM *vm);
static PyObject *vm_peek(NT_VM *vm);
static size_t *vm_current_frame(NT_VM *vm);
static void *vm_push_frame(NT_VM *vm, size_t frame);
static size_t *vm_pop_frame(NT_VM *vm);

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

        fn(vm, &ip, data);
    }
}

PyObject *NT_VM_join(NT_VM *vm)
{
    PyObject *result = StringBuffer_finish(vm->buf);
    vm->buf = NULL;
    return result;
}