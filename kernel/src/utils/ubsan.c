#include <utils/panic.h>

__attribute__((no_sanitize("undefined")))
void __ubsan_handle_out_of_bounds() {
    panic("UBSAN: Out of bounds");
}

__attribute__((no_sanitize("undefined")))
void __ubsan_handle_pointer_overflow() {
    panic("UBSAN: Pointer overflow");
}

__attribute__((no_sanitize("undefined")))
void __ubsan_handle_load_invalid_value() {
    panic("UBSAN: Load invalid value");
}

__attribute__((no_sanitize("undefined")))
void __ubsan_handle_divrem_overflow() {
    panic("UBSAN: Divide overflow");
}

__attribute__((no_sanitize("undefined")))
void __ubsan_handle_add_overflow() {
    panic("UBSAN: Add overflow");
}

__attribute__((no_sanitize("undefined")))
void __ubsan_handle_type_mismatch_v1() {
    panic("UBSAN: Type missmatch");
}

__attribute__((no_sanitize("undefined")))
void __ubsan_handle_shift_out_of_bounds() {
    panic("UBSAN: Shift out of bounds");
}

__attribute__((no_sanitize("undefined")))
void __ubsan_handle_builtin_unreachable() {
    panic("UBSAN: Reached unreachable code");
}

__attribute__((no_sanitize("undefined")))
void __ubsan_handle_sub_overflow() {
    panic("UBSAN: Subtraction overflow");
}

__attribute__((no_sanitize("undefined")))
void __ubsan_handle_mul_overflow() {
    panic("UBSAN: Multiply overflow");
}

__attribute__((no_sanitize("undefined")))
void __ubsan_handle_function_type_mismatch() {
    panic("UBSAN: Function type missmatch");
}

__attribute__((no_sanitize("undefined")))
void __ubsan_handle_negate_overflow() {
    panic("UBSAN: Negate overflow");
}
