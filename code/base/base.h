/* date = April 24th 2026 0:50 pm */

#ifndef BASE_INC_H
#define BASE_INC_H

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>

#if defined(_WIN32) || defined(_WIN64)
#define OS_WIN32 1
#elif __linux__
#define OS_LINUX 1
#else
#error "platform not supported yet!"
#endif

#ifdef _MSC_VER
#define COMP_MSVC 1
#elif defined(__GNUC__)
#define COMP_GCC 1
#elif defined (__clang__)
#define COMP_CLANG 1
#endif

#define DEBUG 1 // TODO(erb): this should not be here, should be builder logic

#if defined(DEBUG) && defined(OS_WIN32)
#define RADDBG_MARKUP_IMPLEMENTATION
#else
#define RADDBG_MARKUP_STUBS
#endif //DEBUG
#include "external/raddbg_markup.h"

typedef uint8_t  U8;
typedef int8_t   I8;

typedef uint16_t  U16;
typedef int16_t   I16;

typedef uint32_t U32;
typedef int32_t  I32;

typedef uint64_t U64;
typedef int64_t  I64;

typedef uint32_t B8;
typedef uint32_t B16;
typedef uint32_t B32;

typedef float    F32;
typedef double   F64;

typedef void Void_Func(void);

#define global           static
#define file_local       static
#define function_persist static

#define false 0
#define true  1

#define Max(a, b) ((a) > (b) ? (a) : (b))
#define Min(a, b) ((a) < (b) ? (a) : (b))
#define Abs(x)    ((x) >= 0 ? (x) : -(x))

#define CapBottom(value, cap)  Max(value, cap)
#define CapTop(value, cap)     Min(value, cap)
#define Cap(value, b, t)       CapTop(CapBottom(value, b), t)
#define Cap0(value)            CapBottom(value, 0)
#define Cap1(value)            CapBottom(value, 1)
#define Cap01(value)           Cap(value, 0, 1)

#define KB(X) (X*1024)
#define MB(X) (KB(X)*1024)
#define GB(X) ((U64)MB(X)*1024)

#define Swap(Type, a, b)     do { Type _swap_temp_ = (a); (a) = (b); (b) = _swap_temp_; } while(0)
#define ArraySize(arr)       ((sizeof(arr) / sizeof(*(arr))))
#define SupressUnused(thing) (((void)(thing)))

#if DEBUG

#ifdef RADDBG_MARKUP_STUBS
#define Assert(condition) do { if (!(condition)) *(volatile int *)0 = 0; } while (0)
#else
#define Assert(condition) raddbg_break_if(!(condition))
#endif //RADDBG_MARKUP_STUBS

#define AssertImplies(condition, implies) do { if ((condition)) Assert((implies)); } while (0)

#else
#define Assert(condition) ((void)0)
#define AssertImplies() ((void)0)
#endif //DEBUG

#define InvalidCodePath Assert(false)
#define InvalidDefaultCase default: { InvalidCodePath; } break

#if DEBUG
#define Implement() do { os_logf("%s:%d: [implement] %s\n", __FILE__, __LINE__, __func__); InvalidCodePath; } while (0)
#else
#define Implement() ((void)0)
#endif //DEBUG

file_local I32 _global_defer_index_ = 0;
#define DeferLoop(begin, end) for(_global_defer_index_ = ((begin), 0); !_global_defer_index_; _global_defer_index_ += 1, (end))

#define U32Each(it, count)           U32 it = 0;             (it) < (count);                         (it) += 1
#define U64Each(it, count)           U64 it = 0;             (it) < (count);                         (it) += 1
#define EachIdx(it, array)           U32 it = 0;             (it) < ArraySize((array));              (it) += 1
#define EachElement(Type, it, array) Type *it = (array);     (it) != ((array) + ArraySize((array))); (it) += 1
#define EachNode(Type, it, first)    Type *it = (first);     (it) != 0;                              (it) = (it)->next
#define U32EachInRange(it, range)    U32 it = (range).start; (it) <= (range).end; (it) += 1
#define U64EachInRange(it, range)    U64 it = (range).start; (it) <= (range).end; (it) += 1

#define SLLStackPop(node)                 ((node) = (node)->next)
#define SLLStackPush(first, node)         ((node)->next = (first), (first) = (node))
#define SLLQueueAppend(first, last, node) (((first) != 0) ? ((last)->next = (node), (last) = (node)) : ((first) = (last) = (node)))
#define DLLQueueAppend(first, last, node) (((first) != 0) \
? ((last)->next = (node), (node)->prev = (last), (last) = (node)) \
: ((first) = (last) = (node)))
#define DLLRemove(first, last, node)      (((node)->prev ? ((node)->prev->next = (node)->next) : ((first) = (node)->next)), \
((node)->next ? ((node)->next->prev = (node)->prev) : ((last) = (node)->prev)))

#define ArrayCopy(src, dest, count)        mem_copy((U8 *)(src), (U8 *)(dest), (count)*sizeof(*src))
#define ArraySetValues(ptr, count, value)  do { for (U32Each(set_value_idx, (count))) *((ptr) + set_value_idx) = (value); } while (0)

#if COMP_MSVC
#define printf_format_arg _Printf_format_string_
#define printf_format_function_tag(str_idx, first_idx)
#elif COMP_GCC
#define printf_format_arg 
#define printf_format_function_tag(str_idx, first_idx)  __attribute__((format(printf, str_idx, first_idx)))
#elif COMP_CLANG
#define printf_format_arg 
#define printf_format_function_tag(str_idx, first_idx)  __attribute__((__format__(__printf__, str_idx, first_idx)))
#endif

// ///////////////////////////////////////////
// NOTE(erb): max values

U8 u8_max = 0xFF; //  255
I8 i8_min = 0x80; // -128
I8 i8_max = 0x7F; //  127

U16 u16_max = 0xFFFF; //  65_636
I16 i16_min = 0x8000; // -32_798
I16 i16_max = 0x7FFF; //  32_767

U32 u32_max = 0xFFFFFFFF; //  4_294_967_295
I32 i32_min = 0x80000000; // -2_147_483_648
I32 i32_max = 0x7FFFFFFF; //  2_147_483_647

U64 u64_max = 0xFFFFFFFFFFFFFFFF; // 18_446_744_073_709_551_615
I64 i64_min = 0x8000000000000000; // -9_223_372_036_854_775_808
I64 i64_max = 0x7FFFFFFFFFFFFFFF; //  9_223_372_036_854_775_807

F32 f32_max = (F32)0x7F7FFFFF; // 0x1.fffffep127 | ~3.40282347*10^38
F32 f32_infinity = (F32)0x7F800000;
F32 f32_NaN_min = (F32)0x7F800001;
F32 f32_NaN_max = (F32)0x7FFFFFFF;

F64 f64_max = (F64)0x7FEFFFFFFFFFFFFF; // 0x1.fffffffffffffp1023 | ~1.7977 × 10³⁰⁸
F64 f64_infinity = (F64)0x7FF0000000000000;

// ///////////////////////////////////////////
// NOTE(erb): bit masks
I32 bit_8  = (1 << 7);
I32 bit_16 = (1 << 15);
I32 bit_31 = (1 << 30);
I32 bit_32 = (1 << 31);
I32 bit_mask_2 = 0x03;
I32 bit_mask_3 = 0x07;
I32 bit_mask_4 = 0x0F;
I32 bit_mask_5 = 0x1F;
I32 bit_mask_6 = 0x3F;
I32 bit_mask_16 = ((1 << 16) - 1);

typedef enum Axis2
{
  Axis2_X,
  Axis2_Y,
} Axis2;

typedef enum Orientation2
{
  Orientation2_NS,
  Orientation2_WE,
  Orientation2_NESW,
  Orientation2_NWSE,
} Orientation2;

typedef enum Direction2
{
  Direction2_North,
  Direction2_South,
  Direction2_West,
  Direction2_East,
  Direction2_COUNT,
} Direction2;

typedef struct Index_U64 Index_U64;
struct Index_U64
{
	B32 exists;
	U64 value;
};

typedef struct Index_U32 Index_U32;
struct Index_U32
{
  B32 exists;
  U32 value;
};

// ///////////////////////////////////////////
//- includes
#include "base_numeric.h"
#include "base_memory.h"
#include "base_string.h"
#include "base_time.h"
#include "base_os/os.h"

#endif //BASE_INC_H
