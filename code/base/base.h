/* date = April 24th 2026 0:50 pm */

#ifndef BASE_INC_H
#define BASE_INC_H

#undef min
#undef max

#include <stdint.h>
#include <stdarg.h>

typedef uint8_t U8;
typedef int8_t I8;

typedef uint8_t U16;
typedef int8_t I16;

typedef uint32_t U32;
typedef int32_t I32;

typedef uint64_t U64;
typedef int64_t I64;

typedef uint32_t B8;
typedef uint32_t B16;
typedef uint32_t B32;

typedef float F32;
typedef double F64;

#define global static
#define file_local static
#define function_persist static

#define false 0
#define true 1

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define abs(x) (x < 0 ? -x : x)

#define cap_bottom(value, cap) max(value, cap)
#define cap_top(value, cap) min(value, cap)
#define cap(value, b, t) cap_top(cap_bottom(value, b), t)
#define cap0(value) cap_bottom(value, 0)
#define cap01(value) cap(value, 0, 1)

#define kb(X) (X*1024)
#define mb(X) (kb(X)*1024)
#define gb(X) ((U64)mb(X)*1024)

#define array_size(arr) ((sizeof(arr) / sizeof(*(arr))))
#define debug_break() __builtin_debugtrap()
#define assert(condition) if (!(condition)) *(int *)0 = 0
#define supress_unused(thing) (((void)(thing)))

file_local I32 _global_defer_index_ = 0;
#define defer_loop(begin, end) for(_global_defer_index_ = ((begin), 0); !_global_defer_index_; _global_defer_index_ += 1, (end))

typedef struct Read_File_Result
{
  void *memory;
  U32 size;
} Read_File_Result;

typedef enum Axis2
{
  Axis2_X,
  Axis2_Y,
} Axis2;

typedef struct Index_U64
{
	B32 exists;
	U64 value;
} Index_U64;

typedef struct Index_U32
{
  B32 exists;
  U32 value;
} Index_U32;


// ///////////////////////////////////////////
//- includes
#include "base_numeric.h"
#include "base_memory.h"
#include "base_string.h"

#endif //BASE_INC_H
