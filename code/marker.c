#include "marker.h"

// //////////////////////////////////////////////
// NOTE(erb): bytes
// //////////////////////////////////////////////

U8 *mem_copy(U8 *source, U8 *dest, U64 count)
{
	while (count-- > 0)
	{
		*dest++ = *source++;
	}
	return dest;
}

void mem_set(U8 *bytes, U64 count, U8 value)
{
	while (count-- > 0)
	{
		*bytes++ = value;
	}
}

void mem_zero(U8 *bytes, U64 count)
{
	mem_set(bytes, count, 0);
}

// //////////////////////////////////////////////
// NOTE(erb): arena
// //////////////////////////////////////////////

Arena
arena_make(void *memory, U64 size)
{
  Arena arena = {0};
	arena.size = size;
	arena.base = memory;
	arena.used = 0;
  mem_set(memory, size, 0xDD);
  return arena;
}

void arena_init(Arena *arena, void *memory, U64 size)
{
	arena->size = size;
	arena->base = memory;
	arena->used = 0;
  mem_set(memory, size, 0xDD);
}

void arena_release(Arena *arena)
{
	arena->used = 0;
}

void *arena_push_size(Arena *arena, U64 size) 
{
  assert(arena->used + size < arena->size);
  void *result = (U8 *)arena->base + arena->used;
  mem_zero(result, size);
  arena->used += size;
  
  return (void *)result;
}

Arena push_sub_arena(Arena *parent, U64 size)
{
  Arena result = {0};
  void *memory = arena_push_size(parent, size);
  arena_init(&result, memory, size);
  return result;
}

Scratch scratch_begin(Arena *arena)
{
  Scratch result = {0};
  result.arena = arena;
  result.position = arena->used;
  arena_push_size(arena, kb(4));
  return result;
}

void scratch_end(Scratch *scratch)
{
  scratch->arena->used = scratch->position;
  scratch->arena = 0;
  scratch->position = 0;
}

void mem_scratch_init(Arena *arena, Scratch_Memory *mem)
{
  for (U32 arena_idx = 0;
       arena_idx < array_size(mem->arenas);
       arena_idx++)
  {
    mem->arenas[arena_idx] = push_sub_arena(arena, kb(8));
  }
}

Scratch mem_scratch_begin(Scratch_Memory *mem)
{
  Arena *scratch_base = mem->arenas + (mem->counter++ % array_size(mem->arenas));
  Scratch result = scratch_begin(scratch_base);
  return result;
}

void mem_scratch_end(Scratch_Memory *mem, Scratch *scratch)
{
  Arena *scratch_base = mem->arenas + (--mem->counter % array_size(mem->arenas));
  assert(scratch_base == scratch->arena);
  scratch_end(scratch);
  mem->counter--;
}

// //////////////////////////////////////////////
// NOTE(erb): hash
// //////////////////////////////////////////////

U64 hash_string(String str)
{
	U64 hash = 0;
	I32 p = 31;
  I32 m = (I32)(1e9 + 9);
	U64 ppow = 1;
	
	for (U32 char_idx = 0; 
       char_idx < str.length;
       char_idx ++)
	{
		U8 c = str.value[char_idx];
		hash = (hash + (c - 'a' + 1) * ppow) % m;
		ppow = (ppow * p) % m;
	}
	return hash;
}

// //////////////////////////////////////////////
// NOTE(erb): char helpers
// //////////////////////////////////////////////

B32 is_digit(char c)
{
	B32 result = (c >= '0' && c <= '9');
	return result;
}

B32 is_math_op(char c)
{
  B32 result = (c == '+' || c == '-' || 
                c == '/' || c == '*');
  return result;
}

B32 is_blank(char c)
{
	B32 result = (c == ' ' || c == '\t' || c == '\n');
	return result;
}

B32 is_lower_letter(char ch)
{
	B32 result = (ch >= 'a' && ch <= 'z');
	return result;
}

B32 is_upper_letter(char Ch)
{
	B32 result = (Ch >= 'A' && Ch <= 'Z');
	return result;
}

B32 is_letter(char Ch)
{
	B32 result = is_lower_letter(Ch) || is_upper_letter(Ch);
	return result;
}

// //////////////////////////////////////////////
// NOTE(erb): string
// //////////////////////////////////////////////

char *cstr_trim_start(char *str)
{
	while (*str && (*str == ' ' || *str == '\n' || *str == '\t'))
	{
		str++;
	}
	return str;
}

B32 cstr_equals(char *a, char *b)
{
	B32 result = true;
	while (result && *a && *b)
	{
		result = (*a++ == *b++);
	}
  
	return result;
}

U64 cstr_length(char *str)
{
	char *str_it = str;
	while (*str_it++);
  
	U64 result = cap_bottom((U64)(str_it - str) - 1, 0);
	return result;
}

B32 cstr_to_U64(char *str, U64 *value)
{
	U64 max_value = ~(U64)0;
	*value = 0;
  
	while (*str 
         && is_digit(*str) 
         && *value < max_value)
	{
		*value = (*value)*10 + ((*str) - '0');
		str++;
	}
	
	B32 result = (*str == 0);
	return result;
}

B32 cstr_to_U32(char *str, U32 *value)
{
	U32 max_value = ~(U32)0;
	*value = 0;
  
	while (*str 
         && is_digit(*str) 
         && *value < max_value)
	{
		*value = (*value)*10 + ((*str) - '0');
		str++;
	}
	B32 result = (*str == 0);
	return result;
}

String str_null()
{
	String result = {0};
	return result;
}

String str_copy(Arena *arena, String source)
{
  String str = {0};
  
	str.value = push_array(arena, source.length, char);
	str.length = source.length;
	mem_copy((U8 *)source.value, (U8 *)str.value, source.length);
  
	return str;
}


B32 str_equals(String str_a, String str_b)
{
	B32 result = false;
	
	if (str_a.length == str_b.length)
	{
		result = true;
		
		for (U64 Index = 0;
         Index < str_a.length && (result);
         Index++)
		{
			result = (str_a.value[Index] == str_b.value[Index]);
		}
	}
  
	return result;
}

String str_from(String str, U64 from_idx)
{
  String slice = {0};
	
	if (from_idx <= str.length)
	{
		slice.value = (str.value + from_idx);
		slice.length = str.length - from_idx;
	}
	
	return slice;
}

String str_clip(String str, U64 to_idx)
{
	String slice = {0};
	
	if (to_idx >= 0 && to_idx <= str.length)
	{
		slice.value = str.value;
		slice.length = to_idx;
	}
	
	return slice;
}

String str_slice(String str, U64 from_idx, U64 to_idx)
{
	String slice = {0};
	String to = str_clip(str, to_idx);
	if (to.length > 0)
	{
		slice = str_from(to, from_idx);
	}
	
	return slice;
}

String str_copy_buffer_length(Arena *arena, char *buffer, U64 length)
{
  String str = {0};
  str.value = push_array(arena, length, char);
  mem_copy((U8 *)buffer, (U8 *)str.value, length);
  str.length = length;
  return str;
}

String str_concat(Arena *arena, String str_a, String str_b)
{
  String result = {0};
  result.length = str_a.length + str_b.length;
  result.value = push_array(arena, result.length, char);
  
  mem_copy((U8 *)str_a.value, (U8 *)result.value, str_a.length);
  mem_copy((U8 *)str_b.value, (U8 *)(result.value + str_a.length), str_b.length);
  return result;
}

Index_U64 str_index_char(String str, char ch)
{
	Index_U64 result = {0};
	for (U64 ch_idx = 0;
       ch_idx < str.length && !result.exists;
       ch_idx++)
	{
		if (ch == str.value[ch_idx])
		{
			result.value = ch_idx;
			result.exists = true;
		}
	}
  
	return result;
}

Index_U64 str_index_char_backward(String str, char ch)
{
	Index_U64 result = {0};
	for (U64 ch_idx = str.length - 1;
       ch_idx >= 0 && !result.exists;
       ch_idx--)
	{
		if (ch == str.value[ch_idx])
		{
			result.value = ch_idx;
			result.exists = true;
		}
	}
  
	return result;
}

Index_U64 str_index_blank(String str)
{
	Index_U64 result = {0};
	for (U64 ch_idx = 0;
       ch_idx < str.length && !result.exists;
       ch_idx++)
	{
		if (is_blank(str.value[ch_idx]))
		{
			result.value = ch_idx;
			result.exists = true;
		}
	}
  
	return result;
}

Index_U64 str_index_not_blank(String str)
{
	Index_U64 result = {0};
	for (U64 ch_index = 0;
       ch_index < str.length && !result.exists;
       ch_index++)
	{
		if (!is_blank(str.value[ch_index]))
		{
			result.value = ch_index;
			result.exists = true;
		}
	}
  
	return result;
}

Index_U64 str_index_blank_backward(String str)
{
	Index_U64 result = {0};
	for (I64 ch_index = str.length - 1;
       ch_index >= 0 && !result.exists;
       ch_index--)
	{
		if (is_blank(str.value[ch_index]))
		{
			result.value = ch_index;
			result.exists = true;
		}
	}
  
	return result;
}

Index_U64 str_index_not_blank_backward(String str)
{
	Index_U64 result = {0};
	for (I64 ch_idx = str.length - 1;
       ch_idx >= 0 && !result.exists;
       ch_idx--)
	{
		if (!is_blank(str.value[ch_idx]))
		{
			result.value = ch_idx;
			result.exists = true;
		}
	}
  
	return result;
}

Index_U64 str_index_letter(String str)
{
	Index_U64 result = {0};
	for (U64 ch_idx = 0;
       ch_idx < str.length && !result.exists;
       ch_idx++)
	{
		if (is_letter(str.value[ch_idx]))
		{
			result.value = ch_idx;
			result.exists = true;
		}
	}
  
	return result;
}

Index_U64 str_index_digit(String str)
{
	Index_U64 result = {0};
	for (U64 ch_idx = 0;
       ch_idx < str.length && !result.exists;
       ch_idx++)
	{
		if (is_digit(str.value[ch_idx]))
		{
			result.value = ch_idx;
			result.exists = true;
		}
	}
  
	return result;
}

Index_U64 str_index_math_op(String str)
{
	Index_U64 result = {0};
	for (U64 ch_idx = 0;
       ch_idx < str.length && !result.exists;
       ch_idx++)
	{
		if (is_math_op(str.value[ch_idx]))
		{
			result.value = ch_idx;
			result.exists = true;
		}
	}
  
	return result;
}

String str_next_word(String str)
{
	String word = {0};
  
	Index_U64 word_begin_idx = str_index_not_blank(str);
	if (word_begin_idx.exists)
	{
		Index_U64 word_end_idx = str_index_blank(str_from(str, word_begin_idx.value+1));
		if (word_end_idx.exists)
		{
			word_end_idx.value++;
		}
		else
		{
			word_end_idx.value = str.length;
		}
    
		word = str_slice(str, word_begin_idx.value, word_end_idx.value);
	}
	
	return word;
}


Index_U64 str_index_word_end_backward(String str)
{
  Index_U64 result = {0};
  
	Index_U64 word_begin_idx = str_index_not_blank_backward(str);
	if (word_begin_idx.exists)
	{
    Index_U64 word_end_idx = str_index_blank_backward(str_clip(str, word_begin_idx.value));
		if (word_end_idx.exists)
		{
      result.value = word_end_idx.value+1;
      result.exists = true;
		}
    else
    {
      result.value = 0;
    }
	}
  
	return result;
}

Index_U64 str_index_word_end(String str)
{
  Index_U64 result = {0};
  
	Index_U64 word_begin_idx = str_index_not_blank(str);
	if (word_begin_idx.exists)
	{
    Index_U64 word_end_idx = str_index_blank(str_from(str, word_begin_idx.value));
		if (word_end_idx.exists)
		{
      result.value = word_end_idx.value + word_begin_idx.value;
      result.exists = true;
		}
    else
    {
      result.value = str.length;
    }
	}
  
	return result;
}

Index_U64 str_index_word_end_delta(String str, U32 start, I32 delta)
{
  Index_U64 result = {0};
  result.value = start;
  
  for (U32 scan_count = abs(delta);
       scan_count > 0; 
       scan_count--) 
  {
    Index_U64 scan = {0};
    if (delta > 0) 
    {
      scan = str_index_word_end(str_from(str, result.value));
      result.value += scan.value;
    }
    else
    {
      scan = str_index_word_end_backward(str_clip(str, result.value));
      result.value = scan.value;
    }
    
    if (scan.exists != 0) 
    {
      break;
    }
  }
  
  result.exists = true;
	return result;
}

String str_pop_dir(String str, char dir_sep)
{
  String result = str;
  Index_U64 dir_sep_idx = str_index_char_backward(str, dir_sep);
  if (dir_sep_idx.exists) 
  {
    result = str_clip(str, dir_sep_idx.value);
  }
  return result;
}

B32 str_to_U64(String Str, U64 *value)
{
	U64 maxvalue = ~(U64)0;
	*value = 0;
  
	char *base = Str.value;
	for (U64 Index = 0; 
       Index < Str.length
       && is_digit(*base) 
       && *value < maxvalue;
       base++, Index++)
	{
		*value = (*value)*10 + ((*base) - '0');
	}
	
	B32 result = (base == Str.value+Str.length);
	return result;
}

Index_U64 str_try_to_U64(String str, U64 *value)
{
	U64 max_value = ~(U64)0;
	*value = 0;
  
	Index_U64 result = {0};
	Index_U64 digit_idx = str_index_digit(str);
	
	if (digit_idx.exists)
	{
		result.value = digit_idx.value;
		
		while (result.value < str.length 
           && *value < max_value)
		{
			char ch = str.value[result.value];
			if (is_digit(ch))
			{
				*value = (*value)*10 + (ch - '0');
				result.value++;
			}
			else
			{
				break;
			}
		}
	}
  
	result.exists = (result.value > digit_idx.value);
	return result;
}

char *str_to_temp256_cstr(String str)
{
	assert(str.length + 1 < 256);
	static char temp_buffer[256];
  
	mem_copy((U8 *)str.value, (U8 *)temp_buffer, str.length);
	temp_buffer[str.length] = 0;
  
	return temp_buffer;
}

char *str_to_temp512_cstr(String str)
{
	assert(str.length + 1 < 512);
	static char temp_buffer[512];
  
	mem_copy((U8 *)str.value, (U8 *)temp_buffer, str.length);
	temp_buffer[str.length] = 0;
  
	return temp_buffer;
}

char *push_cstr(Arena *arena, String str)
{
  char *result = push_array(arena, str.length+1, char);
  mem_copy((U8 *)str.value, (U8 *)result, str.length);
  result[str.length] = 0;
  return result;
}

String push_cstr_copy(Arena *arena, char *cstr)
{
	U64 len = cstr_length(cstr);
  
	char *dest = push_array(arena, len, char);
	mem_copy((U8 *)cstr, (U8 *)dest, len);
  
	String result = {0};
	result.value = dest;
	result.length = len;
	return result;
}

B32 str_builder_grow_maybe(Arena *arena, String_Builder *builder, U64 size)
{
	U64 total_size = builder->buffer.length + size;
	B32 result = total_size > builder->capacity;
	
	if (result)
	{
		total_size = max(total_size*2, 256);
		
		char *new_mem = push_array(arena, total_size, char);
		mem_copy((U8 *)builder->buffer.value, (U8 *)new_mem, builder->buffer.length);
		
		builder->buffer.value = new_mem;
		builder->capacity = total_size;
	}
  
	return result;
}

String str_build_cstr(Arena *arena, String_Builder *builder, char *cstr)
{
	U64 len = cstr_length(cstr);
	str_builder_grow_maybe(arena, builder, len);
  
	char *dest = (builder->buffer.value + builder->buffer.length);
	mem_copy((U8 *)cstr, (U8 *)dest, len);
	builder->buffer.length += len;
  
	String result = {0};
	result.value = dest;
	result.length = len;
	return result;
}


String str_build_char(Arena *arena, String_Builder *builder, char ch)
{
	str_builder_grow_maybe(arena, builder, 1);
  
	char *dest = (builder->buffer.value + builder->buffer.length);
	*dest = ch;
	builder->buffer.length++;
  
	String result = {0};
	result.value = dest;
	result.length = 1;
	return result;	
}

String str_build_str(Arena *arena, String_Builder *builder, String str)
{
	str_builder_grow_maybe(arena, builder, str.length);
  
	char *dest = (builder->buffer.value + builder->buffer.length);
	mem_copy((U8 *)str.value, (U8 *)dest, str.length);
	builder->buffer.length += str.length;
  
	return str;
}

typedef union Range_U64 
{
  struct { U64 begin, end; };
  struct { U64 min, max; };
} Range_U64;

U64 range_U64_size(Range_U64 range)
{
  U64 size = range.max - range.min;
  return size;
}

Range_U64 range_U64_from_size(U64 size)
{
  Range_U64 result = {0};
  result.max = size;
  return result;
}

B32 range_U64_inclusive(Range_U64 range, U64 value)
{
  B32 result = (range.min <= value && 
                value <= range.max);
  return result;
}

B32 range_U64_inclusive_range(Range_U64 in, Range_U64 container)
{
  B32 result = (range_U64_inclusive(container, in.min) &&
                range_U64_inclusive(container, in.max));
  return result;
}

void str_build_replace(Arena *arena, 
                       String_Builder *builder, 
                       String replace_str, 
                       Range_U64 replace_range)
{
  if (range_U64_inclusive_range(replace_range, range_U64_from_size(builder->buffer.length))) 
  {
    I32 diff = (I32)replace_str.length - (I32)range_U64_size(replace_range);
    if (diff > 0) 
    {
      str_builder_grow_maybe(arena, builder, diff);
    }
    
    Scratch scratch = scratch_begin(arena);
    {
      String post_replace = str_copy(scratch.arena, str_from(builder->buffer, replace_range.max));
      U8 *replace_dest = (U8 *)(builder->buffer.value + replace_range.min);
      U8 *advance = replace_dest + replace_str.length;
      
      mem_copy((U8 *)post_replace.value, advance, post_replace.length);
      mem_copy((U8 *)replace_str.value, replace_dest, replace_str.length);
      
      builder->buffer.length += diff;
    }
    scratch_end(&scratch);
  }
}

String join_cargs(Arena *arena, I32 arg_count, char **args)
{
	String_Builder builder = {0};
	
	for (I32 arg_idx = 0;
       arg_idx < arg_count;
       arg_idx++)
	{
		str_build_cstr(arena, &builder, args[arg_idx]);
		
		if (arg_idx < arg_count - 1)
		{
			str_build_char(arena, &builder, ' ');
		}
	}
  
	return builder.buffer;
}

String str_format(Arena *arena, const char *format, ...)
{
  // TODO(erb): switch to real thread safe scratch arena here
  char buffer[1024];
  va_list args;
  va_start(args, format);
  U64 size = vsnprintf(buffer, array_size(buffer), format, args);
  va_end(args);
  
  String result = {0};
  result.value = push_array(arena, size, char);
  result.length = size;
  mem_copy((U8 *)buffer, (U8 *)result.value, size);
  return result;
}

// //////////////////////////////////////////////
// NOTE(erb): Date
// //////////////////////////////////////////////

B32 date_equals(Date a, Date b)
{
	B32 result = a.seconds == b.seconds
		&& a.minutes   == b.minutes
		&& a.hours     == b.hours
		&& a.month_day == b.month_day
		&& a.month     == b.month
		&& a.year      == b.year
		&& a.week_day  == b.week_day
		&& a.year_day  == b.year_day;
	return result;
}

Date add_day(Date date, U32 days)
{
	// TODO(enrique): Polish this to handle months with 31 days
	date.month_day = (date.month_day + days) % 31;
	date.month     = (date.month     + (days/31)) % 13 + 1;
	date.year      = (date.year      + (days/366));
	date.year_day  = (date.year_day  + days) % 366;
	date.week_day  = (date.week_day  + days) % 7;
	return date;
}

// //////////////////////////////////////////////
// NOTE(erb): math
// //////////////////////////////////////////////

F32 lerp(F32 min, F32 max, F32 t)
{
  F32 result = min + (max - min)*t;
  return result;
}

V2f v2f(F32 x, F32 y)
{
	V2f result = {0};
	result.x = x;
	result.y = y;
	return result;
}

V2f v2ff(F32 v)
{
	V2f result = {0};
	result.x = result.y = v;
	return result;
}

V2f v2fi(V2i v)
{
	V2f result = {0};
  result.x = (F32)v.x;
	result.y = (F32)v.y;
	return result;
}

V2f v2f_add(V2f a, V2f b)
{
	V2f result = {0};
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	return result;
}

V2f v2f_sub(V2f a, V2f b)
{
	V2f result = {0};
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return result;
}

V2f v2f_hada(V2f a, V2f b)
{
	V2f result = {0};
	result.x = a.x * b.x;
	result.y = a.y * b.y;
	return result;
}

V2f v2f_hada_div(V2f a, V2f b)
{
	V2f result = {0};
	result.x = a.x / b.x;
	result.y = a.y / b.y;
	return result;
}

V2f v2f_scale(V2f vec, F32 scalar) 
{
	V2f result = {0};
	result.x = vec.x * scalar;
	result.y = vec.y * scalar;
	return result;
}

V2f v2f_abs(V2f v) 
{
  V2f result = {0};
	result.x = abs(v.x);
	result.y = abs(v.y);
  return result;
}

V2f v2f_lerp(V2f min, V2f max, F32 t) 
{
  V2f result = {0};
	result.x = lerp(min.x, max.x, t);
	result.y = lerp(min.y, max.y, t);
  return result;
}

V2f v2f_min(V2f a, V2f b)
{
  V2f result = {0};
	result.x = min(a.x, b.x);
	result.y = min(a.y, b.y);
  return result;
}

V2f v2f_reverse_y(V2f v, F32 y)
{
  V2f result = {0};
  result.x = v.x;
  result.y = y - v.y;
  return result;
}

// NOTE(erb): v2i

V2i v2i(I32 x, I32 y)
{
	V2i result = {0};
	result.x = x;
	result.y = y;
	return result;
}

V2i v2ii(I32 v)
{
	V2i result = {0};
	result.x = result.y = v;
	return result;
}

V2i v2i_add(V2i a, V2i b)
{
	V2i result = {0};
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	return result;
}

V2i v2i_sub(V2i a, V2i b)
{
	V2i result = {0};
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return result;
}

B32 v2i_equals(V2i a, V2i b)
{
  B32 result = (a.x == b.x && a.y == b.y);
  return result;
}

// NOTE(erb): v3f

V3f v3f(F32 x, F32 y, F32 z)
{
	V3f result = {0};
  result.x = x;
  result.y = y;
  result.z = z;
	return result;
}

V3f v3ff(F32 v)
{
	V3f result = {0};
	result.x = result.y = result.z = v;
	return result;
}

// NOTE(erb): v4

V4f v4f(F32 x, F32 y, F32 z, F32 w)
{
	V4f result = {0};
  result.x = x;
  result.y = y;
  result.z = z;
  result.w = w;
	return result;
}

V4f v4ff(F32 v)
{
	V4f result = {0};
  result.x = result.y = result.z = result.w = v;
	
	return result;
}

V4f with_alpha(V4f color, F32 a)
{
  V4f result = color;
  result.a = a;
  return result;
}

V4f v4f_lerp(V4f min, V4f max, F32 t)
{
  V4f result = {0};
  result.x = lerp(min.x, max.x, t);
  result.y = lerp(min.y, max.y, t);
  result.z = lerp(min.z, max.z, t);
  result.w = lerp(min.w, max.w, t);
  return result;
}

// NOTE(erb): rects and ranges
V2f rect_size(Screen_Rect_F32 size);

Range_V2f range_point_add(V2f point, V2f add)
{
  Range_V2f result = {0};
  result.min = point;
  result.max = v2f_add(point, add);
  return result;
}

Range_V2f to_range(Screen_Rect_F32 srect, F32 max_y)
{
  V2f size = rect_size(srect);
  V2f pos = v2f(srect.left, max_y - srect.top - size.y);
  Range_V2f result = range_point_add(pos, size);
  return result;
}

Range_V2f range_from_size(V2f size)
{
  Range_V2f result = {0};
  result.max = size;
  return result;
}

Screen_Rect_F32 rect_from_size(V2f size)
{
  Screen_Rect_F32 result = {0};
  result.bottom_right = size;
  return result;
}

Screen_Rect_F32 rect_top_left(V2f top_left, V2f size)
{
  Screen_Rect_F32 result = {0};
  result.top_left = top_left;
  result.bottom_right = v2f_add(top_left, size);
  return result;
}

Screen_Rect_F32 rect_center(V2f center, V2f size)
{
  V2f top_left = v2f_sub(center, v2f_scale(size, 0.5f));
  Screen_Rect_F32 result = rect_top_left(top_left, size);
  return result;
}

V2f rect_size(Screen_Rect_F32 size)
{
  V2f result = v2f(size.right - size.left, size.bottom - size.top);
  return result;
}
