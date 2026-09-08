/* date = April 24th 2026 1:05 pm */

#ifndef BASE_STRING_H
#define BASE_STRING_H

typedef struct String8 String8;
struct String8
{
  U8 *value;
	U64 size;
};
raddbg_type_view(String8, array($.value, $.size));


////////////////////////////////////////////////// 
// NOTE(erb): containers

typedef struct String8_Node String8_Node;
struct String8_Node
{
  struct String8_Node *next;
  String8 str;
};

typedef struct String8_Array String8_Array;
struct String8_Array
{
  String8 *items;
  U32 count;
};
raddbg_type_view(String8_Array, array($.items, $.count));

typedef struct String8_List String8_List;
struct String8_List
{
  String8_Node *first;
  String8_Node *last;
  U32 count;
  U64 str_value_count;
};
raddbg_type_view(String8_List, list($.first, $.next));

typedef struct String8_Hash_Set String8_Hash_Set;
struct String8_Hash_Set
{
  U32 total_count;
  U32 slot_count;
  String8_Node *slots;
};
raddbg_type_view(String8_List, array(&, slots, slot_count));

////////////////////////////////////////////////// 
// NOTE(erb): builder

typedef struct String8_Chunk String8_Chunk;
struct String8_Chunk
{
  String8_Chunk *next;
};

typedef struct String8_Builder String8_Builder;
struct String8_Builder
{
  String8_Chunk *first;
  String8_Chunk *last;
  String8_Chunk *first_free;
  U64 count;
  U64 total_size;
  U32 chunk_payload_size;
};

typedef struct String8_Writer String8_Writer;
struct String8_Writer
{
  String8 str;
  U64 capacity;
};

////////////////////////////////////////////////////////////
// NOTE(erb): utf8

typedef struct UTF8_Encode UTF8_Encode;
struct UTF8_Encode
{
  U32 size;
  U8 bytes[4];
};

typedef struct UTF8_Decode UTF8_Decode;
struct UTF8_Decode
{
  I32 codepoint;
  U32 byte_count;
};

typedef struct UTF8_Decode_Iterator UTF8_Decode_Iterator;
struct UTF8_Decode_Iterator
{
  I32 codepoint;
  U64 cursor;
  String8 base_str;
};

////////////////////////////////////////////////////////////
// NOTE(erb): utils
typedef struct String8_Join String8_Join;
struct String8_Join
{
  String8 sep;
  String8 prefix;
  String8 postfix;
};

////////////////////////////////////////////////////////////
// NOTE(erb): macros

#define S8StructRaw(_value, _size)   { .value = (U8 *)_value, .size = (U64)_size }
#define S8Struct(_value, _size)      (String8)S8StructRaw(_value, _size)

#define S8CStr(str)                  S8Struct((str), cstr_length((str)))
#define S8Lit(str)                   S8StructRaw(str, Cap0((sizeof(str) - 1)))
#define S8(str)                      (String8)S8Lit(str)
#define S8Z()                        S8Struct(0, 0)

#define S8Expand(str)                (I32)str.size, str.value
#define S8ArrayLit(...)              { .items = (String8[]){ __VA_ARGS__ }, .count = ArraySize((String8[]){ __VA_ARGS__ }), }

I32 codepoint_latin_suplement_begin = 0xA0;
I32 codepoint_latin_suplement_end = 0x100;

#endif //BASE_STRING_H
