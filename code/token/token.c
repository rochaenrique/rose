B32
char_is_identifier(I32 c)
{
  B32 result = (char_is_alphabetic(c) || c == '_' || c == '$');
  return(result);
}

B32
char_is_not_identifier(I32 c)
{
  B32 result = !char_is_identifier(c);
  return(result);
}

B32
char_is_operator(I32 c)
{
  String8 s = S8Struct(&c, 1);
  B32 result = str8_match(s, S8("/+-*%.<>=&|,:"), MatchFlags_AnyChar);
  return(result);
}

B32
char_is_not_operator(I32 c)
{
  B32 result = !char_is_operator(c);
  return(result);
}

U32
str8_parse_u32_simple(String8 str, U64 *advance)
{
  Assert(advance);
  U32 value = 0;
  U64 idx = 0;
  for(;idx < str.size && char_is_digit(str.value[idx]); ++idx)
  {
    value = value*10 + (str.value[idx] - '0');
  }
  *advance = idx;
  return(value);
}

Token_List 
token_list_from_str(Arena *arena, String8 str, String8 *error)
{
  Token_List result = {0};
  
  // TODO(erb): no utf8 for now
  String8 it = str;
  for(;it.size > 0;)
  {
    // NOTE(erb): skip whitespaces
    Index_U64 no_blank_idx = str8_index_predicate(it, char_is_not_blank, 
                                                  IndexFlags_FallbackValueLength);
    if(no_blank_idx.exists)
    {
      it = str8_postfix(it, no_blank_idx.value);
    }
    
    if(it.size == 0)
    {
      break;
    }
    
    // NOTE(erb): this might leak the token if an error happens
    Token *token = PushStruct(arena, Token); 
    U64 skip_to_idx = 0;
    U8 c = it.value[0];
    if(char_is_identifier(c))
    {
      Index_U64 idx = str8_index_predicate(it, char_is_not_identifier, IndexFlags_FallbackValueLength);
      token->kind = TokenKind_Identifier;
      token->str8 = str8_prefix(it, idx.value);
      skip_to_idx = idx.value;
      
      token->is_postfix = (no_blank_idx.value == 0 &&
                           result.last != 0 &&
                           result.last->kind == TokenKind_NumLiteral);
    }
    else if(char_is_operator(c))
    {
      Index_U64 idx = str8_index_predicate(it, char_is_not_operator, IndexFlags_FallbackValueLength);
      token->kind = TokenKind_Operator;
      token->str8 = str8_prefix(it, idx.value);
      skip_to_idx = idx.value;
    }
    else if(c == '"')
    {
      it = str8_postfix(it, 1);
      Index_U64 idx = str8_index(it, '"', IndexFlags_FallbackValueLength);
      token->kind = TokenKind_StrLiteral;
      token->str8 = str8_prefix(it, idx.value);
      skip_to_idx = idx.value+1;
    }
    else if(c == '(' || c == ')')
    {
      token->kind = (c == '(') ? TokenKind_ParenOpen : TokenKind_ParenClose;
      token->str8 = str8_prefix(it, 1);
      skip_to_idx = 1;
    }
    else if(c == '{' || c == '}')
    {
      token->kind = (c == '{') ? TokenKind_ScopeOpen : TokenKind_ScopeClose;
      token->str8 = str8_prefix(it, 1);
      skip_to_idx = 1;
    }
    else if(char_is_digit(c))
    {
      U64 idx = 0;
      U32 value = str8_parse_u32_simple(it, &idx);
      Assert(idx >= 1);
      token->kind = TokenKind_NumLiteral;
      token->u32 = value;
      token->str8 = str8_prefix(it, idx);
      skip_to_idx = idx;
    }
    else
    {
      *error = S8("invalid token char");
      InvalidCodePath;
      break;
    }
    
    SLLQueueAppend(result.first, result.last, token);
    it = str8_postfix(it, skip_to_idx);
  }
  return(result);
}

/////////////////////////////////////////////////////////////
// NOTE(erb): helpers

void
token_it_push_current_location(Token_Iterator *it)
{
  if(it->location_count+1 < ArraySize(it->locations))
  {
    it->locations[it->location_count++] = it->current;
  }
  else InvalidCodePath;
}

Token *
token_it_set_current(Token_Iterator *it, Token *token)
{
  Token *other = it->current;
  it->current = token;
  return(other);
}

Token *
token_it_pop_or_restore_last_location(Token_Iterator *it, B32 restore)
{
  Token *current = it->current;
  if(it->location_count > 0)
  {
    if(restore)
    {
      token_it_set_current(it, it->locations[it->location_count-1]);
    }
    it->location_count--;
  }
  else InvalidCodePath;
  return(current);
}

Token *
token_it_peek(Token_Iterator *it)
{
  Token *token = it->current;
  return(token);
}

B32
token_it_peek_has_kind(Token_Iterator *it, Token_Kind kind)
{
  Token *peek = token_it_peek(it);
  B32 result = (peek != 0 && peek->kind == kind);
  return(result);
}

B32
token_it_peek_is_iden_str8(Token_Iterator *it, String8 str)
{
  Token *peek = token_it_peek(it);
  B32 result = (peek != 0 && peek->kind == TokenKind_Identifier && str8_equals(peek->str8, str));
  return(result);
}


Token *
token_it_next(Token_Iterator *it)
{
  Token *result = it->current;
  if(result != 0)
  {
    SLLStackPop(it->current);
  }
  return(result);
}

Token *
token_it_next_if_kind(Token_Iterator *it, Token_Kind kind)
{
  Token *result = 0;
  if(token_it_peek_has_kind(it, kind))
  {
    result = token_it_next(it);
  }
  return(result);
}

Token *
token_it_next_if_op_str8(Token_Iterator *it, String8 str)
{
  Token *result = 0;
  Token *peek = token_it_peek(it);
  if(peek != 0 && 
     peek->kind == TokenKind_Operator && 
     str8_equals(peek->str8, str))
  {
    result = token_it_next(it);
  }
  return(result);
}

Token *
token_it_next_if_iden_str8(Token_Iterator *it, String8 str)
{
  Token *result = 0;
  Token *peek = token_it_peek(it);
  if(peek != 0 && 
     peek->kind == TokenKind_Identifier && 
     str8_equals(peek->str8, str))
  {
    result = token_it_next(it);
  }
  return(result);
}