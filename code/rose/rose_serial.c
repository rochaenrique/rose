B32
token_it_try_next_key(Token_Iterator *it, String8 *out_str)
{
  B32 result = false;
  token_it_push_current_location(it);
  
  Token *iden = token_it_next_if_kind(it, TokenKind_Identifier);
  if(iden != 0)
  {
    Token *colon = token_it_next_if_op_str8(it, S8(":"));
    if(colon != 0)
    {
      *out_str = iden->str8;
      result = true;
    }
  }
  token_it_pop_or_restore_last_location(it, !result);
  return(result);
}

Calendar_Date
srl_date_value(Token *value_token, String8 *error)
{
  if(error->value == 0 &&
     value_token->kind != TokenKind_StrLiteral)
  {
    *error = S8("expected date to be string literal identifier");
    InvalidCodePath;
  }
  
  Calendar_Date result = {0};
  
  if(error->value == 0)
  {
    Scratch scratch = scratch_begin(0, 0);
    Token_List date_tokens = token_list_from_str(scratch.arena, value_token->str8, error);
    
    Date_Lit_Result date_lit = {0};
    if(error->value == 0)
    {
      Token_Iterator date_token_it = {0};
      date_token_it.current = date_tokens.first;
      date_lit = token_it_next_date_literal(&date_token_it);
    }
    
    if(date_lit.success && date_lit.has_year)
    {
      result = date_lit.date;
    }
    else
    {
      *error = S8("invalid date format");
      InvalidCodePath;
    }
    
    scratch_end(&scratch);
  }
  return(result);
}

Clock_Time
srl_time_value(Token *value_token, String8 *error)
{
  Clock_Time result = {0};
  if(error->value == 0)
  {
    Scratch scratch = scratch_begin(0, 0);
    
    Token_List time_tokens = token_list_from_str(scratch.arena, value_token->str8, error);
    
    Token_Iterator time_token_it = {0};
    time_token_it.current = time_tokens.first;
    
    U32 hour = 0;
    if(error->value == 0)
    {
      Token *num = token_it_next_if_kind(&time_token_it, TokenKind_NumLiteral);
      if(num != 0)
      {
        hour = num->u32;
      }
      else
      {
        *error = S8("expected hour");
        InvalidCodePath;
      }
    }
    
    if(error->value == 0 &&
       hour > 23)
    {
      *error = S8("expected hour to be less than or equal to 23");
      InvalidCodePath;
    }
    
    if(error->value == 0)
    {
      Token *delim = token_it_next_if_op_str8(&time_token_it, S8(":"));
      if(delim == 0)
      {
        *error = S8("expected colon ':' time seperator");
        InvalidCodePath;
      }
    }
    
    U32 minute = 0;
    if(error->value == 0)
    {
      Token *num = token_it_next_if_kind(&time_token_it, TokenKind_NumLiteral);
      if(num != 0)
      {
        minute = num->u32;
      }
      else
      {
        *error = S8("expected minute");
        InvalidCodePath;
      }
    }
    
    if(error->value == 0 && 
       hour > 59)
    {
      *error = S8("expected hour to be less than or equal to 59");
      InvalidCodePath;
    }
    
    if(error->value == 0)
    {
      result.hour = hour;
      result.minute = minute;
    }
    else
    {
      *error = S8("invalid time format");
      InvalidCodePath;
    }
    
    scratch_end(&scratch);
  }
  return(result);
}

typedef struct SRL_Key_Value SRL_Key_Value;
struct SRL_Key_Value
{
  String8 key;
  Token *value_start;
};

SRL_Key_Value
srl_key_value(Token_Iterator *it, String8 *error)
{
  SRL_Key_Value result = {0};
  if(!token_it_try_next_key(it, &result.key))
  {
    *error = S8("unkown event key");
    InvalidCodePath;
  }
  result.value_start = token_it_next(it);
  if(error->value == 0 && result.value_start == 0)
  {
    *error = S8("expected value");
    InvalidCodePath;
  }
  return(result);
}

B32
srl_expect_object_begin(Token_Iterator *it, String8 *error)
{
  B32 result = token_it_next_if_kind(it, TokenKind_ScopeOpen) != 0;
  if(!result)
  {
    *error = S8("expected object");
    InvalidCodePath;
  }
  return(result);
}

B32
srl_is_object_end(Token_Iterator *it, String8 *error)
{
  B32 result = ((it->current == 0) || (error->value != 0) || 
                token_it_next_if_kind(it, TokenKind_ScopeClose) != 0);
  return(result);
}

B32
srl_object_it(Token_Iterator *it, String8 *error, SRL_Key_Value *kv)
{
  B32 result = false;
  if(!srl_is_object_end(it, error))
  {
    *kv = srl_key_value(it, error);
    result = (error->value == 0);
  }
  return(result);
}

Date_Time
srl_date_time_value(Token_Iterator *it, String8 *error)
{
  Date_Time result = {0};
  
  SRL_Key_Value kv = {0};
  for(srl_expect_object_begin(it, error);
      srl_object_it(it, error, &kv);)
  {
    if(str8_equals(kv.key, S8("date")))
    {
      result.date = srl_date_value(kv.value_start, error);
    }
    else if(str8_equals(kv.key, S8("time")))
    {
      result.time = srl_time_value(kv.value_start, error);
    }
    else
    {
      *error = S8("invalid key");
      InvalidCodePath;
    }
  }
  return(result);
}

SRL_Project_File 
srl_project_from_str(Arena *arena, String8 str)
{
  Scratch scratch = scratch_begin(0, 0);
  
  String8 error = {0};
  Token_List tokens_list = token_list_from_str(scratch.arena, str, &error);
  Token_Iterator it = {0};
  it.current = tokens_list.first;
  
  SRL_Project_File result = {0};
  if(error.value == 0)
  {
    for(;it.current != 0 && error.value == 0;)
    {
      String8 head_key = {0};
      if(!token_it_try_next_key(&it, &head_key))
      {
        break;
      }
      
      if(str8_equals(head_key, S8("global_id_counter")))
      {
        Token *num = token_it_next_if_kind(&it, TokenKind_NumLiteral);
        if(num != 0)
        {
          result.global_id_counter = num->u32;
        }
        else
        {
          error = S8("expected integer lgobal id counter");
          InvalidCodePath;
        }
      }
      else if(str8_equals(head_key, S8("event")))
      {
        SRL_Event event = {0};
        SRL_Key_Value kv = {0};
        
        for(srl_expect_object_begin(&it, &error);
            srl_object_it(&it, &error, &kv);)
        {
          if(str8_equals(kv.key, S8("name")))
          {
            if(error.value == 0 &&
               kv.value_start->kind != TokenKind_StrLiteral &&
               kv.value_start->kind != TokenKind_Identifier)
            {
              error = S8("expected name to be string literal or identifier");
              InvalidCodePath;
            }
            else
            {
              event.name = kv.value_start->str8;
            }
          }
          else if(str8_equals(kv.key, S8("id")))
          {
            if(error.value == 0 &&
               kv.value_start->kind != TokenKind_NumLiteral)
            {
              error = S8("expected id to be a num literal");
              InvalidCodePath;
            }
            else
            {
              event.id = kv.value_start->u32;
            }
          }
          else if(str8_equals(kv.key, S8("start")))
          {
            token_it_set_current(&it, kv.value_start);
            event.start = srl_date_time_value(&it, &error);
          }
          else if(str8_equals(kv.key, S8("end")))
          {
            token_it_set_current(&it, kv.value_start);
            event.end = srl_date_time_value(&it, &error);
          }
          else if(str8_equals(kv.key, S8("tag")))
          {
            if(error.value == 0 &&
               kv.value_start->kind != TokenKind_NumLiteral)
            {
              error = S8("expected tag to be a num literal");
              InvalidCodePath;
            }
            else
            {
              SRL_Tag_Ref *ref = PushStruct(arena, SRL_Tag_Ref);
              ref->id = kv.value_start->u32;
              SLLStackPush(event.first_tag, ref);
            }
          }
          else
          {
            error = S8("invalid event key");
            InvalidCodePath;
          }
        } 
        
        // TODO(erb): check missing values
        if(error.value == 0 &&
           event.name.value == 0)
        {
          error = S8("expected a name");
          InvalidCodePath;
        }
        
        if(error.value == 0)
        {
          SRL_Event *new_event = PushStruct(arena, SRL_Event);
          *new_event = event;
          SLLStackPush(result.first_event, new_event);
        }
      }
      else if(str8_equals(head_key, S8("tag")))
      {
        SRL_Tag tag = {0};
        SRL_Key_Value kv = {0};
        
        for(srl_expect_object_begin(&it, &error);
            srl_object_it(&it, &error, &kv);)
        {
          if(str8_equals(kv.key, S8("id")))
          {
            if(error.value == 0 &&
               kv.value_start->kind != TokenKind_NumLiteral)
            {
              error = S8("expected id to be number literal");
              InvalidCodePath;
            }
            else
            {
              tag.id = kv.value_start->u32;
            }
          }
          else if(str8_equals(kv.key, S8("name")))
          {
            if(error.value == 0 &&
               kv.value_start->kind != TokenKind_StrLiteral &&
               kv.value_start->kind != TokenKind_Identifier)
            {
              error = S8("expected name to be string literal or identifier");
              InvalidCodePath;
            }
            else
            {
              tag.name = kv.value_start->str8;
            }
          }
          else if(str8_equals(kv.key, S8("color")))
          {
            if(error.value == 0 &&
               kv.value_start->kind != TokenKind_StrLiteral)
            {
              error = S8("expected color to be string literal identifier");
              InvalidCodePath;
            }
            
            if(error.value == 0)
            {
              U32 value = 0;
              String8 hex = kv.value_start->str8;
              for(EachU32(idx, hex.size))
              {
                value *= 16;
                U8 c = hex.value[idx];
                U8 upper = char_to_upper(c);
                if(char_is_digit(c))
                {
                  value += (c - '0');
                }
                else if(upper >= 'A' && upper <= 'F')
                {
                  value += (upper - 'A') + 9;
                }
                else
                {
                  error = S8("invalid color hex format");
                  InvalidCodePath;
                  break;
                }
              }
              if(error.value == 0)
              {
                tag.color = rgba_from_packed_u32(value);
              }
            }
          }
          else
          {
            error = S8("invalid tag key");
            InvalidCodePath;
          }
        }
        
        // TODO(erb): check last scope
        
        if(error.value == 0 && 
           tag.name.size == 0)
        {
          error = S8("expected a tag name");
          InvalidCodePath;
        }
        
        if(error.value == 0)
        {
          SRL_Tag *new_tag = PushStruct(arena, SRL_Tag);
          *new_tag = tag;
          SLLStackPush(result.first_tag, new_tag);
        }
      }
      else
      {
        error = S8("undefined key");
        InvalidCodePath;
      }
    }
  }
  scratch_end(&scratch);
  
  result.global_id_counter = CapBottom(result.global_id_counter, 1);
  result.error = error;
  return(result);
}