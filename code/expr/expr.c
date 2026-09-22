Expr *
expr_from_token_it(Arena *arena, Token_Iterator *it, String8 *error);

void
expr_list_append(Expr_List *list, Expr *e)
{
  DLLQueueAppend(list->first, list->last, e);
  list->count += 1;
}

void
expr_list_remove(Expr_List *list, Expr *e)
{
  DLLRemove(list->first, list->last, e);
}

Expr_List 
expr_list_from_token_it(Arena *arena, Token_Iterator *it, String8 *error)
{
  Assert(it != 0);
  
  Expr_List result = {0};
  for(;;)
  {
    Token *token = token_it_peek(it);
    if(token == 0 || 
       (token->kind == TokenKind_ParenClose || 
        token->kind == TokenKind_Operator && str8_equals(token->str8, S8(","))))
    {
      break;
    }
    token_it_next(it);
    
    Expr *expr = 0;
    if(token->kind != TokenKind_ParenOpen)
    {
      expr = PushStruct(arena, Expr);
    }
    switch(token->kind)
    {
      case TokenKind_Operator:
      {
        expr->kind = ExprKind_Binary;
        
        if(0){}
        else if(str8_equals(token->str8, S8("/" ))) expr->bin.kind = ExprBinaryKind_Div;
        else if(str8_equals(token->str8, S8("+" ))) expr->bin.kind = ExprBinaryKind_Sum;
        else if(str8_equals(token->str8, S8("-" ))) expr->bin.kind = ExprBinaryKind_Sub;
        else if(str8_equals(token->str8, S8("*" ))) expr->bin.kind = ExprBinaryKind_Mul;
        else if(str8_equals(token->str8, S8("%" ))) expr->bin.kind = ExprBinaryKind_Mod;
        
        else if(str8_equals(token->str8, S8("=="))) expr->bin.kind = ExprBinaryKind_Eq;
        else if(str8_equals(token->str8, S8(">" ))) expr->bin.kind = ExprBinaryKind_Gt;
        else if(str8_equals(token->str8, S8(">="))) expr->bin.kind = ExprBinaryKind_Ge;
        else if(str8_equals(token->str8, S8("<" ))) expr->bin.kind = ExprBinaryKind_Lt;
        else if(str8_equals(token->str8, S8("<="))) expr->bin.kind = ExprBinaryKind_Le;
        
        else if(str8_equals(token->str8, S8("&" ))) expr->bin.kind = ExprBinaryKind_BitAnd;
        else if(str8_equals(token->str8, S8("|" ))) expr->bin.kind = ExprBinaryKind_BitOr;
        
        else if(str8_equals(token->str8, S8("&&"))) expr->bin.kind = ExprBinaryKind_LogicAnd;
        else if(str8_equals(token->str8, S8("||"))) expr->bin.kind = ExprBinaryKind_LogicOr;
        
        else if(str8_equals(token->str8, S8("." ))) expr->bin.kind = ExprBinaryKind_Access;
        
        else *error = S8("unkown operator");
      } break;
      
      case TokenKind_Identifier:
      {
        Token *paren_open = token_it_next_if_kind(it, TokenKind_ParenOpen);
        if(paren_open == 0) // NOTE(erb): simple identifier
        {
          expr->kind = ExprKind_Primary;
          expr->prim.kind = ExprPrimaryKind_Identifier;
          expr->prim.str8 = token->str8;
        }
        else // NOTE(erb): function parsing
        {
          expr->kind = ExprKind_Function;
          expr->func.name = token->str8;
          
          for(;;)
          {
            Expr *arg = expr_from_token_it(arena, it, error);
            if(error->value != 0 || arg == 0)
            {
              break;
            }
            expr_list_append(&expr->func.args, arg);
            token_it_next_if_op_str8(it, S8(","));
          }
          
          Token *paren_close = token_it_next_if_kind(it, TokenKind_ParenClose);
          if(paren_close != 0)
          {
            expr->kind = ExprKind_Function;
          }
          else
          {
            *error = S8("expected paren at end of function call");
          }
        }
      } break;
      
      case TokenKind_StrLiteral:
      {
        expr->kind = ExprKind_Primary;
        expr->prim.kind = ExprPrimaryKind_StrLiteral;
        expr->prim.str8 = token->str8;
      } break;
      
      case TokenKind_NumLiteral:
      {
        expr->kind = ExprKind_Primary;
        expr->prim.kind = ExprPrimaryKind_NumLiteral;
        expr->prim.u32 = token->u32;
      } break;
      
      case TokenKind_ParenOpen:
      {
        Expr *group = expr_from_token_it(arena, it, error);
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
    expr_list_append(&result, expr);
  }
  return(result);
}

void
expr_join_binary(Expr_List *list, Expr_Binary_Kind bin_kind, String8 *error)
{
  for(EachNode(Expr, expr, list->first))
  {
    if(expr->kind == ExprKind_Binary && 
       expr->bin.kind == bin_kind && 
       expr->bin.right == 0 && expr->bin.left == 0) // NOTE(erb): not joined yet
    {
      Expr *left = expr->prev;
      Expr *right = expr->next;
      if(left != 0 && right != 0)
      {
        expr_list_remove(list, left);
        expr_list_remove(list, right);
        left->prev = left->next = 0;
        right->prev = right->next = 0;
        
        expr->bin.right = right;
        expr->bin.right->next = left;
        
        expr->bin.left = left;
        expr->bin.left->prev = right;
      }
      else
      {
        *error = S8("expected left and right side operands");
      }
    }
  }
}

Expr *
expr_from_token_it(Arena *arena, Token_Iterator *it, String8 *error)
{
  Expr_List list = expr_list_from_token_it(arena, it, error);
  
  if(error->value == 0)
  {
    Expr_Binary_Kind bin_kinds[] = 
    {
      ExprBinaryKind_Access,
      
      ExprBinaryKind_BitAnd,
      ExprBinaryKind_BitOr,
      
      ExprBinaryKind_Mod,
      ExprBinaryKind_Mul,
      ExprBinaryKind_Div,
      ExprBinaryKind_Sum,
      ExprBinaryKind_Sub,
      
      ExprBinaryKind_Eq,
      ExprBinaryKind_Gt,
      ExprBinaryKind_Ge,
      ExprBinaryKind_Lt,
      ExprBinaryKind_Le,
      
      ExprBinaryKind_LogicAnd,
      ExprBinaryKind_LogicOr,
    };
    for(EachElement(Expr_Kind, kind, bin_kinds))
    {
      expr_join_binary(&list, *kind, error);
      if(error->value != 0)
      {
        break;
      }
    }
  }
  
  Expr *result = 0;
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

