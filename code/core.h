#ifndef _H_BASE_
#define _H_BASE_

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

typedef uint8_t  b8;
typedef uint32_t b32;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

#define file_local static
#define function_persist static

#define false 0
#define true 1

#define defer_loop(begin, end) for(int _i_ = ((begin), 0); !_i_; _i_ += 1, (end))
#define array_size(arr) ((sizeof(arr) / sizeof(*(arr))))
#define debug_break() __builtin_debugtrap()

#define S(str) (String){.value = str, .length = (sizeof(str) - 1) }
#define str_expand(str) (i32)str.length, str.value

#define supress_unused(thing) (((void)(thing)))

#define pf_log(Fmt, ...) fprintf(stdout, Fmt, ##__VA_ARGS__)
#define pf_assert(condition) \
(!(condition) ? (fprintf(stdout, "%s:%d: ASSERT [%s] at %s()\n", __FILE__, __LINE__, #condition, __func__), abort()) : false)

// TODO(erb): add proper f32 max value
#define MAX_f32 10000000000.0f

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define abs(x) (x < 0 ? -x : x)
#define cap_bottom(value, cap) max(value, cap)
#define cap_top(value, cap) min(value, cap)
#define cap(value, b, t) cap_top(cap_bottom(value, b), t)
#define cap01(value) cap(value, 0, 1)

#define kb(X) (X*1024)
#define mb(X) (kb(X)*1024)
#define gb(X) ((u64)mb(X)*1024)

#define swap(a, b, Type) \
do { Type swap_temp = a; a = b; b = swap_temp; } while (0)

typedef struct Date
{
	u32 seconds;  // 0-60
	u32 minutes;  // 0-59
	u32 hours;    // 0-23
	u32 month_day; // 1-31
	u32 month;    // 0-11
	u32 year;     // 0-year
	u32 week_day;  // 0-6, Sunday = 0
	u32 year_day;  // 0-365
} Date; 
#define date_expand(date) date.month_day, date.month, date.year, date.hours, date.minutes, date.seconds

typedef struct High_Freq_Time
{
  u64 ns;
  f64 ms;
  f64 s;
} High_Freq_Time;

typedef enum Axis2
{
  Axis2_X,
  Axis2_Y,
} Axis2;

typedef struct Read_File_result
{
	void *data;
	u64 size;
} Read_File_result;

typedef struct String
{
	char *value;
	u64 length;
} String;

typedef struct Index_u64
{
	b32 exists;
	u64 value;
} Index_u64;

typedef struct Index_u32
{
  b32 exists;
  u32 value;
} Index_u32;

typedef struct String_Builder
{
	String buffer;
	u64 capacity;
} String_Builder;

typedef struct Arena
{
	u64 size;
	u64 used;
	void *base;
} Arena;

typedef struct Scratch
{
  Arena *arena;
  u64 position;
} Scratch;

typedef struct Scratch_Memory
{
  Arena arenas[2];
  u32 counter;
} Scratch_Memory;

typedef union V2f
{
	struct { f32 x, y; };
	f32 c[2];
} V2f;

typedef union V2i
{
	struct { i32 x, y; };
  i32 c[2];
} V2i;
#define v2_expand(v) v.x, v.y

typedef union V3f
{
	struct { f32 x, y, z; };
  struct { V2f xy; f32 _pad_z; };
  struct { V2f _pad_x; f32 yz; };
	f32 c[3];
} V3f;
#define v3_expand(v) v.x, v.y, v.z

typedef union V4f
{
	struct { f32 x, y, z, w; };
  struct { V2f xy, zw; };
	struct { V3f xyz; f32 _pad_w; };
  
	struct { f32 r, g, b, a; };
  struct { V3f rgb; f32 _pad_a; };
	f32 c[4];
} V4f;
#define v4_expand(v) v.x, v.y, v.z, v.w

typedef union Screen_Rect_f32
{
	struct { V2f top_left, bottom_right; };
	struct { f32 left, top, right, bottom; };
  f32 c[4];
} Screen_Rect_f32;

typedef union Screen_Rect_i32
{
	struct { V2i top_left, bottom_right; };
	struct { i32 left, top, right, bottom; };
  i32 c[4];
} Screen_Rect_i32;

typedef union Range_V2f
{
	struct { V2f start, end; };
	struct { V2f min, max; };
  V2f c[2];
} Range_V2f;

#define color(R, G, B, A) (V4f){ .r = R, .g = G, .b = B, .a = A }
V4f color_white = color(1, 1, 1, 1);
V4f color_black = color(0, 0, 0, 1);
V4f color_red = color(1, 0, 0, 1);
V4f color_green = color(0, 1, 0, 1);
V4f color_yellow = color(1, 1, 0, 1);
V4f color_cyan = color(0, 1, 1, 1);
V4f color_blue = color(0, 0, 1, 1);
V4f color_magenta = color(1, 0, 1, 1);
V4f color_debug = color(1, 0, 1, 1);
#undef color

#define push_struct(arena, Type) ((Type *)arena_push_size((arena), sizeof(Type)))
#define push_array(arena, count, Type) ((Type *)arena_push_size((arena), sizeof(Type)*(count)))

#define sll_push_allocate(arena, list, Type) \
((!(list)->first) \
? ((list)->first = (list)->last = push_struct((arena), Type)) \
: ((list)->last = (list)->last->next = push_struct((arena), Type)))

#define copy_struct(src, dest) mem_copy((u8 *)src, (u8 *)dest, sizeof(*src))

#define zero_struct(stru) mem_zero(stru, sizeof(*stru))

#endif // _H_BASE_
