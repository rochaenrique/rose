Expr *
expression_from_eat_token_list(Arena *arena, Token_Iterator *it, String8 *error);

Expr_List 
expression_list_from_eat_token_list(Arena *arena, Token_Iterator *it, String8 *error)
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
        if(0){}
        
        else if(str8_equals(token->str8, S8("/" ))) expr->kind = ExprKind_BinaryDiv;
        else if(str8_equals(token->str8, S8("+" ))) expr->kind = ExprKind_BinarySum;
        else if(str8_equals(token->str8, S8("-" ))) expr->kind = ExprKind_BinarySub;
        else if(str8_equals(token->str8, S8("*" ))) expr->kind = ExprKind_BinaryMul;
        else if(str8_equals(token->str8, S8("%" ))) expr->kind = ExprKind_BinaryMod;
        
        else if(str8_equals(token->str8, S8("=="))) expr->kind = ExprKind_BinaryEq;
        else if(str8_equals(token->str8, S8(">" ))) expr->kind = ExprKind_BinaryGt;
        else if(str8_equals(token->str8, S8(">="))) expr->kind = ExprKind_BinaryGe;
        else if(str8_equals(token->str8, S8("<" ))) expr->kind = ExprKind_BinaryLt;
        else if(str8_equals(token->str8, S8("<="))) expr->kind = ExprKind_BinaryLe;
        
        else if(str8_equals(token->str8, S8("&" ))) expr->kind = ExprKind_BinaryBitAnd;
        else if(str8_equals(token->str8, S8("|" ))) expr->kind = ExprKind_BinaryBitOr;
        
        else if(str8_equals(token->str8, S8("&&"))) expr->kind = ExprKind_BinaryLogicAnd;
        else if(str8_equals(token->str8, S8("||"))) expr->kind = ExprKind_BinaryLogicOr;
        
        else if(str8_equals(token->str8, S8("." ))) expr->kind = ExprKind_BinaryAccess;
        
        else *error = S8("unkown operator");
      } break;
      
      case TokenKind_Identifier:
      {
        // NOTE(erb): simple identifier
        expr->str8 = token->str8;
        expr->kind = ExprKind_Identifier;
        
        // NOTE(erb): function parsing
        Token *paren_open = token_it_next_if_kind(it, TokenKind_ParenOpen);
        if(paren_open != 0)
        {
          
          for(Expr *last_arg = 0;;)
          {
            Expr *arg = expression_from_eat_token_list(arena, it, error);
            if(error->value != 0 || arg == 0)
            {
              break;
            }
            SLLQueueAppend(expr->right, last_arg, arg);
            token_it_next_if_op_str8(it, S8(","));
          }
          
          Token *paren_close = token_it_next_if_kind(it, TokenKind_ParenClose);
          if(paren_close != 0)
          {
            expr->kind = ExprKind_Function;
            token_it_next(it);
          }
          else
          {
            *error = S8("expected paren at end of function call");
          }
        }
      } break;
      
      case TokenKind_StrLiteral:
      {
        expr->kind = ExprKind_StrLiteral;
        expr->str8 = token->str8;
      } break;
      
      case TokenKind_NumLiteral:
      {
        expr->kind = ExprKind_NumLiteral;
        expr->u32 = token->u32;
      } break;
      
      case TokenKind_ParenOpen:
      {
        token_it_next(it);
        Expr *group = expression_from_eat_token_list(arena, it, error);
        if(error->value == 0 && group != 0)
        {
          if(token_it_peek_has_kind(it, TokenKind_ParenClose))
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
    DLLQueueAppend(result.first, result.last, expr);
  }
  return(result);
}

B32
expression_kind_is_binary_op(Expr_Kind kind)
{
  B32 result = (kind == ExprKind_BinarySum ||
                kind == ExprKind_BinaryMul ||
                kind == ExprKind_BinarySub ||
                kind == ExprKind_BinaryDiv ||
                kind == ExprKind_BinaryMod ||
                
                kind == ExprKind_BinaryEq ||
                kind == ExprKind_BinaryGt ||
                kind == ExprKind_BinaryGe ||
                kind == ExprKind_BinaryLt ||
                kind == ExprKind_BinaryLe ||
                
                kind == ExprKind_BinaryBitOr  ||
                kind == ExprKind_BinaryBitAnd ||
                
                kind == ExprKind_BinaryLogicOr  ||
                kind == ExprKind_BinaryLogicAnd ||
                
                kind == ExprKind_BinaryAccess);
  return(result);
}

void
expression_join_binary_op_kind(Expr_List *list, Expr_Kind kind, String8 *error)
{
  Assert(expression_kind_is_binary_op(kind));
  for(EachNode(Expr, expr, list->first))
  {
    if(expr->kind == kind && 
       (expr->left == 0 && expr->right == 0))
    {
      Expr *left = expr->prev;
      Expr *right = expr->next;
      if(left != 0 && right != 0)
      {
        DLLRemove(list->first, list->last, left);
        DLLRemove(list->first, list->last, right);
        left->prev = left->next = 0;
        right->prev = right->next = 0;
        expr->left = left;
        expr->right = right;
      }
      else
      {
        *error = S8("expected left and right side operands");
      }
    }
  }
}

Expr *
expression_from_eat_token_list(Arena *arena, Token_Iterator *it, String8 *error)
{
  Expr_List list = expression_list_from_eat_token_list(arena, it, error);
  
  if(error->value == 0)
  {
    // NOTE(erb): special case access to add the global $ identifier
    for(EachNode(Expr, expr, list.first))
    {
      if(expr->kind == ExprKind_BinaryAccess)
      {
        Expr *left = expr->prev;
        Expr *right = expr->next;
        if(right != 0 && (left == 0 || left->kind != ExprKind_Identifier))
        {
          // NOTE(erb): only case where a expression is added through op parsing
          left = PushStruct(arena, Expr);
          left->kind = ExprKind_Identifier;
          left->str8 = S8("$");
          
          DLLRemove(list.first, list.last, right);
          right->prev = right->next = 0;
          expr->right = right;
          expr->left = left;
        }
        else if(right != 0 && left != 0)
        {
          DLLRemove(list.first, list.last, left);
          DLLRemove(list.first, list.last, right);
          left->prev = left->next = 0;
          right->prev = right->next = 0;
          expr->left = left;
          expr->right = right;
        }
        else
        {
          *error = S8("expected left and right side operands");
        }
      }
    }
    
    Expr_Kind op_kinds[] = 
    {
      ExprKind_BinaryBitAnd,
      ExprKind_BinaryBitOr,
      
      ExprKind_BinaryMod,
      ExprKind_BinaryMul,
      ExprKind_BinaryDiv,
      ExprKind_BinarySum,
      ExprKind_BinarySub,
      
      ExprKind_BinaryEq,
      ExprKind_BinaryGt,
      ExprKind_BinaryGe,
      ExprKind_BinaryLt,
      ExprKind_BinaryLe,
      
      ExprKind_BinaryLogicAnd,
      ExprKind_BinaryLogicOr,
    };
    for(EachElement(Expr_Kind, kind, op_kinds))
    {
      expression_join_binary_op_kind(&list, *kind, error);
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

