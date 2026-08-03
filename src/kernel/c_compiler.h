#ifndef C_COMPILER_H
#define C_COMPILER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Built-in Native C Compiler & Execution Engine
int c_compile_source(const char* c_source, char* log_buf, size_t log_max);
int c_execute_binary(char* output_buf, size_t out_max);

#ifdef __cplusplus
}
#endif

#endif // C_COMPILER_H
