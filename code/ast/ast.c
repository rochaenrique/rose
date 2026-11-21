AST_Node *
ast_from_token_it(Arena *arena, Token_Iterator *it, String8 *error);

AST_Node *
ast_from_token_list(Arena *arena, Token_List *list, String8 *error)
{
  Token_Iterator it = {0};
  it.current = list->first;
  AST_Node *result = ast_from_token_it(arena, &it, error);
  return(result);
}

void
ast_list_append(AST_Node_List *list, AST_Node *e)
{
  DLLQueueAppend(list->first, list->last, e);
  list->count += 1;
}

void
ast_list_remove(AST_Node_List *list, AST_Node *e)
{
  DLLRemove(list->first, list->last, e);
  list->count -= 1;
}

AST_Node_List
ast_comma_sep_list_from_token_it(Arena *arena, Token_Iterator *it, String8 *error, Token_Kind end_token_kind)
{
  AST_Node_List result = {0};
  for(;;)
  {
    if(result.count > 0 &&
       token_it_next_if_op_str8(it, S8(",")) == 0)
    {
      *error = S8("expected coma seperating values in list");
      break;
    }
    AST_Node *item = ast_from_token_it(arena, it, error);
    if(error->value != 0 || item == 0)
    {
      break;
    }
    ast_list_append(&result, item);
    token_it_next_if_op_str8(it, S8(","));
  }
  if(error->value == 0)
  {
    Token *close = token_it_next_if_kind(it, end_token_kind);
    if(close == 0)
    {
      *error = S8("expected close at end of list");
    }
  }
  return(result);
}

AST_Node_List 
ast_list_from_token_it(Arena *arena, Token_Iterator *it, String8 *error)
{
  Assert(it != 0);
  
  AST_Node_List result = {0};
  for(;;)
  {
    Token *token = token_it_peek(it);
    if(token == 0 || 
       (token->kind == TokenKind_ParenClose || 
        token->kind == TokenKind_BracketClose || 
        token->kind == TokenKind_Operator && str8_equals(token->str8, S8(","))))
    {
      break;
    }
    token_it_next(it);
    
    AST_Node *expr = 0;
    if(token->kind != TokenKind_ParenOpen &&
       token->kind != TokenKind_BracketOpen)
    {
      expr = PushStruct(arena, AST_Node);
    }
    switch(token->kind)
    {
      case TokenKind_Operator:
      {
        expr->kind = ASTKind_Binary;
        
        if(0){}
        else if(str8_equals(token->str8, S8("/" ))) expr->bin.kind = ASTBinaryKind_Div;
        else if(str8_equals(token->str8, S8("+" ))) expr->bin.kind = ASTBinaryKind_Sum;
        else if(str8_equals(token->str8, S8("-" ))) expr->bin.kind = ASTBinaryKind_Sub;
        else if(str8_equals(token->str8, S8("*" ))) expr->bin.kind = ASTBinaryKind_Mul;
        else if(str8_equals(token->str8, S8("%" ))) expr->bin.kind = ASTBinaryKind_Mod;
        
        else if(str8_equals(token->str8, S8("=="))) expr->bin.kind = ASTBinaryKind_Eq;
        else if(str8_equals(token->str8, S8(">" ))) expr->bin.kind = ASTBinaryKind_Gt;
        else if(str8_equals(token->str8, S8(">="))) expr->bin.kind = ASTBinaryKind_Ge;
        else if(str8_equals(token->str8, S8("<" ))) expr->bin.kind = ASTBinaryKind_Lt;
        else if(str8_equals(token->str8, S8("<="))) expr->bin.kind = ASTBinaryKind_Le;
        
        else if(str8_equals(token->str8, S8("&" ))) expr->bin.kind = ASTBinaryKind_BitAnd;
        else if(str8_equals(token->str8, S8("|" ))) expr->bin.kind = ASTBinaryKind_BitOr;
        
        else if(str8_equals(token->str8, S8("&&"))) expr->bin.kind = ASTBinaryKind_LogicAnd;
        else if(str8_equals(token->str8, S8("||"))) expr->bin.kind = ASTBinaryKind_LogicOr;
        
        else if(str8_equals(token->str8, S8("." ))) expr->bin.kind = ASTBinaryKind_FieldAccess;
        else *error = S8("unkown operator");
      } break;
      
      case TokenKind_Identifier:
      {
        Token *paren_open = token_it_next_if_kind(it, TokenKind_ParenOpen);
        if(paren_open != 0) // NOTE(erb): function parsing
        {
          AST_Node_List args_list = ast_comma_sep_list_from_token_it(arena, it, error, TokenKind_ParenClose);
          if(error->value == 0)
          {
            expr->kind = ASTKind_Function;
            expr->func.name = token->str8;
            expr->func.args = args_list;
          }
        }
        else                       // NOTE(erb): simple identifier
        {
          expr->kind = ASTKind_Primary;
          expr->prim.kind = ASTPrimaryKind_Identifier;
          expr->prim.str8 = token->str8;
          
          Token *bracket_open = token_it_next_if_kind(it, TokenKind_BracketOpen);
          if(bracket_open != 0) // NOTE(erb): array access parsing
          {
            AST_Node *accessor_expr = ast_from_token_it(arena, it, error);
            if(error->value == 0 && accessor_expr != 0)
            {
              Token *close = token_it_next_if_kind(it, TokenKind_BracketClose);
              if(close != 0)
              {
                // NOTE(erb): add the 'accessed'
                ast_list_append(&result, expr);
                
                // NOTE(erb): add the bin op
                expr = PushStruct(arena, AST_Node);
                expr->kind = ASTKind_Binary;
                expr->bin.kind = ASTBinaryKind_ArrayAccess;
                ast_list_append(&result, expr);
                
                // NOTE(erb): add the 'accessor' (later)
                expr = accessor_expr; 
              }
              else
              {
                *error = S8("expected bracket close");
              }
            }
            else
            {
              *error = S8("expected expression in bracket");
            }
          }
        }
      } break;
      
      case TokenKind_StrLiteral:
      {
        expr->kind = ASTKind_Primary;
        expr->prim.kind = ASTPrimaryKind_StrLiteral;
        expr->prim.str8 = token->str8;
      } break;
      
      case TokenKind_NumLiteral:
      {
        expr->kind = ASTKind_Primary;
        expr->prim.kind = ASTPrimaryKind_NumLiteral;
        expr->prim.u32 = token->u32;
      } break;
      
      case TokenKind_ParenOpen:
      {
        AST_Node *group = ast_from_token_it(arena, it, error);
        if(error->value == 0 && group != 0)
        {
          Token *close = token_it_next_if_kind(it, TokenKind_ParenClose);
          if(close != 0)
          {
            expr = group;
          }
          else
          {
            *error = S8("expected paren close");
          }
        }
        else
        {
          *error = S8("expected expression in paren");
        }
      } break;
      
      InvalidDefaultCase;
    }
    if(error->value != 0)
    {
      break;
    }
    Assert(expr != 0);
    ast_list_append(&result, expr);
  }
  return(result);
}


void
ast_join_binary(AST_Node_List *list, AST_Node *ast, String8 *error)
{
  AST_Node *left = ast->prev;
  AST_Node *right = ast->next;
  if(left != 0 && right != 0)
  {
    ast_list_remove(list, left);
    ast_list_remove(list, right);
    left->prev = left->next = 0;
    right->prev = right->next = 0;
    
    ast->bin.right = right;
    ast->bin.right->next = left;
    
    ast->bin.left = left;
    ast->bin.left->prev = right;
  }
  else
  {
    *error = S8("expected left and right side operands");
  }
}

void
ast_join_binary_left_right(AST_Node_List *list, AST_Binary_Kind bin_kind, String8 *error)
{
  for(EachNode(AST_Node, ast, list->first))
  {
    if(ast->kind == ASTKind_Binary && 
       ast->bin.kind == bin_kind && 
       ast->bin.right == 0 && ast->bin.left == 0) // NOTE(erb): not joined yet
    {
      ast_join_binary(list, ast, error);
      if(error->value == 0)
      {
        break;
      }
    }
  }
}

AST_Node *
ast_from_token_it(Arena *arena, Token_Iterator *it, String8 *error)
{
  AST_Node_List list = ast_list_from_token_it(arena, it, error);
  
  // NOTE(erb): join field and array access (same precedence)
  if(error->value == 0)
  {
    for(EachNode(AST_Node, ast, list.first))
    {
      if(ast->kind == ASTKind_Binary && 
         (ast->bin.kind == ASTBinaryKind_FieldAccess ||
          ast->bin.kind == ASTBinaryKind_ArrayAccess) && 
         ast->bin.right == 0 && ast->bin.left == 0)
      {
        ast_join_binary(&list, ast, error);
        if(error->value != 0)
        {
          break;
        }
      }
    }
  }
  
  if(error->value == 0)
  {
    AST_Binary_Kind bin_kinds[] = 
    {
      ASTBinaryKind_BitAnd,
      ASTBinaryKind_BitOr,
      
      ASTBinaryKind_Mod,
      ASTBinaryKind_Mul,
      ASTBinaryKind_Div,
      ASTBinaryKind_Sum,
      ASTBinaryKind_Sub,
      
      ASTBinaryKind_Eq,
      ASTBinaryKind_Gt,
      ASTBinaryKind_Ge,
      ASTBinaryKind_Lt,
      ASTBinaryKind_Le,
      
      ASTBinaryKind_LogicAnd,
      ASTBinaryKind_LogicOr,
    };
    for(EachElement(AST_Binary_Kind, kind, bin_kinds))
    {
      ast_join_binary_left_right(&list, *kind, error);
      if(error->value != 0)
      {
        break;
      }
    }
  }
  
  AST_Node *result = 0;
  if(error->value == 0)
  {
    if(list.first == list.last)
    {
      result = list.first;
    }
    else 
    {
      *error = S8("invalid expression");
    }
    AssertImplies(result != 0, result->prev == 0);
    AssertImplies(result != 0, result->next == 0);
  }
  return(result);
}

AST_Node *
ast_from_str8(Arena *arena, String8 str, String8 *error)
{
  Scratch scratch = scratch_begin(&arena, 1);
  
  AST_Node *result = 0;
  Token_List tokens = token_list_from_str(scratch.arena, str, error);
  if(error->value == 0)
  {
    Token_Iterator token_it = {0};
    token_it.current = tokens.first;
    result = ast_from_token_it(arena, &token_it, error);
  }
  
  scratch_end(&scratch);
  return(result);
}

