// SPDX-License-Identifier: MIT

#include "nano_template/py_bytecode_view.h"
#include "nano_template/lexer.h"
#include "nano_template/parser.h"

static PyTypeObject *BytecodeView_TypeObject = NULL;

PyObject *NTPY_BytecodeView_new(NT_Code *code)
{
    if (!BytecodeView_TypeObject)
    {
        PyErr_SetString(PyExc_RuntimeError,
                        "BytecodeView type is not initialized");
        return NULL;
    }

    PyObject *obj = PyType_GenericNew(BytecodeView_TypeObject, NULL, NULL);
    if (!obj)
    {
        return NULL;
    }

    NTPY_BytecodeViewObject *op = (NTPY_BytecodeViewObject *)obj;
    if (!op)
    {
        return NULL;
    }

    PyObject *py_instructions = NULL;
    PyObject *py_constant_pool = NULL;

    py_instructions = PyBytes_FromStringAndSize((char *)code->instructions,
                                                code->instructions_size);

    if (!py_instructions)
    {
        goto fail;
    }

    py_constant_pool = PyList_New(code->constant_pool_size);
    if (!py_constant_pool)
    {
        goto fail;
    }

    for (size_t i = 0; i < code->constant_pool_size; i++)
    {
        if (PyList_SetItem(py_constant_pool, i, code->constant_pool[i]) < 0)
        {
            goto fail;
        }

        Py_INCREF(code->constant_pool[i]);
    }

    op->instructions = py_instructions;
    op->constant_pool = py_constant_pool;

    return obj;

fail:
    Py_XDECREF(py_instructions);
    Py_XDECREF(py_constant_pool);
    return NULL;
}

static void NTPY_BytecodeView_free(PyObject *self)
{
    NTPY_BytecodeViewObject *op = (NTPY_BytecodeViewObject *)self;
    Py_XDECREF(op->instructions);
    Py_XDECREF(op->constant_pool);
    op->instructions = NULL;
    op->constant_pool = NULL;
    PyObject_Free(op);
}

static PyObject *NTPY_BytecodeView_instructions(PyObject *self,
                                                void *Py_UNUSED(closure))
{
    NTPY_BytecodeViewObject *op = (NTPY_BytecodeViewObject *)self;

    if (!op->instructions)
    {
        Py_RETURN_NONE;
    }

    Py_INCREF(op->instructions);
    return op->instructions;
}

static PyObject *NTPY_BytecodeView_constants(PyObject *self,
                                             void *Py_UNUSED(closure))
{
    NTPY_BytecodeViewObject *op = (NTPY_BytecodeViewObject *)self;

    if (!op->constant_pool)
    {
        Py_RETURN_NONE;
    }

    Py_INCREF(op->constant_pool);
    return op->constant_pool;
}

static PyGetSetDef NTPY_BytecodeView_getset[] = {
    {"instructions", NTPY_BytecodeView_instructions, NULL,
     "bytecode instructions", NULL},
    {"constants", NTPY_BytecodeView_constants, NULL, "bytecode constants",
     NULL},
    {NULL, NULL, NULL, NULL, NULL}};

static PyType_Slot NTPY_BytecodeView_slots[] = {
    {Py_tp_doc, "Lightweight token view into source text"},
    {Py_tp_free, (void *)NTPY_BytecodeView_free},
    {Py_tp_getset, (void *)NTPY_BytecodeView_getset},
    {0, NULL}};

static PyType_Spec NTPY_BytecodeView_spec = {
    .name = "nano_template.BytecodeView",
    .basicsize = sizeof(NTPY_BytecodeViewObject),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = NTPY_BytecodeView_slots,
};

PyObject *NTPY_bytecode(PyObject *Py_UNUSED(self), PyObject *str)
{
    Py_ssize_t token_count = 0;
    NT_Token *tokens = NULL;
    NT_Lexer *lexer = NULL;
    NT_Parser *parser = NULL;

    NT_Mem *ast = NULL;
    NT_Node *root = NULL;
    NT_Compiler *compiler = NULL;
    NT_Code *bytecode = NULL;
    PyObject *bytecode_view = NULL;

    if (!PyUnicode_Check(str))
    {
        PyErr_SetString(PyExc_TypeError,
                        "bytecode() argument must be a string");
        goto cleanup;
    }

    lexer = NT_Lexer_new(str);
    if (!lexer)
    {
        goto cleanup;
    }

    tokens = NT_Lexer_scan(lexer, &token_count);
    if (!tokens)
    {
        goto cleanup;
    }

    ast = NT_Mem_new();
    if (!ast)
    {
        goto cleanup;
    }

    parser = NT_Parser_new(ast, str, tokens, token_count);
    if (!parser)
    {
        goto cleanup;
    }

    tokens = NULL;

    root = NT_Parser_parse_root(parser);
    if (!root)
    {
        goto cleanup;
    }

    compiler = NT_Compiler_new();
    if (!compiler)
    {
        goto cleanup;
    }

    if (NT_Compiler_compile(compiler, root) < 0)
    {
        goto cleanup;
    }

    bytecode = NT_Compiler_bytecode(compiler);
    bytecode_view = NTPY_BytecodeView_new(bytecode);

cleanup:
    if (tokens)
    {
        PyMem_Free(tokens);
        tokens = NULL;
    }

    if (lexer)
    {
        NT_Lexer_free(lexer);
        lexer = NULL;
    }

    if (parser)
    {
        NT_Parser_free(parser);
        parser = NULL;
    }

    if (ast)
    {
        NT_Mem_free(ast);
        ast = NULL;
        // `root` is allocated and freed by `ast`
        root = NULL;
    }

    if (compiler)
    {
        NT_Compiler_free(compiler);
        compiler = NULL;
    }

    if (bytecode)
    {
        NT_Code_free(bytecode);
        bytecode = NULL;
    }

    return bytecode_view;
}

int nt_register_bytecode_view_type(PyObject *module)
{
    PyObject *type_obj = PyType_FromSpec(&NTPY_BytecodeView_spec);
    if (!type_obj)
    {
        return -1;
    }

    /* Store type object for future factories */
    BytecodeView_TypeObject = (PyTypeObject *)type_obj;

    if (PyModule_AddObject(module, "BytecodeView", type_obj) < 0)
    {
        Py_DECREF(type_obj);
        BytecodeView_TypeObject = NULL;
        return -1;
    }

    return 0;
}