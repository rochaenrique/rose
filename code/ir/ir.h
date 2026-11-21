/* date = October 25th 2026 3:44 pm */

#ifndef IR_H
#define IR_H

typedef enum IR_Type_Kind
{
  IRTypeKind_None,
  IRTypeKind_U32,
  IRTypeKind_B32,
  IRTypeKind_U8,
  IRTypeKind_AddressOf,
  IRTypeKind_Function,
  IRTypeKind_Record,
} IR_Type_Kind;

typedef struct IR_Type_ID IR_Type_ID;
struct IR_Type_ID
{
  U32 u32;
};

typedef struct IR_Field_Type IR_Field_Type;
struct IR_Field_Type
{
  U32 byte_offset;
  String8 name;
  IR_Type_ID type;
};

typedef struct IR_Type IR_Type;
struct IR_Type
{
  IR_Type_ID id;
  
  IR_Type_Kind kind;
  IR_Type_ID sub_type;
  IR_Field_Type fields[8];
  U32 field_count;
  U32 byte_count;
  
  String8 record_name;
  B32 is_array_record;
  
  IR_Type *next; // NOTE(erb): next in hash
};


// NOTE(erb): a struct to use as the array layout
typedef struct IR_Array IR_Array;
struct IR_Array
{
  void *data;
  U32 count;
};

typedef struct IR_Symbol_Type IR_Symbol_Type;
struct IR_Symbol_Type
{
  String8 name;
  IR_Type_ID type;
  IR_Symbol_Type *next;
};

typedef enum IR_Op_Kind
{
  IROpKind_None,
  IROpKind_FunctionCall,
  IROpKind_Identifier,
  IROpKind_OffsetAccessDeref,
  IROpKind_LiteralU32,
  IROpKind_LiteralStr8,
  
  IROpKind_Sum,
  IROpKind_Mul,
  IROpKind_Sub,
  IROpKind_Div,
  IROpKind_Mod,
  IROpKind_Eq,
  IROpKind_Gt,
  IROpKind_Ge,
  IROpKind_Lt,
  IROpKind_Le,
  IROpKind_BitAnd,
  IROpKind_BitOr,
  IROpKind_LogicAnd,
  IROpKind_LogicOr,
} IR_Op_Kind;

typedef enum IR_Access_Kind
{
  IRAccessKind_AddressOf,
  IRAccessKind_Direct,
} IR_Access_Kind;

typedef struct IR_Op IR_Op;
struct IR_Op
{
  IR_Op_Kind kind;
  IR_Type_ID type;
  IR_Type_ID func_type;
  IR_Access_Kind access_kind;
  union
  {
    U32 u32;
    String8 str8;
  };
  
  IR_Op *next;
};

typedef struct IR_Op_List IR_Op_List;
struct IR_Op_List
{
  IR_Op *first;
  IR_Op *last;
  U32 count;
};

typedef struct IR_Type_Table IR_Type_Table;
struct IR_Type_Table
{
  B32 initialized;
  IR_Type types[128];
  U32 count;
};

typedef struct E_Value E_Value;
struct E_Value
{
  IR_Type_ID type;
  U32 memory_start;
  U32 memory_size;
};

typedef struct E_Stack E_Stack;
struct E_Stack
{
  U8 memory[1024];
  Arena memory_arena;
  E_Value values[32];
  U32 value_count;
};

typedef struct E_Stack_Snapshot E_Stack_Snapshot;
struct E_Stack_Snapshot
{
  Buffer memory;
  E_Value *values;
  U32 value_count;
};

typedef struct E_Value_Interp E_Value_Interp;
struct E_Value_Interp
{
  E_Value value;
  Buffer memory;
};

#define E_FUNC_ADDR_IMPL(name) E_Value name(Arena *arena, E_Value *args, U32 arg_count, \
Buffer *stack_memory, IR_Type_Table *type_table, void *user_data)
typedef E_FUNC_ADDR_IMPL(E_Func_Type);

typedef struct E_Global_Iden E_Global_Iden;
struct E_Global_Iden
{
  String8 name;
  B32 is_value_not_func;
  IR_Type_ID type;
  
  union
  {
    struct
    {
      E_Func_Type *func;
      void *user_data;
    };
    String8 value_bytes;
  };
  
  E_Global_Iden *next;
};

#endif //IR_H
