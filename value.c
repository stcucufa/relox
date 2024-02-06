#include <inttypes.h>
#include "value.h"

void value_print(FILE* stream, Value v) {
    if (VALUE_IS_NUMBER(v)) {
        fprintf(stream, "%g", v.as_double);
    } else {
        switch (v.as_int & 7) {
            case 1: fprintf(stream, "nil"); break;
            case 2: fprintf(stream, "false"); break;
            case 3: fprintf(stream, "true"); break;
            default: fprintf(stream, "???");
        }
    }

#ifdef DEBUG
    fprintf(stream, "=0x%" PRIx64, v.as_int);
#endif
}

bool value_equal(Value x, Value y) {
    return x.as_int == y.as_int;
}
