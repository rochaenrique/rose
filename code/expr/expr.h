/* date = August 17th 2026 2:18 pm */

#ifndef EXPR_H
#define EXPR_H

typedef enum Expr_Binary_Kind
{
  ExprBinaryKind_Sum,
  ExprBinaryKind_Mul,
  ExprBinaryKind_Sub,
  ExprBinaryKind_Div,
  ExprBinaryKind_Mod,
  
  ExprBinaryKind_Eq,
  ExprBinaryKind_Gt,
  ExprBinaryKind_Ge,
  ExprBinaryKind_Lt,
  ExprBinaryKind_Le,
  
  ExprBinaryKind_BitAnd,
  ExprBinaryKind_BitOr,
  ExprBinaryKind_LogicAnd,
  ExprBinaryKind_LogicOr,
  
  ExprBinaryKind_Access,
  
  ExprBinaryKind_COUNT,
} Expr_Binary_Kind;

typedef enum Expr_Primary_Kind
{
  ExprPrimaryKind_Identifier,
  ExprPrimaryKind_NumLiteral,
  ExprPrimaryKind_StrLiteral,
} Expr_Primary_Kind;

typedef enum Expr_Kind
{
  ExprKind_None,
  ExprKind_Binary,
  ExprKind_Primary,
  ExprKind_Function,
  ExprKind_COUNT,
} Expr_Kind;

typedef struct Expr Expr;

typedef struct Expr_List Expr_List;
struct Expr_List
{
  Expr *first;
  Expr *last;
  U32 count;
};

typedef struct Expr Expr;
struct Expr
{
  Expr *next;
  Expr *prev;
  Expr *parent;
  
  Expr_Kind kind;
  
  union
  {
    struct
    {
      Expr_Binary_Kind kind;
      Expr *left;
      Expr *right;
    } bin;
    
    struct
    {
      Expr_Primary_Kind kind;
      union
      {
        B32 b32;
        U32 u32;
        String8 str8;
      };
    } prim;
    
    struct
    {
      String8 name;
      Expr_List args;
    } func;
  };
};

#endif //EXPR_H
