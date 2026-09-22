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
  ExprType_None,
  ExprType_B32,
  ExprType_U32,
  ExprType_Str8,
  ExprType_DateTime,
  ExprType_Event,
  ExprType_List,
  ExprType_COUNT,
} Expr_Type;

typedef enum Expr_UI_Rule
{
  ExprUIRule_None,
  ExprUIRule_Raw,
  ExprUIRule_Calendar,
  ExprUIRule_FocusEvent,
  ExprUIRule_Columns,
} Expr_UI_Rule;

typedef struct Expr_Value Expr_Value;
struct Expr_Value
{
  Expr_Type type;
  Expr_UI_Rule ui_rule;
  union
  {
    String8 str8;
    U32 u32;
    B32 b32;
    Date_Time dt;
    Event *event;
    struct
    {
      Expr_Value_List list;
      Expr_Type sub_type;
    };
  };
  Expr_Value *next;
};

typedef struct Expr_Func_Impl_Result Expr_Func_Impl_Result;
struct Expr_Func_Impl_Result
{
  Expr_Value value;
  String8 error;
};

typedef struct Expr_Environment Expr_Environment;
#define EXPR_FUNCTION_IMPL_FUNC(name) Expr_Func_Impl_Result name(Arena *arena, Expr_Environment *env, Expr_Value_List *arg_values, Expr_List *arg_exprs)
typedef EXPR_FUNCTION_IMPL_FUNC(Expr_Function_Impl_Func);

typedef struct Expr_Variable Expr_Variable;
struct Expr_Variable
{
  String8 iden;
  B32 is_func;
  union
  {
    Expr_Value value;
    struct
    {
      Expr_Function_Impl_Func *func;
      B32 only_expr_args;
    };
  };
  Expr_Variable *next;
};

typedef struct Expr_Environment Expr_Environment;
struct Expr_Environment
{
  Expr_Variable *first_var;
  void *data;
};

#endif //ROSE_EVAL_H
