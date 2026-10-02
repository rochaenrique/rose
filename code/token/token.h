/* date = August 26th 2026 0:08 pm */

#ifndef TOKEN_H
#define TOKEN_H

typedef enum Token_Kind
{
  TokenKind_None,
  TokenKind_Operator,
  TokenKind_Identifier,
  TokenKind_NumLiteral,
  TokenKind_StrLiteral,
  TokenKind_ParenOpen,
  TokenKind_ParenClose,
  TokenKind_ScopeOpen,
  TokenKind_ScopeClose,
  TokenKind_COUNT,
} Token_Kind;

typedef struct Token Token;
struct Token
{
  Token *next;
  Token_Kind kind;
  String8 str8;
  B32 is_postfix;
  U32 u32;
};

typedef struct Token_List Token_List;
struct Token_List
{
  Token *first;
  Token *last;
  U32 count;
};

typedef struct Token_Iterator Token_Iterator;
struct Token_Iterator
{
  Token *current;
  Token *locations[16];
  U32 location_count;
};

#endif //TOKEN_H
