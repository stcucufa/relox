#ifdef DEBUG
#include <stdio.h>
#endif

#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "compiler.h"
#include "vm.h"

void chunk_init(Chunk* chunk) {
    byte_array_init(&chunk->bytes);
    number_array_init(&chunk->line_numbers);
    value_array_init(&chunk->values);
}

void chunk_add_byte(Chunk* chunk, uint8_t byte, size_t line_number) {
    byte_array_push(&chunk->bytes, byte);
    size_t n = chunk->line_numbers.count;
    if (n == 0 || chunk->line_numbers.items[n - 2] != line_number) {
        number_array_push(&chunk->line_numbers, line_number);
        number_array_push(&chunk->line_numbers, 1);
    } else {
        chunk->line_numbers.items[n - 1] += 1;
    }
}

uint8_t chunk_add_constant(Chunk* chunk, HAMT* constants, Value v) {
    // TODO too many constants (more than UINT8_MAX)
    Value j = hamt_get(constants, v);
    if (!VALUE_IS_NONE(j)) {
        return (uint8_t)VALUE_TO_INT(j);
    }
    size_t i = chunk->values.count;
    value_array_push(&chunk->values, v);
    hamt_set(constants, v, VALUE_FROM_INT(i));
    return (uint8_t)i;
}

void chunk_free(Chunk* chunk) {
    byte_array_free(&chunk->bytes);
    number_array_free(&chunk->line_numbers);
    value_array_free(&chunk->values);
    chunk_init(chunk);
}

#ifdef DEBUG

char const*const opcodes[opcode_count] = {
    [op_nil] = "nil",
    [op_zero] = "zero",
    [op_one] = "one",
    [op_infinity] = "infinity",
    [op_epsilon] = "epsilon",
    [op_constant] = "constant",
    [op_negate] = "negate",
    [op_add] = "add",
    [op_subtract] = "subtract",
    [op_multiply] = "multiply",
    [op_divide] = "divide",
    [op_exponent] = "exponent",
    [op_false] = "false",
    [op_true] = "true",
    [op_not] = "not",
    [op_eq] = "eq",
    [op_ne] = "ne",
    [op_gt] = "gt",
    [op_ge] = "ge",
    [op_lt] = "lt",
    [op_le] = "le",
    [op_bars] = "bars",
    [op_quote] = "quote",
    [op_print] = "print",
    [op_pop] = "pop",
    [op_dup] = "dup",
    [op_define_global] = "define/global",
    [op_get_global] = "get/global",
    [op_set_global] = "set/global",
    [op_get_local] = "get/local",
    [op_set_local] = "set/local",
    [op_jump] = "jump",
    [op_jump_true] = "jump/true",
    [op_jump_false] = "jump/false",
    [op_call] = "call",
    [op_return] = "return",
    [op_nop] = "nop",
};

void chunk_debug(Chunk* chunk, const char* name) {
    fprintf(stderr, "*** ----8<---- %s ----8<----\n", name);
    for (size_t i = 0, j = 0, k = 0; i < chunk->bytes.count;) {
        uint8_t opcode = chunk->bytes.items[i];
        size_t line = chunk->line_numbers.items[j];
        fprintf(stderr, "*** %4zu %4zu  %02x ", i, line, opcode);
        i += 1;
        k += 1;
        switch (opcode) {
            case op_constant: {
                uint8_t arg = chunk->bytes.items[i];
                fprintf(stderr, "%02x     %s ", arg, opcodes[opcode]);
                value_print_debug(stderr, chunk->values.items[arg], true);
                fputc('\n', stderr);
                i += 1;
                k += 1;
                break;
            }
            case op_define_global:
            case op_get_global:
            case op_set_global: {
                uint8_t arg = chunk->bytes.items[i];
                fprintf(stderr, "%02x     %s ", arg, opcodes[opcode]);
                value_print_debug(stderr, hamt_get(&chunk->vm->global_scope, VALUE_FROM_INT(arg)), true);
                fputc('\n', stderr);
                i += 1;
                k += 1;
                break;
            }
            case op_get_local:
            case op_set_local:
            case op_call: {
                uint8_t arg = chunk->bytes.items[i];
                fprintf(stderr, "%02x     %s %d\n", arg, opcodes[opcode], arg);
                i += 1;
                k += 1;
                break;
            }
            case op_jump:
            case op_jump_true:
            case op_jump_false: {
                uint8_t hi = chunk->bytes.items[i];
                uint8_t lo = chunk->bytes.items[i + 1];
                ptrdiff_t offset = (int16_t)((hi << 8) | lo);
                i += 2;
                fprintf(stderr, "%02x %02x  %s -> %zu\n", hi, lo, opcodes[opcode], i + offset);
                k += 1;
                break;
            }
            default:
                fprintf(stderr, "       %s\n", opcodes[opcode]);
                break;
        }
        if (k == chunk->line_numbers.items[j + 1]) {
            j += 2;
            k = 0;
        }
    }
    fprintf(stderr, "*** ----8<---- %s ----8<----\n", name);
}

#endif

static Result vm_runtime_error(VM* vm, const char* format, ...) {
    fputs("\n", stderr);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    // FIXME 2L05 Improve error handling
    // fprintf(stderr, "[line %d] in script\n", line);
    return result_runtime_error;
}

Value vm_add_object(VM* vm, Value v) {
    if (VALUE_IS_STRING(v)) {
        if (VALUE_IS_EPSILON(v) || VALUE_IS_SHORT_STRING(v)) {
            return v;
        }
        String* string = VALUE_TO_STRING(v);
        Value interned = hamt_get_string(&vm->strings, string);
        if (VALUE_IS_STRING(interned)) {
            if (!VALUE_EQUAL(v, interned)) {
                free(string);
            }
            return interned;
        }
        hamt_set(&vm->strings, v, v);
    }
    value_array_push(&vm->objects, v);
    return v;
}

Var* vm_var_new(VM* vm, size_t index, bool mutable, bool global) {
    Var* var = malloc(sizeof(Var));
    var->index = (uint8_t)index;
    var->initialized = false;
    var->mutable = mutable;
    var->global = global;
#ifdef DEBUG
    fprintf(stderr, "+++ vm_var_new(): new var %p\n", (void*)var);
#endif
    value_array_push(&vm->objects, VALUE_FROM_POINTER(var));
    return var;
}

Var* vm_add_global(VM* vm, Value v, bool mutable) {
    Value w = hamt_get(&vm->global_scope, v);
    if (!VALUE_IS_NONE(w)) {
        return (Var*)VALUE_TO_POINTER(w);
    }

    Var* var = vm_var_new(vm, vm->globals.count, mutable, true);
    hamt_set(&vm->global_scope, v, VALUE_FROM_POINTER(var));
    hamt_set(&vm->global_scope, VALUE_FROM_NUMBER(var->index), v);
    value_array_push(&vm->globals, VALUE_NONE);
    return var;
}

static Frame* vm_call(VM* vm, Value v, uint8_t args_count) {
    if (!VALUE_IS_FUNCTION(v)) {
        vm_runtime_error(vm, "Cannot call a non-function value.");
        return 0;
    }

    if (VALUE_IS_FOREIGN_FUNCTION(v)) {
        Value result = (*VALUE_TO_FOREIGN_FUNCTION(v))(args_count, vm->sp - args_count);
        vm->sp += args_count;
        *vm->sp++ = result;
        return &vm->frames[vm->frame_count - 1];

    } else {
        Function* function = VALUE_TO_FUNCTION(v);
        if (args_count != function->arity) {
            vm_runtime_error(vm,
                "Call with number of arguments mismatch: got %d, expected %d\n", args_count, function->arity);
            return 0;
        }

        if (vm->frame_count == FRAMES_MAX) {
            vm_runtime_error(vm, "Stack overflow");
            return 0;
        }

        // FIXME 2L05 Improve error handling
        // Show stack trace on error.

        Frame* frame = &vm->frames[vm->frame_count];
        vm->frame_count += 1;
        frame->function = function;
        frame->ip = function->chunk->bytes.items;
        frame->slots = vm->sp - args_count;
        return frame;
    }
}

Result vm_run(VM* vm) {
    Frame* frame = &vm->frames[vm->frame_count - 1];
    frame->ip = frame->function->chunk->bytes.items;

#define BYTE() *frame->ip++
#define WORD() (((*frame->ip++) << 8) | (*frame->ip++))
#define CONSTANT() frame->function->chunk->values.items[BYTE()]

#define PUSH(x) *vm->sp++ = (x)
#define POP() (*(--vm->sp))
#define PEEK(i) (*(vm->sp - 1 - (i)))
#define POKE(i, x) *(vm->sp - 1 - (i)) = (x)

#define BINARY_OP_NUMBER(op) do { \
    if (!VALUE_IS_NUMBER(PEEK(1))) { \
        return vm_runtime_error(vm, "First operand of arithmetic operation is not a number."); \
    } \
    if (!VALUE_IS_NUMBER(PEEK(0))) { \
        return vm_runtime_error(vm, "Second operand of arithmetic operation is not a number."); \
    } \
    double v = POP().as_double; \
    POKE(0, VALUE_FROM_NUMBER(PEEK(0).as_double op v)); \
} while (0)
#define BINARY_OP_BOOLEAN(op) do { \
    if (!VALUE_IS_NUMBER(PEEK(1))) { \
        return vm_runtime_error(vm, "First operand of comparison operation is not a number."); \
    } \
    if (!VALUE_IS_NUMBER(PEEK(0))) { \
        return vm_runtime_error(vm, "Second operand of comparison operation is not a number."); \
    } \
    double v = POP().as_double; \
    POKE(0, (PEEK(0).as_double op v) ? VALUE_TRUE : VALUE_FALSE); \
} while (0)

    uint8_t opcode;
    while (true) {
#ifdef DEBUG
        fprintf(stderr, "~~~ %4zu ", frame->ip - frame->function->chunk->bytes.items);
#endif
        switch (opcode = BYTE()) {
            case op_nil: PUSH(VALUE_NIL); break;
            case op_zero: PUSH(VALUE_FROM_NUMBER(0)); break;
            case op_one: PUSH(VALUE_FROM_NUMBER(1)); break;
            case op_infinity: PUSH(VALUE_FROM_NUMBER(INFINITY)); break;
            case op_epsilon: PUSH(VALUE_EPSILON); break;
            case op_constant: PUSH(CONSTANT()); break;
            case op_negate:
                if (!VALUE_IS_NUMBER(PEEK(0))) {
                    return vm_runtime_error(vm, "Operand for negate is not a number.");
                }
                POKE(0, VALUE_FROM_NUMBER(-PEEK(0).as_double));
                break;
            case op_add: BINARY_OP_NUMBER(+); break;
            case op_subtract: BINARY_OP_NUMBER(-); break;
            case op_multiply: {
                if (VALUE_IS_STRING(PEEK(0)) && VALUE_IS_STRING(PEEK(1))) {
                    Value v = POP();
                    Value string = value_concatenate_strings(PEEK(0), v);
                    vm_add_object(vm, string);
                    POKE(0, string);
                } else {
                    BINARY_OP_NUMBER(*);
                }
                break;
            }
            case op_divide: BINARY_OP_NUMBER(/); break;
            case op_exponent: {
                if (!VALUE_IS_NUMBER(PEEK(0))) {
                    return vm_runtime_error(vm, "Exponent is not a number.");
                }
                double exponent = POP().as_double;
                Value base = PEEK(0);
                if (VALUE_IS_NUMBER(base)) {
                    POKE(0, VALUE_FROM_NUMBER(pow(base.as_double, exponent)));
                } else if (VALUE_IS_STRING(base)) {
                    POKE(0, value_string_exponent(base, exponent));
                } else {
                    return vm_runtime_error(vm, "Base of exponent is not a number or a string.");
                }
                break;
            }
            case op_false: PUSH(VALUE_FALSE); break;
            case op_true: PUSH(VALUE_TRUE); break;
            case op_not:
                POKE(0, (VALUE_IS_FALSE(PEEK(0)) || VALUE_IS_NIL(PEEK(0))) ? VALUE_TRUE : VALUE_FALSE);
                break;
            case op_eq: {
                Value v = POP();
                POKE(0, VALUE_EQUAL(PEEK(0), v) ? VALUE_TRUE : VALUE_FALSE);
                break;
            }
            case op_ne: {
                Value v = POP();
                POKE(0, VALUE_EQUAL(PEEK(0), v) ? VALUE_FALSE : VALUE_TRUE);
                break;
            }
            case op_gt: BINARY_OP_BOOLEAN(>); break;
            case op_ge: BINARY_OP_BOOLEAN(>=); break;
            case op_lt: BINARY_OP_BOOLEAN(<); break;
            case op_le: BINARY_OP_BOOLEAN(<=); break;
            case op_bars: {
                Value v = PEEK(0);
                if (VALUE_IS_STRING(v)) {
                    // FIXME 2L0N || should handle unicode strings (?)
                    // This is the length in bytes, not unicode characters.
                    POKE(0, VALUE_FROM_NUMBER(VALUE_IS_EPSILON(v) ? 0 :
                        VALUE_IS_SHORT_STRING(v) ? VALUE_SHORT_STRING_LENGTH(v) :
                        (VALUE_TO_STRING(v)->length)));
                } else if (VALUE_IS_NUMBER(v)) {
                    POKE(0, VALUE_FROM_NUMBER(fabs(v.as_double)));
                } else {
                    return vm_runtime_error(vm, "Bars apply to number or string.");
                }
                break;
            }
            case op_quote: POKE(0, value_stringify(PEEK(0))); break;
            case op_print:
                value_print(POP());
                puts("");
                break;

            case op_pop: (void)POP(); break;
            case op_dup: {
                Value top = PEEK(0);
                PUSH(top);
                break;
            }

            case op_define_global: vm->globals.items[BYTE()] = POP(); break;
            case op_get_global: {
                uint8_t n = BYTE();
                Value value = vm->globals.items[n];
                if (VALUE_IS_NONE(value)) {
                    return vm_runtime_error(vm, "undefined var \"%s\"",
                        value_to_cstring(hamt_get(&vm->global_scope, VALUE_FROM_INT(n))));
                }
                PUSH(value);
                break;
            }
            case op_set_global: {
                uint8_t n = BYTE();
                if (VALUE_IS_NONE(vm->globals.items[n])) {
                    return vm_runtime_error(vm, "undefined var \"%s\"",
                        value_to_cstring(hamt_get(&vm->global_scope, VALUE_FROM_INT(n))));
                }
                vm->globals.items[n] = PEEK(0);
                break;
            }
            case op_get_local: PUSH(frame->slots[BYTE()]); break;
            case op_set_local: frame->slots[BYTE()] = PEEK(0); break;

            case op_jump: frame->ip += (int16_t)WORD(); break;
            case op_jump_true: {
                ptrdiff_t offset = (int16_t)WORD();
                Value p = PEEK(0);
                if (!(VALUE_IS_FALSE(p) || VALUE_IS_EPSILON(p) || p.as_double == 0)) {
                    frame->ip += offset;
                }
                break;
            }
            case op_jump_false: {
                ptrdiff_t offset = (int16_t)WORD();
                Value p = PEEK(0);
                if (VALUE_IS_FALSE(p) || VALUE_IS_EPSILON(p) || p.as_double == 0) {
                    frame->ip += offset;
                }
                break;
            }

            case op_call: {
                uint8_t args_count = BYTE();
                frame = vm_call(vm, PEEK(args_count), args_count);
                if (!frame) {
                    return result_runtime_error;
                }
                break;
            }

            case op_nop: break;

            case op_return: {
                Value result = POP();
                vm->frame_count -= 1;
                if (vm->frame_count == 0) {
                    POP();
                    return result_ok;
                }
                vm->sp = frame->slots;
                PUSH(result);
                frame = &vm->frames[vm->frame_count - 1];
                break;
            }
        }
#ifdef DEBUG
        fprintf(stderr, "%s {", opcodes[opcode]);
        for (Value* sp = vm->stack; sp != vm->sp; ++sp) {
            fputc(' ', stderr);
            value_print_debug(stderr, *sp, true);
        }
        fprintf(stderr, " }\n");
#endif
    }

    // unreachable
    return result_runtime_error;

#undef BYTE
#undef WORD
#undef CONSTANT
#undef PUSH
#undef POP
#undef PEEK
#undef POKE
#undef BINARY_OP_NUMBER
#undef BINARY_OP_BOOLEAN
}

static void vm_foreign_function(VM* vm, const char* name, ForeignFunction function) {
    Var* var = vm_var_new(vm, vm->globals.count, false, true);
    Value v = value_copy_string(name, strlen(name));
    hamt_set(&vm->global_scope, v, VALUE_FROM_POINTER(var));
    hamt_set(&vm->global_scope, VALUE_FROM_NUMBER(var->index), v);
    value_array_push(&vm->globals, VALUE_FROM_FOREIGN_FUNCTION(function));
}

static Value foreign_clock(size_t arg_count, Value* args) {
    return VALUE_FROM_NUMBER((double)clock() / CLOCKS_PER_SEC);
}

Result vm_compile_and_run(VM* vm, const char* source) {
    Function* function = function_new();
    function->chunk->vm = vm;
    hamt_init(&vm->global_scope);
    hamt_init(&vm->strings);
    value_array_init(&vm->objects);
    value_array_init(&vm->globals);

    vm_foreign_function(vm, "clock", foreign_clock);

    if (!compile_function(source, function)) {
        function_free(function);
        return result_compile_error;
    }

#ifdef DEBUG
    chunk_debug(function->chunk, "Top-level function");
#endif

    Frame* frame = &vm->frames[vm->frame_count];
    frame->function = function;
    frame->slots = vm->stack;
    vm->frame_count = 1;
    vm->sp = vm->stack;
    Result result = vm_run(vm);
    function_free(function);
    return result;
}

void vm_free(VM* vm) {
#ifdef DEBUG
    hamt_debug(&vm->global_scope);
    hamt_debug(&vm->strings);
#endif

    hamt_free(&vm->global_scope);
    hamt_free(&vm->strings);
    for (size_t i = 0; i < vm->objects.count; ++i) {
        value_free_object(vm->objects.items[i]);
    }
    value_array_free(&vm->objects);
    value_array_free(&vm->globals);
}
