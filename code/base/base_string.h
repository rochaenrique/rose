/* date = April 24th 2026 1:05 pm */

#ifndef BASE_STRING_H
#define BASE_STRING_H

typedef struct String
{
	char *value;
	U64 length;
} String;

typedef struct String_Builder
{
	String buffer;
	U64 capacity;
} String_Builder;

typedef struct String_Table_Node
{
  String str;
  struct String_Table_Node *next;
} String_Table_Node;

typedef struct String_Table
{
  U32 total_count;
  U32 slot_count;
  String_Table_Node *slots;
} String_Table;

#define S(str) (String){.value = str, .length = cap0((sizeof(str) - 1)) }
#define SC(str) (String){.value = str, .length = cstr_length(str) }
#define str_expand(str) (I32)str.length, str.value

#endif //BASE_STRING_H
