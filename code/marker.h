#ifndef _H_MARKER_
#define _H_MARKER_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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

file_local I32 _global_defer_index_ = 0;

#define defer_loop(begin, end) for(_global_defer_index_ = ((begin), 0); !_global_defer_index_; _global_defer_index_ += 1, (end))
#define array_size(arr) ((sizeof(arr) / sizeof(*(arr))))
#define debug_break() __builtin_debugtrap()

#define S(str) (String){.value = str, .length = (sizeof(str) - 1) }
#define str_expand(str) (I32)str.length, str.value

#define supress_unused(thing) (((void)(thing)))
#define assert(condition) if (!(condition)) {*(int *)0 = 0;}

// TODO(erb): add proper F32 max value
#define MAX_F32 10000000000.0f

#undef max
#undef min

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

#define swap(a, b, Type) \
do { Type swap_temp = a; a = b; b = swap_temp; } while (0)

typedef struct Date
{
	U32 seconds;  // 0-60
	U32 minutes;  // 0-59
	U32 hours;    // 0-23
	U32 month_day; // 1-31
	U32 month;    // 0-11
	U32 year;     // 0-year
	U32 week_day;  // 0-6, Sunday = 0
	U32 year_day;  // 0-365
} Date; 
#define date_expand(date) date.month_day, date.month, date.year, date.hours, date.minutes, date.seconds

typedef struct High_Freq_Time
{
  U64 ns;
  F64 ms;
  F64 s;
} High_Freq_Time;

typedef enum Axis2
{
  Axis2_X,
  Axis2_Y,
} Axis2;

typedef struct Read_File_result
{
	void *data;
	U64 size;
} Read_File_result;

typedef struct String
{
	char *value;
	U64 length;
} String;

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

typedef struct String_Builder
{
	String buffer;
	U64 capacity;
} String_Builder;

typedef struct Arena
{
	U64 size;
	U64 used;
	void *base;
} Arena;

typedef struct Scratch
{
  Arena *arena;
  U64 position;
} Scratch;

typedef struct Scratch_Memory
{
  Arena arenas[2];
  U32 counter;
} Scratch_Memory;

typedef union V2f
{
	struct { F32 x, y; };
	F32 c[2];
} V2f;

typedef union V2i
{
	struct { I32 x, y; };
  I32 c[2];
} V2i;
#define v2_expand(v) v.x, v.y

typedef union V3f
{
	struct { F32 x, y, z; };
  struct { V2f xy; F32 _pad_z; };
  struct { V2f _pad_x; F32 yz; };
	F32 c[3];
} V3f;
#define v3_expand(v) v.x, v.y, v.z

typedef union V4f
{
	struct { F32 x, y, z, w; };
  struct { V2f xy, zw; };
	struct { V3f xyz; F32 _pad_w; };
  
	struct { F32 r, g, b, a; };
  struct { V3f rgb; F32 _pad_a; };
	F32 c[4];
} V4f;
#define v4_expand(v) v.x, v.y, v.z, v.w

typedef union Screen_Rect_F32
{
	struct { V2f top_left, bottom_right; };
	struct { F32 left, top, right, bottom; };
  F32 c[4];
} Screen_Rect_F32;

typedef union Screen_Rect_I32
{
	struct { V2i top_left, bottom_right; };
	struct { I32 left, top, right, bottom; };
  I32 c[4];
} Screen_Rect_I32;

typedef union Range_V2f
{
	struct { V2f start, end; };
	struct { V2f min, max; };
  V2f c[2];
} Range_V2f;

V4f color_white = {1, 1, 1, 1};
V4f color_black = {0, 0, 0, 1};
V4f color_red = {1, 0, 0, 1};
V4f color_green = {0, 1, 0, 1};
V4f color_yellow = {1, 1, 0, 1};
V4f color_cyan = {0, 1, 1, 1};
V4f color_blue = {0, 0, 1, 1};
V4f color_magenta = {1, 0, 1, 1};
V4f color_debug = {1, 0, 1, 1};

#define push_struct(arena, Type) ((Type *)arena_push_size((arena), sizeof(Type)))
#define push_array(arena, count, Type) ((Type *)arena_push_size((arena), sizeof(Type)*(count)))

#define sll_push_allocate(arena, list, Type) \
((!(list)->first) \
? ((list)->first = (list)->last = push_struct((arena), Type)) \
: ((list)->last = (list)->last->next = push_struct((arena), Type)))

#define copy_struct(src, dest) mem_copy((U8 *)src, (U8 *)dest, sizeof(*src))

#define zero_struct(stru) mem_zero(stru, sizeof(*stru))

#endif _H_MARKER_

