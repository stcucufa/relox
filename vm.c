#ifdef DEBUG
#include <stdio.h>
#endif

#include <math.h>
#include <stdarg.h>

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

size_t chunk_add_constant(Chunk* chunk, HAMT* constants, Value v) {
    // TODO too many constants (more than UINT8_MAX)
    Value j = hamt_get(constants, v);
    if (!VALUE_IS_NONE(j)) {
        return (size_t)VALUE_TO_INT(j);
    }
    size_t i = chunk->values.count;
    value_array_push(&chunk->values, v);
    hamt_set(constants, v, VALUE_FROM_INT(i));
    return i;
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
    [op_quote] = "quote",
    [op_print] = "print",
    [op_pop] = "pop",
    [op_define_global] = "define/global",
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
            case op_nil:
            case op_zero:
            case op_one:
            case op_infinity:
            case op_negate:
            case op_add:
            case op_subtract:
            case op_multiply:
            case op_divide:
            case op_exponent:
            case op_false:
            case op_true:
            case op_not:
            case op_eq:
            case op_ne:
            case op_gt:
            case op_ge:
            case op_lt:
            case op_le:
            case op_quote:
            case op_print:
            case op_pop:
            case op_return:
            case op_nop:
                fprintf(stderr, "    %s\n", opcodes[opcode]);
                break;
            case op_constant:
            case op_define_global: {
                uint8_t arg = chunk->bytes.items[i];
                fprintf(stderr, "%02x  %s ", arg, opcodes[opcode]);
                value_printf(stderr, chunk->values.items[arg]);
                fputc('\n', stderr);
                i += 1;
                k += 1;
                break;
             }
            default:
                fprintf(stderr, "???\n");
        }
        if (k == chunk->line_numbers.items[j + 1]) {
            j += 2;
            k = 0;
        }
    }
    fprintf(stderr, "*** ----8<---- %s ----8<----\n", name);
}

#endif

static Result runtime_error(VM* vm, const char* format, ...) {
    fputs("\n", stderr);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    // TODO fprintf(stderr, "[line %d] in script\n", line);
    return result_runtime_error;
}

Value vm_add_object(VM* vm, Value v) {
    if (VALUE_IS_STRING(v)) {
        String* string = VALUE_TO_STRING(v);
        Value interned = hamt_get_string(&vm->strings, string);
        if (VALUE_IS_STRING(interned)) {
            free(string);
            return interned;
        }
        hamt_set(&vm->strings, v, v);
    }
    value_array_push(&vm->objects, v);
    return v;
}

Result vm_run(VM* vm, const char* source) {
    Chunk chunk;
    chunk_init(&chunk);
    chunk.vm = vm;
    hamt_init(&vm->strings);
    hamt_init(&vm->globals);
    value_array_init(&vm->objects);
    if (!compile_chunk(source, &chunk)) {
        chunk_free(&chunk);
        return result_compile_error;
    }

#ifdef DEBUG
    chunk_debug(&chunk, "Compiled chunk");
#endif

    vm->chunk = &chunk;
    vm->ip = chunk.bytes.items;
    vm->sp = vm->stack;

#define BYTE() *vm->ip++
#define CONSTANT() chunk.values.items[BYTE()]
#define PUSH(x) *vm->sp++ = (x)
#define POP() (*(--vm->sp))
#define PEEK(i) (*(vm->sp - 1 - (i)))
#define POKE(i, x) *(vm->sp - 1 - (i)) = (x)
#define BINARY_OP_NUMBER(op) do { \
    if (!VALUE_IS_NUMBER(PEEK(1))) { \
        return runtime_error(vm, "First operand of arithmetic operation is not a number."); \
    } \
    if (!VALUE_IS_NUMBER(PEEK(0))) { \
        return runtime_error(vm, "Second operand of arithmetic operation is not a number."); \
    } \
    double v = POP().as_double; \
    POKE(0, VALUE_FROM_NUMBER(PEEK(0).as_double op v)); \
} while (0)
#define BINARY_OP_BOOLEAN(op) do { \
    if (!VALUE_IS_NUMBER(PEEK(1))) { \
        return runtime_error(vm, "First operand of comparis operation is not a number."); \
    } \
    if (!VALUE_IS_NUMBER(PEEK(0))) { \
        return runtime_error(vm, "Second operand of comparison operation is not a number."); \
    } \
    double v = POP().as_double; \
    POKE(0, (PEEK(0).as_double op v) ? VALUE_TRUE : VALUE_FALSE); \
} while (0)


    uint8_t opcode;
    do {
#ifdef DEBUG
        fprintf(stderr, "~~~ %4zu ", vm->ip - chunk.bytes.items);
#endif
        switch (opcode = BYTE()) {
            case op_nil: PUSH(VALUE_NIL); break;
            case op_zero: PUSH(VALUE_FROM_NUMBER(0)); break;
            case op_one: PUSH(VALUE_FROM_NUMBER(1)); break;
            case op_infinity: PUSH(VALUE_FROM_NUMBER(INFINITY)); break;
            case op_constant: PUSH(CONSTANT()); break;
            case op_negate:
                if (!VALUE_IS_NUMBER(PEEK(0))) {
                    return runtime_error(vm, "Operand for negate is not a number.");
                }
                POKE(0, VALUE_FROM_NUMBER(-PEEK(0).as_double));
                break;
            case op_add: BINARY_OP_NUMBER(+); break;
            case op_subtract: BINARY_OP_NUMBER(-); break;
            case op_multiply: {
                if (VALUE_IS_STRING(PEEK(0)) && VALUE_IS_STRING(PEEK(1))) {
                    Value v = POP();
                    Value string = VALUE_FROM_STRING(
                        string_concatenate(VALUE_TO_STRING(PEEK(0)), VALUE_TO_STRING(v))
                    );
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
                    return runtime_error(vm, "Exponent is not a number.");
                }
                double exponent = POP().as_double;
                Value base = PEEK(0);
                if (VALUE_IS_NUMBER(base)) {
                    POKE(0, VALUE_FROM_NUMBER(pow(base.as_double, exponent)));
                } else if (VALUE_IS_STRING(base)) {
                    POKE(0, VALUE_FROM_STRING(string_exponent(VALUE_TO_STRING(base), exponent)));
                } else {
                    return runtime_error(vm, "Base of exponent is not a number or a string.");
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
                POKE(0, value_equal(PEEK(0), v) ? VALUE_TRUE : VALUE_FALSE);
                break;
            }
            case op_ne: {
                Value v = POP();
                POKE(0, value_equal(PEEK(0), v) ? VALUE_FALSE : VALUE_TRUE);
                break;
            }
            case op_gt: BINARY_OP_BOOLEAN(>); break;
            case op_ge: BINARY_OP_BOOLEAN(>=); break;
            case op_lt: BINARY_OP_BOOLEAN(<); break;
            case op_le: BINARY_OP_BOOLEAN(<=); break;
            case op_quote: POKE(0, value_stringify(PEEK(0))); break;
            case op_pop: POP(); break;
            case op_define_global: hamt_set(&chunk.vm->globals, CONSTANT(), POP()); break;
            case op_print:
                value_print(POP());
                puts("");
                break;
            case op_nop: break;
        }
#ifdef DEBUG
        fprintf(stderr, "%s {", opcodes[opcode]);
        for (Value* sp = vm->stack; sp != vm->sp; ++sp) {
            fputc(' ', stderr);
            value_printf(stderr, *sp);
        }
        fprintf(stderr, " }\n");
#endif
    } while (opcode != op_return);

#ifdef DEBUG
    fprintf(stderr, "^^^ ");
    value_printf(stderr, *vm->stack);
    fputs("\n", stderr);
#endif

    chunk_free(&chunk);
    return result_ok;

#undef BYTE
#undef PUSH
#undef POP
#undef POKE
#undef PEEK
#undef BINARY_OP
}

void vm_free(VM* vm) {
#ifdef DEBUG
    hamt_debug(&vm->strings);
    hamt_debug(&vm->globals);
#endif

    hamt_free(&vm->strings);
    hamt_free(&vm->globals);
    for (size_t i = 0; i < vm->objects.count; ++i) {
        value_free_object(vm->objects.items[i]);
    }
    value_array_free(&vm->objects);
}
