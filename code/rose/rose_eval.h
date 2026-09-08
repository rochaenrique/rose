/* date = August 26th 2026 0:23 pm */

#ifndef ROSE_EVAL_H
#define ROSE_EVAL_H

typedef struct Date_Lit_Result Date_Lit_Result;
struct Date_Lit_Result
{
  Calendar_Date date;
  B32 success;
  B32 has_year;
};

typedef struct Expr_Value Expr_Value;
typedef struct Expr_Value_List Expr_Value_List;
struct Expr_Value_List
{
  Expr_Value *first;
  Expr_Value *last;
  U32 count;
};

typedef enum Expr_Type
{
  ExprType_B32,
  ExprType_U32,
  ExprType_Str8,
  ExprType_DateTime,
  ExprType_Event,
  ExprType_List,
} Expr_Type;

typedef struct Expr_Value Expr_Value;
struct Expr_Value
{
  Expr_Type type;
  union
  {
    String8 str8;
    U32 u32;
    B32 b32;
    Date_Time dt;
    Event *event;
    Expr_Value_List list;
  };
  Expr_Value *next;
};

// TODO(erb): return error as well
#define EXPR_FUNCTION_IMPL_FUNC(name) Expr_Value name(Arena *arena, Expr *expr, void *data)
typedef EXPR_FUNCTION_IMPL_FUNC(Expr_Function_Impl_Func);

typedef struct Expr_Variable Expr_Variable;
struct Expr_Variable
{
  String8 iden;
  B32 is_func;
  union
  {
    Expr_Value value;
    Expr_Function_Impl_Func *func;
  };
};

typedef struct Expr_Environment Expr_Environment;
struct Expr_Environment
{
  Expr_Variable *var_arr;
  U32 var_count;
  void *data;
};

#endif //ROSE_EVAL_H
