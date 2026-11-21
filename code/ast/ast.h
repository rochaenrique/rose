/* date = October 24th 2026 9:31 pm */

#ifndef AST_H
#define AST_H

//////////////////////////////////////////////////
// NOTE(erb): AST
//////////////////////////////////////////////////

typedef enum AST_Binary_Kind
{
  ASTBinaryKind_Sum,
  ASTBinaryKind_Mul,
  ASTBinaryKind_Sub,
  ASTBinaryKind_Div,
  ASTBinaryKind_Mod,
  
  ASTBinaryKind_Eq,
  ASTBinaryKind_Gt,
  ASTBinaryKind_Ge,
  ASTBinaryKind_Lt,
  ASTBinaryKind_Le,
  
  ASTBinaryKind_BitAnd,
  ASTBinaryKind_BitOr,
  ASTBinaryKind_LogicAnd,
  ASTBinaryKind_LogicOr,
  
  ASTBinaryKind_FieldAccess,
  ASTBinaryKind_ArrayAccess,
  
  ASTBinaryKind_COUNT,
} AST_Binary_Kind;

typedef enum AST_Primary_Kind
{
  ASTPrimaryKind_Identifier,
  ASTPrimaryKind_NumLiteral,
  ASTPrimaryKind_StrLiteral,
} AST_Primary_Kind;

typedef enum AST_Kind
{
  ASTKind_None,
  ASTKind_Binary,
  ASTKind_Primary,
  ASTKind_Function,
  ASTKind_COUNT,
} AST_Kind;

typedef struct AST_Node AST_Node;

typedef struct AST_Node_List AST_Node_List;
struct AST_Node_List
{
  AST_Node *first;
  AST_Node *last;
  U32 count;
};

typedef struct AST_Node AST_Node;
struct AST_Node
{
  AST_Node *next;
  AST_Node *prev;
  AST_Node *parent;
  
  AST_Kind kind;
  
  union
  {
    struct
    {
      AST_Binary_Kind kind;
      AST_Node *left;
      AST_Node *right;
    } bin;
    
    struct
    {
      AST_Primary_Kind kind;
      union
      {
        B32 b32;
        U32 u32;
        String8 str8;
        AST_Node_List list;
      };
    } prim;
    
    struct
    {
      String8 name;
      AST_Node_List args;
    } func;
  };
};

#endif //AST_H
