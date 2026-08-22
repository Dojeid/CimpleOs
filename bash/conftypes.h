#ifndef _CONFTYPES_H_
#define _CONFTYPES_H_
#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>
typedef uint16_t bits16_t;
typedef uint32_t bits32_t;
typedef uint64_t bits64_t;
typedef uint32_t u_bits32_t;
#define GETGROUPS_T gid_t
#define BASH_SOURCE_FULLPATH_DEFAULT 0
#define EXTGLOB_DEFAULT 1
#define GLOBASCII_DEFAULT 0
#endif
