#include <stdio.h>

U64 
cstr_length(char *str)
{
	char *str_it = str;
	while (*str_it++);
	U64 result = Cap0((U64)(str_it - str) - 1);
	return result;
}

String8
push_str8(Arena *arena, U64 size)
{
  String8 str = S8Struct(PushArray(arena, size, U8), size);
  return(str);
}

String8 
push_str8_copy(Arena *arena, String8 source)
{
  String8 str = push_str8(arena, source.size);
  ArrayCopy(source.value, str.value, source.size);
	return str;
}

String8 
push_str8_from_cstr(Arena *arena, char *cstr)
{
  String8 source = S8CStr(cstr);
  String8 result = push_str8_copy(arena, source);
	return(result);
}

// //////////////////////////////////////////////
// NOTE(erb): char helpers / predicates
// //////////////////////////////////////////////

B32
is_ascii(I32 x)
{
  B32 result = (x >= 0 && x <= 127);
  return(result);
}


B32 
char_is_digit(I32 c)
{
	B32 result = (c >= '0' && c <= '9');
	return(result);
}

B32 
char_is_blank(I32 c)
{
	B32 result = (c == ' ' || c == '\t' || c == '\n');
	return(result);
}

B32 
char_is_not_blank(I32 c)
{
	B32 result = !char_is_blank(c);
	return(result);
}

B32 
char_is_lower_alphabetic(I32 c)
{
	B32 result = (c >= 'a' && c <= 'z');
	return(result);
}

B32 
char_is_upper_alphabetic(I32 c)
{
	B32 result = (c >= 'A' && c <= 'Z');
	return(result);
}

B32 
char_is_alphabetic(I32 c)
{
	B32 result = (char_is_lower_alphabetic(c) || char_is_upper_alphabetic(c));
	return(result);
}

B32
char_is_punctuation(I32 c)
{
  B32 result = ((c >= '!' && c <= '/') ||
                (c >= ':' && c <= '@') ||
                (c >= '[' && c <= '`') ||
                (c >= '{' && c <= '~'));
  return(result);
}

B32
char_is_alphanumeric(I32 c)
{
  B32 result = (char_is_digit(c) || char_is_alphabetic(c));
  return(result);
}

B32
char_is_graphical(I32 c)
{
  B32 result = (char_is_punctuation(c) || char_is_alphanumeric(c));
  return(result);
}

B32
char_is_printable(I32 c)
{
  B32 result = (char_is_graphical(c) || c == ' ');
  return(result);
}

char 
char_to_upper(char ch)
{
  char result = ch;
  if (char_is_alphabetic(ch) && char_is_lower_alphabetic(ch))
  {
    result += 'A' - 'a';
  }
	return(result);
}

// //////////////////////////////////////////////
// NOTE(erb): string
// //////////////////////////////////////////////

char *
cstr_trim_start(char *str)
{
	while (*str && (*str == ' ' || *str == '\n' || *str == '\t'))
	{
		str++;
	}
	return str;
}

B32 
cstr_equals(char *a, char *b)
{
	B32 result = true;
	while (result && *a && *b)
	{
		result = (*a++ == *b++);
	}
	return result;
}

U64 
str8_hash(String8 str)
{
	U64 hash = 0;
	I32 p = 31;
  I32 m = (I32)(1e9 + 9);
	U64 ppow = 1;
	
	for (U64Each(char_idx, str.size))
	{
		U8 c = str.value[char_idx];
		hash = (hash + (c - 'a' + 1) * ppow) % m;
		ppow = (ppow * p) % m;
	}
	return hash;
}

Index_U64
str8_index_diff(String8 str_a, String8 str_b)
{
  Index_U64 result = {0};
	U64 size = Min(str_a.size, str_b.size);
  for (U64Each(idx, size))
  {
    if (str_a.value[idx] != str_b.value[idx])
    {
      result.exists = true;
      result.value = idx;
      break;
    }
  }
  if(!result.exists && str_a.size != str_b.size)
  {
    result.exists = true;
    result.value = size+1;
  }
	return(result);
}

B32 
str8_equals(String8 a, String8 b)
{
  B32 result = ((a.size == b.size) && 
                !str8_index_diff(a, b).exists);
  return(result);
}

B32
str8_equals_case_insens(String8 str_a, String8 str_b)
{
	B32 result = false;
	if (str_a.size == str_b.size)
	{
		result = true;
		for (U32Each(idx, str_a.size))
		{
      U8 a = char_to_upper(str_a.value[idx]);
      U8 b = char_to_upper(str_b.value[idx]);
			if (a != b)
      {
        result = false;
        break;
      }
		}
	}
	return(result);
}

String8
str8_postfix(String8 str, U64 from_idx)
{
  U64 idx = CapTop(from_idx, str.size);
  String8 result = S8Struct(str.value+idx, str.size-idx);
	return(result);
}

String8
str8_prefix(String8 str, U64 to_idx)
{
  String8 result = S8Struct(str.value, CapTop(to_idx, str.size));
	return(result);
}

String8 
str8_slice(String8 str, Range_U64 rng)
{
  String8 pre = str8_prefix(str, rng.max);
  String8 result = str8_postfix(pre, rng.min);
  return(result);
}

// //////////////////////////////////////////////
// NOTE(erb): finding and slicing
// //////////////////////////////////////////////

typedef enum Index_Flags
{
  IndexFlags_Backward            = (1 << 0),
  IndexFlags_ValuePlusOne        = (1 << 1),
  IndexFlags_ValueMinusOne       = (1 << 2),
  IndexFlags_CharCaseInsens      = (1 << 3),
  IndexFlags_FallbackValueZero   = (1 << 4),
  IndexFlags_FallbackValueLength = (1 << 5)
} Index_Flags;

typedef B32 Char_Predicate_Func(I32 c);

void
str8_index_apply_post_flags(Index_U64 *index, String8 str, Index_Flags flags)
{
  if (flags & IndexFlags_FallbackValueLength && !index->exists)
  {
    index->value = str.size;
    index->exists = true;
  }
  if (flags & IndexFlags_FallbackValueZero && !index->exists)
  {
    index->value = 0;
    index->exists = true;
  }
  if (flags & IndexFlags_ValuePlusOne && index->exists)
  {
    index->value += 1;
  }
  else if (flags & IndexFlags_ValueMinusOne && index->exists)
  {
    index->value -= 1;
  }
}

Index_U64
str8_index_predicate(String8 str, Char_Predicate_Func predicate, Index_Flags flags)
{
	Index_U64 result = {0};
  for (U64Each(it, str.size))
  {
    U64 idx = (flags & IndexFlags_Backward)?(str.size-it-1):(it);
    if (predicate(str.value[idx]))
    {
      result.value = idx;
      result.exists = true;
      break;
    }
  }
  str8_index_apply_post_flags(&result, str, flags);
  return(result);
}

Index_U64
str8_index(String8 str, char c, Index_Flags flags)
{
	Index_U64 result = {0};
  for (U64Each(it, str.size))
	{
    U64 idx = (flags & IndexFlags_Backward)?(str.size-it-1):(it);
		if ((flags & IndexFlags_CharCaseInsens)
        ? char_to_upper(c) == char_to_upper(str.value[idx])
        : c == str.value[idx])
		{
			result.value = idx;
			result.exists = true;
      break;
		}
	}
  str8_index_apply_post_flags(&result, str, flags);
	return(result);
}

String8 
str8_index_postfix(String8 str, char c, Index_Flags flags)
{
  String8 result = {0};
  Index_U64 idx = str8_index(str, c, flags);
  if (idx.exists)
  {
    result = str8_postfix(str, idx.value);
  }
  return(result);
}

String8
str8_index_postfix_predicate(String8 str, Char_Predicate_Func predicate, Index_Flags flags)
{
  String8 result = {0};
  Index_U64 idx = str8_index_predicate(str, predicate, flags);
  if (idx.exists)
  {
    result = str8_postfix(str, idx.value);
  }
  return(result);
}

String8 
str8_index_prefix(String8 str, char c, Index_Flags flags)
{
  String8 result = {0};
  Index_U64 idx = str8_index(str, c, flags);
  if (idx.exists)
  {
    result = str8_prefix(str, idx.value);
  }
  return(result);
}

String8
str8_index_prefix_predicate(String8 str, Char_Predicate_Func predicate, Index_Flags flags)
{
  String8 result = {0};
  Index_U64 idx = str8_index_predicate(str, predicate, flags);
  if (idx.exists)
  {
    result = str8_prefix(str, idx.value);
  }
  return(result);
}

Index_U64
str8_index_start(String8 str, char ch, U64 start, Index_Flags flags)
{
#if 1
  String8 slice = {0};
  if(flags & IndexFlags_Backward)
  {
    slice = str8_prefix(str, start);
  }
  else
  {
    slice = str8_postfix(str, start);
  }
#else
  String8 slice = str8_postfix(str, start);
#endif
	Index_U64 idx = str8_index(slice, ch, flags);
  if (idx.exists)
  {
    idx.value += start;
  }
	return idx;
}

Index_U64
str8_index_predicate_start(String8 str, Char_Predicate_Func predicate, U64 start, Index_Flags flags)
{
  String8 slice = {0};
  if(flags & IndexFlags_Backward)
  {
    slice = str8_prefix(str, start);
  }
  else
  {
    slice = str8_postfix(str, start);
  }
	Index_U64 idx = str8_index_predicate(slice, predicate, flags);
  if (idx.exists && !(flags & IndexFlags_Backward))
  {
    idx.value += start;
  }
	return idx;
}

String8
str8_pop_dir(String8 str, char dir_sep)
{
  String8 result = str;
  Index_U64 dir_sep_idx = str8_index(str, dir_sep, IndexFlags_Backward);
  if (dir_sep_idx.exists) 
  {
    result = str8_prefix(str, dir_sep_idx.value);
  }
  return result;
}

char *
push_cstr(Arena *arena, String8 str)
{
  char *result = PushArray(arena, str.size+1, char);
  mem_copy((U8 *)str.value, (U8 *)result, str.size);
  result[str.size] = 0;
  return result;
}

// //////////////////////////////////////////////
// NOTE(erb): conversion
// //////////////////////////////////////////////

B32 
cstr_to_U64(char *str, U64 *value)
{
	U64 max_value = ~(U64)0;
	*value = 0;
  
	while (*str 
         && char_is_digit(*str) 
         && *value < max_value)
	{
		*value = (*value)*10 + ((*str) - '0');
		str++;
	}
	
	B32 result = (*str == 0);
	return result;
}

B32
cstr_to_U32(char *str, U32 *value)
{
	U32 max_value = ~(U32)0;
	*value = 0;
  
	while (*str 
         && char_is_digit(*str) 
         && *value < max_value)
	{
		*value = (*value)*10 + ((*str) - '0');
		str++;
	}
	B32 result = (*str == 0);
	return result;
}

B32 
str8_to_U64(String8 Str, U64 *value)
{
	U64 maxvalue = ~(U64)0;
	*value = 0;
  
  U8 *base = Str.value;
	for (U64 Index = 0; 
       Index < Str.size
       && char_is_digit(*base) 
       && *value < maxvalue;
       base++, Index++)
	{
		*value = (*value)*10 + ((*base) - '0');
	}
	B32 result = (base == Str.value+Str.size);
	return(result);
}

Index_U64 
str8_try_to_U64(String8 str, U64 *value)
{
	U64 max_value = ~(U64)0;
	*value = 0;
  
	Index_U64 result = {0};
	Index_U64 digit_idx = str8_index_predicate(str, char_is_digit, 0);
	
	if (digit_idx.exists)
	{
		result.value = digit_idx.value;
		
		while (result.value < str.size 
           && *value < max_value)
		{
			char ch = str.value[result.value];
			if (char_is_digit(ch))
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


// ////////////////////////////////////////////////////
// NOTE(erb): string writer

String8_Writer
push_str8_writer(Arena *arena, U64 size)
{
  String8_Writer result = {0};
  String8 buffer = push_str8(arena, size);
  result.str.value = buffer.value;
  result.capacity = buffer.size;
  return(result);
}

void
str8_writer_reset(String8_Writer *writer)
{
  writer->str.size = 0;
}

U8 *
str8_writer_advance_bytes(String8_Writer *writer, U64 count)
{
  U8 *bytes = 0;
  if(writer->str.size + count <= writer->capacity)
  {
    bytes = writer->str.value + writer->str.size;
    writer->str.size += count;
  }
  else InvalidCodePath;
  return(bytes);
}

void
str8_writer_str(String8_Writer *writer, String8 str)
{
  U8 *dest = str8_writer_advance_bytes(writer, str.size);
  if(dest != 0)
  {
    ArrayCopy(str.value, dest, str.size);
  }
}

void
str8_writer_char(String8_Writer *writer, char ch, U32 count)
{
  U8 *dest = str8_writer_advance_bytes(writer, count);
  if(dest != 0)
  {
    ArraySetValues(dest, count, ch);
  }
}

void
write_u64(U8 *buffer, U32 digit_count, U64 value, U8 radix, B32 is_upper_case)
{
  char ch_offset = (is_upper_case ? 'A' : 'a');
  for(U32Each(idx, digit_count))
  {
    U64 write_idx = digit_count-idx-1;
    U8 digit = (value % radix);
    U8 ch = (digit >= 10) ? (digit - 10 + ch_offset) : (digit + '0');
    buffer[write_idx] = ch;
    value /= radix;
    Assert(idx < digit_count);
  }
}

void
str8_writer_u64(String8_Writer *writer, U64 value,  
                U8 radix, U32 digit_count, B32 upper_case)
{
  U8 *dest = str8_writer_advance_bytes(writer, digit_count);
  if(dest != 0)
  {
    write_u64(dest, digit_count, value, radix, upper_case);
  }
}

void 
str8_writer_replace(String8_Writer *writer, String8 replace_str, Range_U64 range)
{
  if(rng_u64_inclusive_range(range, rng_u64(0, writer->str.size))) 
  {
    I32 diff = (I32)replace_str.size - (I32)rng_u64_size(range);
    if(writer->str.size + diff < writer->capacity)
    {
      Scratch scratch = scratch_begin(0, 0);
      String8 replace_copy = push_str8_copy(scratch.arena, replace_str);
      String8 after_replace_copy = push_str8_copy(scratch.arena, str8_postfix(writer->str, range.max));
      
      writer->str.size = range.min;
      str8_writer_str(writer, replace_copy);
      str8_writer_str(writer, after_replace_copy);
      
      scratch_end(&scratch);
    }
    else InvalidCodePath;
  }
}

String8
str8_concat(Arena *arena, String8 a, String8 b)
{
  String8_Writer sw = push_str8_writer(arena, a.size + b.size);
  str8_writer_str(&sw, a);
  str8_writer_str(&sw, b);
  String8 result = sw.str;
  return(result);
}

String8
str8_concat_3(Arena *arena, String8 s1, String8 s2, String8 s3)
{
  String8_Writer sw = push_str8_writer(arena, s1.size+s2.size+s3.size);
  str8_writer_str(&sw, s1);
  str8_writer_str(&sw, s2);
  str8_writer_str(&sw, s3);
  String8 result = sw.str;
  return(result);
}

// ////////////////////////////////////////////////////
// NOTE(erb): string builder

// NOTE(erb): so we get nice 64 byte alignment
#define STR8_BUILDER_DEFAULT_PAYLOAD_SIZE (64-sizeof(String8_Chunk))

String8_Builder 
str8_builder_make(U32 payload_size)
{
  String8_Builder result = {0};
  result.chunk_payload_size = payload_size;
  return(result);
}

String8_Builder
str8_builder_make_default()
{
  String8_Builder result = str8_builder_make(STR8_BUILDER_DEFAULT_PAYLOAD_SIZE);
  return(result);
}

U8 *
str8_chunk_get_buffer(String8_Chunk *chunk)
{
  U8 *result = ((U8 *)chunk + sizeof(String8_Chunk));
  return(result);
}

String8
push_str8_from_builder(Arena *arena, String8_Builder *builder)
{
  String8 result = push_str8(arena, builder->total_size);
  U64 bytes_written = 0;
  for(EachNode(String8_Chunk, chunk, builder->first))
  {
    U8 *src = str8_chunk_get_buffer(chunk);
    U64 bytes_to_copy = Min(result.size - bytes_written, builder->chunk_payload_size);
    U8 *dest = (result.value + bytes_written);
    ArrayCopy(src, dest, bytes_to_copy);
    bytes_written += bytes_to_copy;
  }
  return(result);
}

String8
str8_builder_get_next_bytes(Arena *arena, String8_Builder *builder, U64 bytes)
{
  String8_Chunk *chunk = 0;
  
  if(builder->total_size < builder->count * builder->chunk_payload_size)
  {
    chunk = builder->last;
    
    U32 non_full_chunks = (U32)(builder->count * builder->chunk_payload_size - 
                                builder->total_size + builder->chunk_payload_size-1)/builder->chunk_payload_size;
    Assert(non_full_chunks == 1);
  }
  else
  {
    chunk = builder->first_free;
    if(chunk)
    {
      SLLStackPop(builder->first_free);
    }
    else
    {
      chunk = (String8_Chunk *)arena_push_size(arena, sizeof(String8_Chunk)+builder->chunk_payload_size);
    }
    ZeroStruct(chunk);
    SLLQueueAppend(builder->first, builder->last, chunk);
    builder->count += 1;
  }
  
  U64 cursor = builder->total_size % builder->chunk_payload_size;
  U64 bytes_to_copy = Min(bytes, builder->chunk_payload_size - cursor);
  U8 *dest = str8_chunk_get_buffer(chunk) + cursor;
  
  builder->total_size += bytes_to_copy;
  String8 result = S8Struct(dest, bytes_to_copy);
  return(result);
}

void
str8_builder_str(Arena *arena, String8_Builder *builder, String8 str)
{
  for(;str.size>0;)
  {
    String8 bytes = str8_builder_get_next_bytes(arena, builder, str.size);
    ArrayCopy(str.value, bytes.value, bytes.size);
    str = str8_postfix(str, bytes.size);
  }
}

void
str8_builder_char(Arena *arena, String8_Builder *builder, char ch, U32 count)
{
  for(;count>0;)
  {
    String8 bytes = str8_builder_get_next_bytes(arena, builder, count);
    ArraySetValues(bytes.value, bytes.size, ch);
    count -= (U32)bytes.size;
  }
}

void
str8_builder_u64(Arena *arena, String8_Builder *builder, U64 value, 
                 U8 radix, U32 digit_count, B32 upper_case)
{
  U64 divisor = 1;
  for(;divisor <= (value/radix);)
  {
    divisor *= radix;
  }
  char ch_offset = (upper_case ? 'A' : 'a');
  for(;digit_count>0;)
  {
    Assert(divisor > 0);
    String8 bytes = str8_builder_get_next_bytes(arena, builder, digit_count);
    for(U32Each(idx, bytes.size))
    {
      U8 digit = (U8)(value / divisor);
      U8 ch = (digit >= 10) ? (digit - 10 + ch_offset) : (digit + '0');
      bytes.value[idx] = ch;
      value = (value % divisor);
      divisor /= radix;
    }
  }
}

String8 
push_str8f_varg(Arena *arena, const char *format, va_list args_src);

void
str8_builderf(Arena *arena, String8_Builder *builder, const char *fmt, ...)
{
  Scratch scratch = scratch_begin(&arena, 1);
  
  va_list args;
  va_start(args, fmt);
  String8 str = push_str8f_varg(scratch.arena, fmt, args);
  va_end(args);
  
  str8_builder_str(arena, builder, str);
  scratch_end(&scratch);
}

// //////////////////////////////////////////////
// NOTE(erb): inplace operations

String8
str8_replace_char(String8 str, char origin, char replacement)
{
  String8 result = str;
  for (U64Each(char_idx, str.size))
  {
    U8 *c = (str.value + char_idx);
    if (*c == origin)
    {
      *c = replacement;
    }
  }
  return(result);
}

String8
push_str8_without_char(Arena *arena, String8 str, char to_remove)
{
  Scratch scratch = scratch_begin(0, 0);
  String8_Builder sb = str8_builder_make(safe_truncate_U64_to_U32(str.size));
  
  String8 it = str;
  for(;it.size > 0;)
  {
    U64 to_skip = 0;
    for(;to_skip < it.size; ++to_skip)
    {
      if(it.value[to_skip] != to_remove)
      {
        break;
      }
    }
    it = str8_postfix(it, to_skip);
    
    Index_U64 idx = str8_index(it, to_remove, IndexFlags_FallbackValueLength);
    str8_builder_str(scratch.arena, &sb, str8_prefix(it, idx.value));
    it = str8_postfix(it, idx.value);
  }
  
  String8 result = push_str8_from_builder(arena, &sb);
  scratch_end(&scratch);
  return(result);
}

// //////////////////////////////////////////////
// NOTE(erb): string list

String8_Node *
str8_list_push(Arena *arena, String8_List *list, String8 str)
{
  String8_Node *node = PushStruct(arena, String8_Node);
  node->str = str;
  SLLQueueAppend(list->first, list->last, node);
  list->count += 1;
  list->str_value_count += str.size;
  return(node);
}

U64
str8_join_total_size(String8_Join *join, U32 count)
{
  U64 sep_size = join->sep.size * Cap0((I64)count-1);
  U64 pre_post_size = (join->prefix.size + join->postfix.size) * count;
  U64 result = (pre_post_size + sep_size);
  return(result);
}

String8
str8_list_join(Arena *arena, String8_List *list, String8_Join join)
{
  U64 size = (list->str_value_count + str8_join_total_size(&join, list->count));
  
  String8_Writer sw = push_str8_writer(arena, size);
  for(EachNode(String8_Node, node, list->first))
  {
    str8_writer_str(&sw, join.prefix);
    str8_writer_str(&sw, node->str);
    str8_writer_str(&sw, join.postfix);
    if(node->next != 0)
    {
      str8_writer_str(&sw, join.sep);
    }
  }
  String8 result = sw.str;
  return(result);
}

String8_List
str8_split_blank(Arena *arena, String8 str)
{
  String8_List list = {0};
  String8 slice = str;
  for(;slice.size > 0;)
  {
    slice = str8_index_postfix_predicate(slice, char_is_not_blank, 0);
    if (slice.size != 0)
    {
      String8 word = str8_index_prefix_predicate(slice, char_is_blank, IndexFlags_FallbackValueLength);
      if (word.size != 0)
      {
        str8_list_push(arena, &list, word);
      }
      slice = str8_postfix(slice, word.size);
    }
    else
    {
      break;
    }
  }
  return(list);
}

String8_List
str8_split_char(Arena *arena, String8 str, char c)
{
  String8_List list = {0};
  String8 slice = str;
  for(;slice.size > 0;)
  {
    String8 chunk = str8_index_prefix(slice, c, IndexFlags_FallbackValueLength);
    str8_list_push(arena, &list, chunk);
    slice = str8_postfix(slice, chunk.size+1); // NOTE(erb): +1 is for the seperator char
  }
  return(list);
}

// //////////////////////////////////////////////
// NOTE(erb): string array
// //////////////////////////////////////////////

String8_Array
str8_array_from_cstr_array(Arena *arena, char **items, U32 count)
{
  String8_Array result = {0};
  result.items = PushArray(arena, count, String8);
  result.count = count;
  for (U64Each(idx, count))
  {
    result.items[idx] = S8CStr(items[idx]);
  }
  return(result);
}

B32
str8_array_contains(String8_Array array, String8 needle)
{
  B32 result = false;
  for(U32Each(idx, array.count))
  {
    String8 test = array.items[idx];
    if (str8_equals(test, needle))
    {
      result = true;
      break;
    }
  }
  return(result);
}

typedef enum Match_Flags
{
  MatchFlags_CaseInsens = (1<<0),
  MatchFlags_AnyChar    = (1<<1),
} Match_Flags;

B32
str8_match(String8 haystack, String8 needles, Match_Flags flags)
{
  B32 result = false;
  if(flags & MatchFlags_AnyChar)
  {
    Index_Flags index_flags = (flags & MatchFlags_CaseInsens) ? IndexFlags_CharCaseInsens : 0;
    for(U32Each(idx, haystack.size))
    {
      if(str8_index(needles, haystack.value[idx], index_flags).exists)
      {
        result = true;
        break;
      }
    }
  }
  else if(haystack.size == needles.size)
  {
    result = ((flags & MatchFlags_CaseInsens) 
              ? str8_equals_case_insens(haystack, needles)
              : str8_equals(haystack, needles));
  }
  return(result);
}

Index_U64
str8_find(String8 haystack, String8 needle, Match_Flags flags)
{
  Index_U64 result = {0};
  for (U32Each(idx, haystack.size))
  {
    String8 slice = str8_slice(haystack, rng_u64(idx, idx + needle.size));
    if (str8_match(slice, needle, flags))
    {
      result.exists = true;
      result.value = idx;
      break;
    }
  }
  return(result);
}

B32
str8_contains_char(String8 haystack, char ch)
{
  B32 result = str8_index(haystack, ch, 0).exists;
  return(result);
}

B32
str8_contains_char_case_insens(String8 haystack, char ch)
{
  B32 result = str8_index(haystack, ch, IndexFlags_CharCaseInsens).exists;
  return(result);
}

U64
str8_array_total_length(String8_Array *array)
{
  U64 result = 0;
  for (U64Each(str_idx, array->count))
  {
    result += array->items[str_idx].size;
  }
  return(result);
}

B32
str8_contains_str_array_node(String8 haystack, String8_Array *needles)
{
  B32 result = false;
  for (U64Each(idx, needles->count))
  {
    if(str8_find(haystack, needles->items[idx], MatchFlags_CaseInsens).exists)
    {
      result = true;
      break;
    }
  }
  return(result);
}

String8
str8_array_join(Arena *arena, String8_Array *array, String8_Join join)
{
  // NOTE(erb): calculate size
  U64 total_size = (str8_array_total_length(array) + 
                    str8_join_total_size(&join, array->count));
  
  // NOTE(erb): perform copy
  String8_Writer sw = push_str8_writer(arena, total_size);
  for(U64Each(str_idx, array->count))
  {
    str8_writer_str(&sw, join.prefix);
    str8_writer_str(&sw, array->items[str_idx]);
    str8_writer_str(&sw, join.postfix);
    if(str_idx < array->count-1)
    {
      str8_writer_str(&sw, join.sep);
    }
  }
  String8 result = sw.str;
  return(result);
}

B32
str8_hash_set_contains(String8_Hash_Set table, String8 needle)
{
  B32 result = false;
  U64 idx = str8_hash(needle) % table.slot_count;
  for(EachNode(String8_Node, slot, (table.slots + idx)))
  {
    if (str8_equals(slot->str, needle)) 
    {
      result = true;
      break;
    }
  }
  return result;
}


// //////////////////////////////////////////////
// NOTE(erb): format
// //////////////////////////////////////////////

B32
is_printf_flag(char ch)
{
  B32 result = str8_contains_char(S8(" -+#0"), ch);
  return(result);
}

B32
is_printf_specifier(char ch)
{
  B32 result = str8_contains_char(S8("diuoxXfFeEgGaAcsSpn%"), ch);
  return(result);
}


B32
is_printf_length_char(char ch)
{
  B32 result = str8_contains_char(S8("hljztL"), ch);
  return(result);
}


char *
printf_skip_format(char *fmt_start)
{
  char *it = fmt_start;
  
  it++;
  // NOTE(erb): flag
  if(is_printf_flag(*it))
  {
    it++;
  }
  // NOTE(erb): width
  if(*it == '*')
  {
    it++;
  }
  else 
  {
    while(char_is_digit(*it))
    {
      it++;
    }
  }
  // NOTE(erb): precision
  if(*it == '.')
  {
    it++;
    if(*it == '*')
    {
      it++;
    }
    else 
    {
      while(char_is_digit(*it))
      {
        it++;
      }
    }
  }
  // NOTE(erb): length
  switch(*it)
  {
    case 'h': 
    {
      it++;
      if(*it == 'h')
      {
        it++;
      }
    } break;
    
    case 'l': 
    {
      it++;
      if(*it == 'l')
      {
        it++;
      }
    } break;
    
    case 'j':
    case 'z':
    case 't':
    case 'L':
    {
      it++;
    } break;
  }
  // NOTE(erb): specifier
  if(is_printf_specifier(*it))
  {
    it++;
  }
  return(it);
}

typedef enum Printf_Flags
{
  PrintfFlags_LeftJustify        = (1<<0),
  PrintfFlags_ForceNumberSign    = (1<<1),
  PrintfFlags_BlankSpaceIfNoSign = (1<<2),
  PrintfFlags_Hash               = (1<<3),
  PrintfFlags_LeftPadWithZero    = (1<<4),
} Printf_Flags;

typedef struct Printf_Number Printf_Number;
struct Printf_Number
{
  U32 value;
  B32 is_dynamic;
  B32 is_specified;
};

typedef enum Printf_Length
{
  PrintfLength_None = 0,
  PrintfLength_hh,
  PrintfLength_h,
  PrintfLength_l,
  PrintfLength_ll,
  PrintfLength_j,
  PrintfLength_z,
  PrintfLength_t,
  PrintfLength_L,
} Printf_Length;

typedef struct Printf_Format Printf_Format;
struct Printf_Format
{
  Printf_Flags flags;
  Printf_Number width_num;
  Printf_Number precision_num;
  Printf_Length length;
  char specifier;
  
  String8 error;
};

Printf_Number
printf_parse_number(char **cursor)
{
  char *it = *cursor;
  Printf_Number result = {0};
  if (*it == '*')
  {
    result.is_dynamic = true;
    it++;
  }
  else
  {
    for(;char_is_digit(*it);++it)
    {
      result.value = (result.value*10) + ((*it)-'0');
    }
  }
  result.is_specified = (it != *cursor);
  (*cursor) = it;
  return(result);
}

Printf_Format
printf_parse_format(char **cursor)
{
  char *it = *cursor;
  String8 error = {0};
  
  if(*it == '%')
  {
    it++;
  }
  else
  {
    error = S8("no percent");
  }
  
  // NOTE(erb): flags
  Printf_Flags flags = 0;
  if(error.value == 0)
  {
    for(;is_printf_flag(*it);++it)
    {
      Printf_Flags specific_flag = 0;
      switch(*it)
      {
        case '-': specific_flag = PrintfFlags_LeftJustify;        break;
        case '+': specific_flag = PrintfFlags_ForceNumberSign;    break;
        case ' ': specific_flag = PrintfFlags_BlankSpaceIfNoSign; break;
        case '#': specific_flag = PrintfFlags_Hash;               break;
        case '0': specific_flag = PrintfFlags_LeftPadWithZero;    break;
      }
      if(specific_flag&flags)
      {
        error = S8("flag specified twice");
        break;
      }
      flags |= specific_flag;
    }
  }
  
  // NOTE(erb): width
  Printf_Number width_num = {0};
  if(error.value == 0)
  {
    width_num = printf_parse_number(&it);
  }
  
  // NOTE(erb): precision
  Printf_Number precision_num = {0};
  if(error.value == 0)
  {
    if(*it == '.')
    {
      it++;
      precision_num = printf_parse_number(&it);
    }
  }
  
  Printf_Length length = PrintfLength_None;
  if(error.value == 0)
  {
    switch(*it)
    {
      default: {}; break;
      case 'h': 
      {
        it++;
        if(*it == 'h')
        {
          length = PrintfLength_hh;
          it++;
        }
        else
        {
          length = PrintfLength_h;
        }
      } break;
      
      case 'l': 
      {
        it++;
        if(*it == 'l')
        {
          length = PrintfLength_ll;
          it++;
        }
        else
        {
          length = PrintfLength_l;
        }
      } break;
      
      // NOTE(erb): fallthrough
      case 'j': length = PrintfLength_j;
      case 'z': length = PrintfLength_z;
      case 't': length = PrintfLength_t;
      case 'L': length = PrintfLength_L;
      {
        it++;
      } break;
    }
  }
  
  char specifier = 0;
  if(error.value == 0)
  {
    if(is_printf_specifier(*it))
    {
      specifier = *it;
      it++;
    }
    else
    {
      error = S8("expected printf specifier");
    }
  }
  
  (*cursor) = it;
  Printf_Format result = {0};
  if(error.value == 0)
  {
    result.flags = flags;
    result.width_num = width_num;
    result.precision_num = precision_num;
    result.length = length;
    result.specifier = specifier;
  }
  return(result);
}

#define printf_resolve_number(num, args) (I32)((num).is_dynamic ? va_arg((args), int) : (num).value)
#define printf_resolve_number_or_default(num, args, value) (I32)((num).is_specified ? printf_resolve_number((num), (args)) : (value))

U32
digit_count_u64(U64 value, U8 radix)
{
  U32 count = 0;
  for(;value>0;++count)
  {
    value /= radix;
  }
  U32 result = Cap1(count);
  return(result);
}

typedef struct Printf_Metrics Printf_Metrics;
struct Printf_Metrics
{
  U32 digit_count;
  U32 total_length;
  U32 number_part_length;
  
  U8 sign_char;
  B32 has_sign_char;
  
  U8 padding_char;
  U32 padding_length;
  U32 zero_padding_length;
  
  String8 number_prefix;
};

Printf_Metrics
printf_get_metrics_i64(Printf_Format *format, I32 min_chars, I32 min_digits, U64 value, B32 is_negative, U8 radix)
{
  B32 has_sign_char = true;
  char sign_char = 0;
  if(is_negative)
  {
    sign_char = '-';
  }
  else if(format->flags & PrintfFlags_ForceNumberSign)
  {
    sign_char = '+';
  }
  else if(format->flags & PrintfFlags_BlankSpaceIfNoSign)
  {
    sign_char = ' ';
  }
  else
  {
    has_sign_char = false;
  }
  
  String8 number_prefix = {0};
  if(format->flags & PrintfFlags_Hash)
  {
    switch(format->specifier)
    {
      case 'o': number_prefix = S8("0");  break;
      case 'x': number_prefix = S8("0x"); break;
      case 'X': number_prefix = S8("0X"); break;
    }
  }
  
  U32 min_total_length = (U32)min_chars;
  U32 min_number_part_length = (U32)min_digits;
  
  U32 digit_count = digit_count_u64(value, radix);
  U32 signed_char_length = has_sign_char?1:0;
  
  U32 number_part_length = Max(digit_count, min_number_part_length) + (U32)number_prefix.size + signed_char_length;
  U32 total_length = Max(min_total_length, number_part_length);
  
  Assert(total_length >= number_part_length);
  U32 padding_length = (total_length - number_part_length);
  U8 padding_char = (format->flags & PrintfFlags_LeftPadWithZero)?'0':' ';
  
  U32 signed_digit_count = (digit_count + signed_char_length);
  U32 zero_padding_length = (number_part_length - signed_digit_count - (U32)number_prefix.size);
  if((format->flags & PrintfFlags_Hash) && format->specifier == 'o')
  {
    if(zero_padding_length > 0)
    {
      // NOTE(erb): zero padding and the '0' prefix are sort of 'merged' in the number part
      // so we remove one zero padding value
      zero_padding_length--;
    }
  }
  
  Printf_Metrics result = {0};
  result.digit_count         = digit_count;
  result.total_length        = total_length;
  result.number_part_length  = number_part_length;
  result.sign_char           = sign_char;
  result.has_sign_char       = has_sign_char;
  result.padding_char        = padding_char;
  result.padding_length      = padding_length;
  result.zero_padding_length = zero_padding_length;
  result.number_prefix       = number_prefix;
  return(result);
}

String8
printf_resolve_signed_u64(Arena *arena, Printf_Format *format, U64 value, B32 is_negative, I32 min_chars, I32 min_digits, U8 radix)
{
  Printf_Metrics metrics = printf_get_metrics_i64(format, min_chars, min_digits, value, is_negative, radix);
  
  String8_Writer sw = push_str8_writer(arena, metrics.total_length);
  
  // NOTE(erb): left padding (right justified)
  if(!(format->flags & PrintfFlags_LeftJustify) && !(format->flags & PrintfFlags_LeftPadWithZero))
  {
    str8_writer_char(&sw, metrics.padding_char, metrics.padding_length);
  }
  
  // NOTE(erb): sign char
  if(metrics.has_sign_char)
  {
    str8_writer_char(&sw, metrics.sign_char, 1);
  }
  
  // NOTE(erb): prefix
  str8_writer_str(&sw, metrics.number_prefix);
  
  // NOTE(erb): left padding (right justified)
  if(!(format->flags & PrintfFlags_LeftJustify) && (format->flags & PrintfFlags_LeftPadWithZero))
  {
    str8_writer_char(&sw, metrics.padding_char, metrics.padding_length);
  }
  
  // NOTE(erb): zero padding
  str8_writer_char(&sw, '0', metrics.zero_padding_length);
  
  // NOTE(erb): value
  B32 upper_case = (format->specifier == 'X');
  str8_writer_u64(&sw, value, radix, metrics.digit_count, upper_case);
  
  // NOTE(erb): right padding (left justified)
  if(format->flags & PrintfFlags_LeftJustify)
  {
    str8_writer_char(&sw, ' ', metrics.padding_length);
  }
  String8 result = sw.str;
  return(result);
}

#define arg_i64(args, Type) (I64)(Type)va_arg((args), Type)
#define arg_u64(args, Type) (U64)(Type)va_arg((args), Type)

U64
write_formatted_varg(char *buffer, U64 buffer_size, char *format, va_list args)
{
  U64 bytes_written = 0;
  for(char *it = format;*it;)
  {
    if (*it == '%')
    {
      U64 size_left = (Max(buffer_size, bytes_written) - bytes_written);
      char *dest = buffer != 0 ? (buffer + bytes_written) : 0;
      
      char *parse_it = it;
      Printf_Format parsed = printf_parse_format(&parse_it);
      
      Scratch scratch = scratch_begin(0, 0);
      String8 resolved = {0};
      {
        switch(parsed.specifier)
        {
          case 'd': case 'i':
          case 'o': case 'u':
          case 'x': case 'X':
          {
            
            // NOTE(erb): 
            // - min digits (precision field) pads with zeros
            // - min chars (width field) pads with space (by default) or zeros (flag), 
            //   according to the width (left or right justified)
            I32 min_chars = printf_resolve_number(parsed.width_num, args);
            Assert(min_chars >= 0);
            I32 min_digits = printf_resolve_number(parsed.precision_num, args);
            Assert(min_digits >= 0);
            
            U64 value = 0;
            B32 is_negative = false;
            if(parsed.specifier == 'd' || parsed.specifier == 'i')
            {
              Assert(!(parsed.flags & PrintfFlags_Hash));
              
              I64 value_i64 = 0;
              switch(parsed.length)
              {
                case PrintfLength_None: value_i64 = arg_i64(args, int);           break;
                case PrintfLength_hh  : value_i64 = arg_i64(args, signed char);   break;
                case PrintfLength_h   : value_i64 = arg_i64(args, short int);     break;
                case PrintfLength_l   : value_i64 = arg_i64(args, long int);      break;
                case PrintfLength_ll  : value_i64 = arg_i64(args, long long int); break;
                case PrintfLength_j   : value_i64 = arg_i64(args, intmax_t);      break;
                case PrintfLength_z   : value_i64 = arg_i64(args, size_t);        break;
                case PrintfLength_t   : value_i64 = arg_i64(args, ptrdiff_t);     break;
                case PrintfLength_L  : Assert(false); break;
              }
              is_negative = (value_i64 < 0);
              value = Abs(value_i64);
            }
            else
            {
              // NOTE(erb): '+' and ' ' flags are contradictory
              Assert(!((parsed.flags & PrintfFlags_ForceNumberSign) && 
                       (parsed.flags & PrintfFlags_BlankSpaceIfNoSign)));
              
              switch(parsed.length)
              {
                case PrintfLength_None: value = arg_u64(args, unsigned int);           break;
                case PrintfLength_hh  : value = arg_u64(args, unsigned char);          break;
                case PrintfLength_h   : value = arg_u64(args, unsigned short int);     break;
                case PrintfLength_l   : value = arg_u64(args, unsigned long int);      break;
                case PrintfLength_ll  : value = arg_u64(args, unsigned long long int); break;
                case PrintfLength_j   : value = arg_u64(args, uintmax_t);              break;
                case PrintfLength_z   : value = arg_u64(args, size_t);                 break;
                case PrintfLength_t   : value = arg_u64(args, ptrdiff_t);              break;
                case PrintfLength_L   : InvalidCodePath; break;
              }
            }
            
            U8 radix = 0;
            switch(parsed.specifier)
            {
              case 'x':
              case 'X': radix = 16; break;
              
              case 'o': radix = 8;  break;
              
              case 'd':
              case 'i':
              case 'u': radix = 10; break;
              InvalidDefaultCase;
            }
            
            resolved = printf_resolve_signed_u64(scratch.arena, &parsed, value, is_negative, min_chars, min_digits, radix);
          } break;
          
          case 'f': case 'F':
          case 'a': case 'A':
          {
            Assert((parsed.length == PrintfLength_None) || (parsed.length & PrintfLength_L));
            
            I32 min_chars = printf_resolve_number(parsed.width_num, args);
            Assert(min_chars >= 0);
            I32 digits_after_decimal_point = printf_resolve_number_or_default(parsed.precision_num, args, 6);
            Assert(digits_after_decimal_point >= 0);
            
            F64 value = 0;
            switch(parsed.length)
            {
              InvalidDefaultCase;
              case PrintfLength_None: value = (F64)(double)va_arg(args, double);           break;
              case PrintfLength_L   : value = (F64)(long double)va_arg(args, long double); break;
            }
            
            F64_Parts value_parts = split_parts_f64(value);
            
            {
              B32 has_sign_char = true;
              char sign_char = 0;
              if(value_parts.integer < 0)
              {
                sign_char = '-';
              }
              else if(parsed.flags & PrintfFlags_ForceNumberSign)
              {
                sign_char = '+';
              }
              else if(parsed.flags & PrintfFlags_BlankSpaceIfNoSign)
              {
                sign_char = ' ';
              }
              else
              {
                has_sign_char = false;
              }
              U32 signed_char_length = has_sign_char?1:0;
              
              B32 force_decimal_point = (parsed.flags & PrintfFlags_Hash);
              B32 has_decimal_point = (force_decimal_point || value_parts.fraction != 0 || digits_after_decimal_point != 0);
              U32 decimal_char_length = has_decimal_point?1:0;
              
              U32 min_total_length = (U32)min_chars;
              
              U32 integer_digit_count = digit_count_u64(Abs(value_parts.integer), 10);
              U32 digit_count = integer_digit_count + digits_after_decimal_point;
              
              U32 number_part_length = digit_count + signed_char_length + decimal_char_length;
              U32 total_length = Max(min_total_length, number_part_length);
              
              Assert(total_length >= number_part_length);
              U32 padding_length = (total_length - number_part_length);
              U8 padding_char = (parsed.flags & PrintfFlags_LeftPadWithZero)?'0':' ';
              
              U32 full_digit_count = (digit_count + signed_char_length + decimal_char_length);
              U32 zero_padding_length = (number_part_length - full_digit_count);
              
              {
                String8_Writer sw = push_str8_writer(scratch.arena, total_length);
                
                // NOTE(erb): left padding (right justified)
                if(!(parsed.flags & PrintfFlags_LeftJustify) && !(parsed.flags & PrintfFlags_LeftPadWithZero))
                {
                  str8_writer_char(&sw, padding_char, padding_length);
                }
                
                // NOTE(erb): sign char
                if(has_sign_char)
                {
                  str8_writer_char(&sw, sign_char, 1);
                }
                
                // NOTE(erb): left padding (right justified)
                if(!(parsed.flags & PrintfFlags_LeftJustify) && (parsed.flags & PrintfFlags_LeftPadWithZero))
                {
                  str8_writer_char(&sw, padding_char, padding_length);
                }
                
                // NOTE(erb): zero padding
                str8_writer_char(&sw, '0', zero_padding_length);
                
                // NOTE(erb): value
                {
                  // NOTE(erb): value integer
                  str8_writer_u64(&sw, Abs(value_parts.integer), 10, integer_digit_count, false);
                  
                  // NOTE(erb): decimal point
                  if(has_decimal_point)
                  {
                    str8_writer_char(&sw, '.', 1);
                  }
                  
                  // NOTE(erb): fractional part
                  {
                    // NOTE(erb): round the fraction to an integer
                    U64 integer_fraction = 0;
                    if(digits_after_decimal_point > 0)
                    {
                      F64 fraction = value_parts.fraction * power_f64(10, digits_after_decimal_point);
                      integer_fraction = round_f64_to_u64(Abs(fraction));
                    }
                    str8_writer_u64(&sw, integer_fraction, 10, digits_after_decimal_point, false);
                  }
                }
                
                // NOTE(erb): right padding (left justified)
                if(parsed.flags & PrintfFlags_LeftJustify)
                {
                  str8_writer_char(&sw, ' ', padding_length);
                }
                
                resolved = sw.str;
              }
            }
          } break;
          
          case 'e':
          {
            InvalidCodePath;
          } break;
          
          case 'E':
          {
            InvalidCodePath;
          } break;
          
          case 'g':
          {
            InvalidCodePath;
          } break;
          
          case 'G':
          {
            InvalidCodePath;
          } break;
          
          case 'c':
          {
            InvalidCodePath;
          } break;
          
          case 's': case 'S':
          {
            Printf_Flags allowed_flags = PrintfFlags_LeftJustify;
            Assert((parsed.flags & (~allowed_flags)) == 0);
            Assert(parsed.length == PrintfLength_None || parsed.length == PrintfLength_l);
            
            I32 min_chars = printf_resolve_number(parsed.width_num, args);
            Assert(min_chars >= 0);
            B32 has_max_chars = parsed.precision_num.is_specified;
            I32 max_chars = printf_resolve_number(parsed.precision_num, args);
            Assert(max_chars >= 0);
            
            B32 is_wide = (parsed.length == PrintfLength_l);
            if(is_wide)
            {
              // TODO(erb): wchar_t* processing here
              InvalidCodePath;
            }
            else
            {
              String8 str = {0};
              switch(parsed.specifier)
              {
                case 's':
                {
                  char *raw_str = (char *)va_arg(args, char *);
                  if(raw_str)
                  {
                    str = S8CStr(raw_str);
                  }
                } break;
                
                case 'S':
                {
                  str = va_arg(args, String8);
                } break;
                
                InvalidDefaultCase;
              }
              
              if(!has_max_chars)
              {
                max_chars = safe_truncate_U64_to_U32(Max(str.size, (U32)min_chars));
              }
              
              U32 padding = safe_truncate_U64_to_U32(((U32)min_chars > str.size) ? (min_chars - str.size) : 0);
              U64 total_size = Cap(str.size, (U32)min_chars, (U32)max_chars);
              
              String8_Writer sw = push_str8_writer(scratch.arena, total_size);
              
              if(!(parsed.flags & PrintfFlags_LeftJustify))
              {
                str8_writer_char(&sw, ' ', padding);
              }
              
              str8_writer_str(&sw, str8_prefix(str, max_chars));
              
              if(parsed.flags & PrintfFlags_LeftJustify)
              {
                str8_writer_char(&sw, ' ', padding);
              }
              
              resolved = sw.str;
            }
            
          } break;
          
          case 'p':
          {
            // NOTE(erb): skip 0 bytes
            U64 value = (U64)va_arg(args, void *);
            
            U32 skip_count = 0;
            for(U32Each(idx, sizeof(U64)))
            {
              U32 rev_idx = (sizeof(U64) - idx - 1);
              U64 byte = (value >> (rev_idx*8));
              if((byte & 0xFF) != 0)
              {
                break;
              }
              skip_count++;
            }
            U32 digit_count = 2*(sizeof(void *) - skip_count);
            
            String8_Writer sw = push_str8_writer(scratch.arena, digit_count+2);
            str8_writer_str(&sw, S8("0x"));
            str8_writer_u64(&sw, value, 16, digit_count, false);
            resolved = sw.str;
          } break;
          
          case 'n':
          {
            InvalidCodePath;
          } break;
          
          case '%':
          {
            Assert(parsed.flags == 0);
            AssertImplies(parsed.width_num.value == 0, !parsed.width_num.is_dynamic);
            AssertImplies(parsed.precision_num.value == 0, !parsed.precision_num.is_dynamic);
            Assert(parsed.length == PrintfLength_None);
            resolved = push_str8_copy(scratch.arena, S8("%"));
          } break;
        }
      }
      
      ArrayCopy(resolved.value, dest, CapTop(resolved.size, size_left));
      bytes_written += resolved.size;
      it = parse_it;
      
      scratch_end(&scratch);
    }
    else
    {
      if(bytes_written+1 < buffer_size)
      {
        buffer[bytes_written] = *it;
      }
      bytes_written++;
      it++;
    }
  }
  return(bytes_written);
}

U64
str8_format_write_varg_nt(String8 buffer, const char *format, va_list args)
{
  U64 result = write_formatted_varg((char *)buffer.value, buffer.size, (char *)format, args);
  return(result);
}

String8 
push_str8f_varg(Arena *arena, const char *format, va_list args_src)
{
  va_list args_for_size;
  va_copy(args_for_size, args_src);
  U64 bytes_written = str8_format_write_varg_nt(S8Struct(0, 0), format, args_for_size);
  U64 buffer_size = bytes_written + 1;
  va_end(args_for_size);
  
  va_list actual_args;
  va_copy(actual_args, args_src);
  String8 str = push_str8(arena, buffer_size);
  str8_format_write_varg_nt(str, format, actual_args);
  va_end(actual_args);
  
  str.size = bytes_written;
  return(str);
}

printf_format_function_tag(2, 3) String8 
push_str8f(Arena *arena, printf_format_arg const char *format, ...)
{
  va_list args;
  va_start(args, format);
  String8 str = push_str8f_varg(arena, format, args);
  va_end(args);
  return(str);
}


//////////////////////////////////////////////////////
// NOTE(erb): utf8


// NOTE(erb): https://en.wikipedia.org/wiki/UTF-8
UTF8_Encode
utf8_encode(I32 codepoint)
{
  UTF8_Encode utf8 = {0};
  if(codepoint <= 0x007F)
  {
    utf8.size = 1;
    utf8.bytes[0] = (U8)codepoint;
  }
  else if(codepoint <= 0x07FF)
  {
    utf8.size = 2;
    utf8.bytes[0] = (U8)(((codepoint >> 6) & bit_mask_5) | 0xC0);
    utf8.bytes[1] = (U8)(((codepoint     ) & bit_mask_6) | 0x80);
  }
  else if(codepoint <= 0xFFFF)
  {
    utf8.size = 3;
    utf8.bytes[0] = (U8)(((codepoint >> 12) & bit_mask_4) | 0xE0);
    utf8.bytes[1] = (U8)(((codepoint >>  6) & bit_mask_6) | 0x80);
    utf8.bytes[2] = (U8)(((codepoint      ) & bit_mask_6) | 0x80);
  }
  else if(codepoint <= 0x10FFFF)
  {
    utf8.size = 4;
    utf8.bytes[0] = (U8)(((codepoint >> 18) & bit_mask_3) | 0xF0);
    utf8.bytes[1] = (U8)(((codepoint >> 12) & bit_mask_6) | 0x80);
    utf8.bytes[2] = (U8)(((codepoint >>  6) & bit_mask_6) | 0x80);
    utf8.bytes[3] = (U8)(((codepoint      ) & bit_mask_6) | 0x80);
  }
  else
  {
    InvalidCodePath;
  }
  return(utf8);
}

U64
write_utf8_from_codepoints(U8 *buffer, U64 buffer_size, I32 *codepoints, U64 codepoint_count)
{
  U64 size = 0;
  for(U32 idx = 0; idx<codepoint_count; ++idx)
  {
    UTF8_Encode utf8 = utf8_encode(codepoints[idx]);
    if(size + utf8.size <= buffer_size)
    {
      ArrayCopy(utf8.bytes, (buffer+size), utf8.size);
    }
    size += utf8.size;
  }
  return(size);
}

String8
push_str8_utf8_encoded(Arena *arena, I32 *codepoints, U64 codepoint_count)
{
  U64 size = write_utf8_from_codepoints(0, 0, codepoints, codepoint_count);
  String8 str = push_str8(arena, size);
  write_utf8_from_codepoints(str.value, str.size, codepoints, codepoint_count);
  return(str);
}

B32
check_utf8_continuation_bytes(String8 buffer, U32 bytes_to_check)
{
  Assert(bytes_to_check > 0);
  Assert(buffer.size > 0);
  B32 result = false;
  if(bytes_to_check+1 <= buffer.size)
  {
    result = true;
    for(U64Each(idx, bytes_to_check))
    {
      U8 byte = buffer.value[idx+1];
      if((byte & 0xC0) != 0x80)
      {
        result = false;
        break;
      }
    }
  }
  else
  {
    InvalidCodePath;
  }
  return(result);
}

UTF8_Decode
utf8_decode(String8 buffer)
{
  UTF8_Decode result = {0};
  if(buffer.size >= 1)
  {
    B32 error = false;
    
    U8 b0 = buffer.value[0];
    if((b0 & 0x80) == 0)
    {
      result.byte_count = 1;
      result.codepoint = b0;
      error = !(result.codepoint >= 0 && result.codepoint <= 0x7F); // NOTE(erb): codepoint range validation
      Assert(!error);
    }
    else if((b0 & 0xE0) == 0xC0)
    {
      if(buffer.size >= 2)
      {
        error = !check_utf8_continuation_bytes(buffer, 1);
        if(!error)
        {
          result.byte_count = 2;
          result.codepoint = (((buffer.value[0] & bit_mask_5) << 6) |
                              ((buffer.value[1] & bit_mask_6)));
          error = !(result.codepoint >= 0x80 && result.codepoint <= 0x7FF); // NOTE(erb): codepoint range validation
        }
        Assert(!error);
      }
      else
      {
        InvalidCodePath;
      }
    }
    else if((b0 & 0xF0) == 0xE0)
    {
      if(buffer.size >= 3)
      {
        error = !check_utf8_continuation_bytes(buffer, 2);
        if(!error)
        {
          result.byte_count = 3;
          result.codepoint = (((buffer.value[0] & bit_mask_4) << 12) |
                              ((buffer.value[1] & bit_mask_6) << 6) |
                              ((buffer.value[2] & bit_mask_6)));
          error = !(result.codepoint >= 0x800 && result.codepoint <= 0xFFFF); // NOTE(erb): codepoint range validation
        }
        Assert(!error);
      }
      else
      {
        InvalidCodePath;
      }
    }
    else if((b0 & 0xF8) == 0xF0)
    {
      if(buffer.size >= 4)
      {
        error = !check_utf8_continuation_bytes(buffer, 3);
        if(!error)
        {
          result.byte_count = 4;
          result.codepoint = (((buffer.value[0] & bit_mask_3) << 18) |
                              ((buffer.value[1] & bit_mask_6) << 12) |
                              ((buffer.value[2] & bit_mask_6) << 6) |
                              ((buffer.value[3] & bit_mask_6)));
          error = !(result.codepoint >= 0x10000 && result.codepoint <= 0x10FFFF); // NOTE(erb): codepoint range validation
        }
        Assert(!error);
      }
      else
      {
        InvalidCodePath;
      }
    }
    else
    {
      InvalidCodePath;
    }
    
    if(!error)
    {
      error = (result.codepoint>= 0xD800 && result.codepoint <= 0xDFFF); // NOTE(erb): surrogate checking
    }
    
    if(error)
    {
      ZeroStruct(&result);
    }
  }
  return(result);
}

UTF8_Decode_Iterator
utf8_it(String8 str)
{
  UTF8_Decode_Iterator result = {0};
  result.base_str = str;
  return(result);
}

B32
utf8_it_next(UTF8_Decode_Iterator *it)
{
  B32 result = false;
  if(it->cursor < it->base_str.size)
  {
    UTF8_Decode decoded = utf8_decode(str8_postfix(it->base_str, it->cursor));
    it->cursor += decoded.byte_count;
    it->codepoint = decoded.codepoint;
    result = (decoded.byte_count > 0);
    Assert(result);
  }
  return(result);
}


B32
utf8_it_prev(UTF8_Decode_Iterator *it)
{
  B32 result = false;
  if(it->cursor > 0 && it->cursor <= it->base_str.size)
  {
    it->cursor -= 1;
    // NOTE(erb): go back while there is a multi-byte indicator (0x10yyxxxx)
    for(;it->cursor > 0; --it->cursor)
    {
      U8 byte = it->base_str.value[it->cursor];
      B32 is_multi_byte = ((byte >> 6) != 0x2);
      if(is_multi_byte)
      {
        break;
      }
      AssertImplies(!is_multi_byte, it->cursor != 0);
    }
    
    UTF8_Decode decoded = utf8_decode(str8_postfix(it->base_str, it->cursor));
    it->codepoint = decoded.codepoint;
    result = (decoded.byte_count > 0);
    Assert(result);
  }
  return(result);
}

U32
write_codepoints_from_utf8(I32 *codepoints, U64 codepoints_buffer_count, String8 utf8)
{
  U32 size = 0;
  UTF8_Decode_Iterator it = utf8_it(utf8);
  for(;utf8_it_next(&it);++size)
  {
    if(size+1 <= codepoints_buffer_count)
    {
      codepoints[size] = it.codepoint;
    }
  }
  return(size);
}

typedef struct Array_I32 Array_I32;
struct Array_I32
{
  I32 *buffer;
  U64 size;
};

Array_I32
push_utf8_decoded(Arena *arena, String8 utf8)
{
  U64 size = write_codepoints_from_utf8(0, 0, utf8);
  Array_I32 array = {0};
  array.size = size;
  array.buffer = PushArray(arena, size, I32);
  write_codepoints_from_utf8(array.buffer, array.size, utf8);
  return(array);
}

////////////////////////////////////////////////////////////////
// NOTE(erb): matching algorithms

U32 
str8_lev_distance(String8 a, String8 b)
{
  Scratch scratch = scratch_begin(0, 0);
  I32 *row0 = PushArray(scratch.arena, b.size+1, I32);
  I32 *row1 = PushArray(scratch.arena, b.size+1, I32);
  
  for(U32Each(idx, b.size+1))
  {
    row0[idx] = idx;
  }
  
  for(U32Each(i, a.size))
  {
    row1[0] = (i + 1);
    for(U32Each(j, b.size))
    {
      I32 delete_cost = (row0[j+1] + 1);
      I32 insert_cost = (row1[j] + 1);
      I32 subs_cost = (a.value[i]==b.value[j] ? row0[j] : row0[j]+1);
      
      row1[j+1] = Min(delete_cost, Min(insert_cost, subs_cost));
    }
    Swap(I32 *, row0, row1);
  }
  
  U32 result = row0[b.size];
  scratch_end(&scratch);
  return(result);
}

String8
str8_longest_common_substring(String8 a, String8 b)
{
  Scratch scratch = scratch_begin(0, 0);
  I32 *table = PushArray(scratch.arena, (b.size+1), I32);
  
  U64 max_length = 0;
  U64 end_pos = 0;
  
  for(U64 i = 1; i <= a.size; ++i)
  {
    for(U64 j = b.size; j >= 1; --j)
    {
      if(a.value[i-1] == b.value[j-1])
      {
        table[j] = table[j-1] + 1;
        if(table[j] > max_length)
        {
          max_length = table[j];
          end_pos = i;
        }
      }
      else
      {
        table[j] = 0;
      }
    }
  }
  
  U8 *src = (a.value + end_pos - max_length);
  String8 result = S8Struct(src, max_length);
  return(result);
}

U64
str8_overlap_postfix(String8 a, String8 b)
{
  U64 result = 0;
  for(U64 idx = Max(a.size, b.size); idx>0; --idx)
  {
    String8 a_end = str8_postfix(a, idx);
    String8 b_start = str8_prefix(b, idx);
    if(a_end.size > 0 && b_start.size > 0 && 
       str8_equals(a_end, b_start))
    {
      result = idx;
      break;
    }
  }
  return(result);
}

String8
str8_concat_no_postifx_overlap(Arena *arena, String8 a, String8 b)
{
  U64 over_ab = str8_overlap_postfix(a, b);
  U64 over_ba = str8_overlap_postfix(b, a);
  if(over_ba > over_ab)
  {
    Swap(String8, a, b);
  }
  U64 overlap = Max(over_ab, over_ba);
  
  String8_Writer sw = push_str8_writer(arena, a.size + b.size - overlap);
  str8_writer_str(&sw, a);
  str8_writer_str(&sw, str8_postfix(b, overlap));
  String8 result = sw.str;
  return(result);
}


typedef enum Scan_Direction
{
  ScanDirection_Forward,
  ScanDirection_Backward,
} Scan_Direction;

Index_U64
str8_scan_word(String8 str, U64 from, Scan_Direction direction)
{
  Index_U64 result = {0};
  if(direction == ScanDirection_Forward)
  {
    Index_U64 blank = str8_index_predicate_start(str, char_is_blank, from, IndexFlags_FallbackValueLength);
    result = str8_index_predicate_start(str, char_is_not_blank, blank.value, IndexFlags_FallbackValueLength);
    result.exists = blank.exists;
  }
  else
  {
    Index_U64 not_blank = str8_index_predicate_start(str, char_is_not_blank, from, IndexFlags_Backward);
    result = str8_index_predicate_start(str, char_is_blank, not_blank.value, IndexFlags_Backward|IndexFlags_ValuePlusOne);
    result.exists = not_blank.exists;
  }
  return(result);
}

I64
str8_scan_word_delta(String8 str, U64 start, I64 word_delta)
{
  Scan_Direction direction = (word_delta >= 0)?ScanDirection_Forward:ScanDirection_Backward;
  Index_U64 it = {};
  it.value = start;
  for(U32Each(idx, Abs(word_delta)))
  {
    Index_U64 scan = str8_scan_word(str, it.value, direction);
    if(!scan.exists)
    {
      break;
    }
    it = scan;
  }
  I64 result = (it.value - start);
  return(result);
}

I64
utf8_scan_delta(String8 str, U64 start, I64 delta)
{
  UTF8_Decode_Iterator it = {};
  it.base_str = str;
  it.cursor = start;
  for(U32Each(i, Abs(delta)))
  {
    B32 did_something = ((delta > 0) ? utf8_it_next(&it) : utf8_it_prev(&it));
    if (!did_something)
    {
      break;
    }
  }
  I64 result = (it.cursor - start); 
  return(result);
}
