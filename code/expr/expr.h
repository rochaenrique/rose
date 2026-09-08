/* date = August 17th 2026 2:18 pm */

#ifndef EXPR_H
#define EXPR_H

typedef enum Expr_Kind
{
  ExprKind_None,
  
  ExprKind_BinarySum,
  ExprKind_BinaryMul,
  ExprKind_BinarySub,
  ExprKind_BinaryDiv,
  ExprKind_BinaryMod,
  
  ExprKind_BinaryEq,
  ExprKind_BinaryGt,
  ExprKind_BinaryGe,
  ExprKind_BinaryLt,
  ExprKind_BinaryLe,
  
  ExprKind_BinaryBitAnd,
  ExprKind_BinaryBitOr,
  ExprKind_BinaryLogicAnd,
  ExprKind_BinaryLogicOr,
  
  ExprKind_BinaryAccess,
  ExprKind_Identifier,
  ExprKind_NumLiteral,
  ExprKind_StrLiteral,
  
  ExprKind_Function,
} Expr_Kind;

typedef struct Expr Expr;
struct Expr
{
  Expr *prev;
  Expr *next;
  Expr *left;
  Expr *right;
  
  Expr_Kind kind;
  union
  {
    B32 b32;
    U32 u32;
    String8 str8;
  };
};

typedef struct Expr_List Expr_List;
struct Expr_List
{
  Expr *first;
  Expr *last;
};

#endif //EXPR_H
