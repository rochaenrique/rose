///////////////////////////////////////////////////////////
// NOTE(erb): expression evaluation

Expr_Value
expression_value_from_event_expression(Arena *arena, Expr *expr, Event *event, String8 *error)
{
  Scratch scratch = scratch_begin(&arena, 1);
  Expr_Value result = {0};
  if(expr == 0)
  {
    *error = S8("empty");
  }
  else
  {
    if(expr->left == 0 && expr->right == 0)
    {
      switch(expr->kind)
      {
        case ExprKind_Identifier:
        {
          if(str8_equals(expr->str8, S8("$")))
          {
            result.event = event;
            result.type = ExprType_Event;
          }
          else
          {
            *error = S8("unkown global identifier");
          }
        } break;
        
        case ExprKind_NumLiteral:
        {
          result.u32 = expr->u32;
          result.type = ExprType_U32;
        } break;
        
        case ExprKind_StrLiteral:
        {
          result.str8 = expr->str8;
          result.type = ExprType_Str8;
        } break;
        
        InvalidDefaultCase;
      }
    }
    else if(expr->left != 0 && expr->right != 0)
    {
      Assert(expression_kind_is_binary_op(expr->kind));
      if(expr->kind == ExprKind_BinaryAccess)
      {
        String8 right_iden = {0};
        if(expr->right->kind == ExprKind_Identifier)
        {
          right_iden = expr->right->str8;
        }
        else
        {
          *error = S8("expected an identifier at the right of '.'");
        }
        
        Expr_Value left = {0};
        if(error->value == 0)
        {
          left = expression_value_from_event_expression(scratch.arena, expr->left, event, error);
        }
        
        if(error->value == 0)
        {
          switch(left.type)
          {
            case ExprType_Event:
            {
              if(str8_equals(right_iden, S8("name")))
              {
                result.str8 = str8_from_name_chunk_list(arena, &left.event->title_chunk_list);
                result.type = ExprType_Str8;
              }
              else if(str8_equals(right_iden, S8("start")))
              {
                result.dt = left.event->start;
                result.type = ExprType_DateTime;
              }
              else if(str8_equals(right_iden, S8("end")))
              {
                result.dt = left.event->end;
                result.type = ExprType_DateTime;
              }
              else if(str8_equals(right_iden, S8("id")))
              {
                result.u32 = hash_from_event_id(left.event->id);
                result.type = ExprType_U32;
              }
              else
              {
                *error = S8("unknown event access");
              }
            } break;
            
            case ExprType_DateTime:
            {
              if(str8_equals(right_iden, S8("day")))
              {
                result.u32 = left.dt.date.day;
                result.type = ExprType_U32;
              }
              else if(str8_equals(right_iden, S8("year")))
              {
                result.u32 = left.dt.date.year;
                result.type = ExprType_U32;
              }
              else if(str8_equals(right_iden, S8("month")))
              {
                result.u32 = left.dt.date.month;
                result.type = ExprType_U32;
              }
              else
              {
                *error = S8("unkown date access");
              }
            } break;
            
            default:
            {
              *error = S8("only accessible types are event and date");
            } break;
          }
        }
      }
      else
      {
        Expr_Value left = expression_value_from_event_expression(scratch.arena, expr->left, event, error);
        Expr_Value right = expression_value_from_event_expression(scratch.arena, expr->right, event, error);
        if(error->value == 0)
        {
          if(left.type == right.type)
          {
            B32 operands_type_valid = false;
            switch(expr->kind)
            {
              case ExprKind_BinarySub:
              {
                operands_type_valid = (left.type == right.type && 
                                       (left.type == ExprType_DateTime || left.type == ExprType_U32));
                result.type = left.type;
              } break;
              
              case ExprKind_BinaryMul:
              case ExprKind_BinaryMod:
              case ExprKind_BinaryDiv:
              case ExprKind_BinarySum:
              case ExprKind_BinaryBitAnd:
              case ExprKind_BinaryBitOr:
              {
                operands_type_valid = (left.type == right.type && 
                                       left.type == ExprType_U32); 
                result.type = ExprType_U32;
              } break;
              
              case ExprKind_BinaryEq:
              case ExprKind_BinaryGt: case ExprKind_BinaryGe: 
              case ExprKind_BinaryLt: case ExprKind_BinaryLe:
              {
                operands_type_valid = (left.type == right.type && 
                                       (left.type == ExprType_DateTime || left.type == ExprType_U32));
                result.type = ExprType_B32;
              } break;
              
              case ExprKind_BinaryLogicOr:
              case ExprKind_BinaryLogicAnd:
              {
                operands_type_valid = (left.type == right.type &&
                                       (left.type == ExprType_Str8 || left.type == ExprType_B32 || left.type == ExprType_U32));
                result.type = (left.type == ExprType_Str8) ? ExprType_Str8 : ExprType_B32;
              } break;
              
              InvalidDefaultCase;
            }
            
            if(operands_type_valid)
            {
              switch(result.type)
              {
                case ExprType_U32:
                case ExprType_B32:
                {
                  switch(expr->kind)
                  {
                    case ExprKind_BinaryMod: result.u32 = left.u32 % right.u32; break;
                    case ExprKind_BinaryMul: result.u32 = left.u32 * right.u32; break;
                    case ExprKind_BinaryDiv: result.u32 = left.u32 / right.u32; break;
                    case ExprKind_BinarySum: result.u32 = left.u32 + right.u32; break;
                    case ExprKind_BinarySub: result.u32 = left.u32 - right.u32; break;
                    
                    case ExprKind_BinaryBitOr : result.u32 = left.u32 | right.u32; break;
                    case ExprKind_BinaryBitAnd: result.u32 = left.u32 & right.u32; break;
                    
                    // NOTE(erb): implicit cast result
                    case ExprKind_BinaryEq: result.b32 = left.u32 == right.u32; break;
                    case ExprKind_BinaryGt: result.b32 = left.u32 >  right.u32; break;
                    case ExprKind_BinaryGe: result.b32 = left.u32 >= right.u32; break;
                    case ExprKind_BinaryLt: result.b32 = left.u32 <  right.u32; break;
                    case ExprKind_BinaryLe: result.b32 = left.u32 <= right.u32; break;
                    
                    case ExprKind_BinaryLogicAnd: result.b32 = left.b32 && right.b32; break;
                    case ExprKind_BinaryLogicOr: result.b32 = left.b32 || right.b32; break;
                    
                    InvalidDefaultCase;
                  }
                } break;
                
                case ExprType_Str8:
                {
                  switch(expr->kind)
                  {
                    case ExprKind_BinaryLogicAnd: 
                    {
                      result.str8 = push_str8_copy(arena, str8_longest_common_substring(left.str8, right.str8)); 
                    } break;
                    case ExprKind_BinaryLogicOr: 
                    {
                      result.str8 = str8_concat_no_postifx_overlap(arena, left.str8, right.str8); 
                    } break;
                    InvalidDefaultCase;
                  }
                } break;
                
                case ExprType_DateTime:
                {
                  I32 diff = (cal_date_to_days(left.dt.date) - cal_date_to_days(right.dt.date));
                  switch(expr->kind)
                  {
                    case ExprKind_BinaryEq: result.b32 = diff == 0; break;
                    case ExprKind_BinaryGt: result.b32 = diff >  0; break;
                    case ExprKind_BinaryGe: result.b32 = diff >= 0; break;
                    case ExprKind_BinaryLt: result.b32 = diff <  0; break;
                    case ExprKind_BinaryLe: result.b32 = diff <= 0; break;
                    
                    case ExprKind_BinarySub: 
                    {
                      // NOTE(erb): autocast to date
                      if(diff != 0)
                      {
                        result.dt.date = cal_date_from_days(diff);
                      }
                    } break;
                    default:
                    {
                      *error = S8("invalid operands for date");
                    } break;
                  }
                } break;
              }
            }
            else
            {
              *error = S8("invalid operands for type");
            }
          }
          else
          {
            *error = S8("invalid operand between types");
          }
        }
      }
    }
    else InvalidCodePath;
  }
  scratch_end(&scratch);
  return(result);
}

Expr_Value
expression_value_from_str8(Arena *arena, String8 expr_str, Event *event, String8 *error)
{
  Scratch scratch = scratch_begin(&arena, 1);
  Token_List tokens = token_list_from_str(scratch.arena, expr_str, error);
  
  Expr *expr = 0;
  if(error->value == 0)
  {
    Token_Iterator token_it = {0};
    token_it.current = tokens.first;
    expr = expression_from_eat_token_list(scratch.arena, &token_it, error);
  }
  
  Expr_Value value = {0};
  if(error->value == 0)
  {
    value = expression_value_from_event_expression(arena, expr, event, error);
  }
  scratch_end(&scratch);
  return(value);
}

///////////////////////////////////////////////////////////
// NOTE(erb): date "expression" parsing

Date_Lit_Result
token_it_next_date_literal(Token_Iterator *it)
{
  B32 failed = false;
  Token *num1 = token_it_next_if_kind(it, TokenKind_NumLiteral);
  failed = (num1 == 0);
  
  Token *delim1 = 0;
  if(!failed)
  {
    delim1 = token_it_next_if_op_str8(it, S8("/"));
    if(delim1 == 0)
    {
      delim1 = token_it_next_if_op_str8(it, S8("-"));
    }
    failed = (delim1 == 0);
  }
  
  Token *num2 = 0;
  if(!failed)
  {
    num2 = token_it_next_if_kind(it, TokenKind_NumLiteral);
    failed = (num2 == 0);
  }
  
  Token *delim2 = 0;
  if(!failed)
  {
    delim2 = token_it_next_if_op_str8(it, S8("/"));
    if(delim2 == 0)
    {
      delim2 = token_it_next_if_op_str8(it, S8("-"));
    }
    failed = (delim1 == 0);
  }
  
  
  Date_Lit_Result result = {0};
  if(!failed)
  {
    result.success = true;
    result.date.day = num1->u32;
    result.date.month = num2->u32;
    
    Token *num3 = token_it_next_if_kind(it, TokenKind_NumLiteral);
    if(num3 != 0)
    {
      result.date.year = num3->u32;
      result.has_year = true;
    }
  }
  return(result);
}

typedef struct Tagged_String8 Tagged_String8;
struct Tagged_String8
{
  String8 str;
  I32 i32[2];
};

typedef struct Lev_Match_Result Lev_Match_Result;
struct Lev_Match_Result
{
  U32 dist;
  U32 idx;
};

Lev_Match_Result
find_closest_match(Tagged_String8 *array, U32 size, String8 needle)
{
  Lev_Match_Result result = {0};
  result.dist = u32_max;
  for(U32Each(idx, size))
  {
    Tagged_String8 *test = (array + idx);
    U32 lev_dist = str8_lev_distance(test->str, needle);
    if(lev_dist < result.dist)
    {
      result.idx = idx;
      result.dist = lev_dist;
      if(lev_dist == 0)
      {
        break;
      }
    }
  }
  return(result);
}

Index_U32
token_it_next_closest_match(Token_Iterator *it, Tagged_String8 *haystack,
                            U32 haystack_size, U32 max_dist)
{
  Index_U32 result = {0};
  if(token_it_peek_has_kind(it, TokenKind_Identifier))
  {
    Lev_Match_Result match = find_closest_match(haystack, 
                                                haystack_size, 
                                                token_it_peek(it)->str8);
    if(match.dist <= max_dist)
    {
      token_it_next(it);
      result = idx_u32_true(match.idx);
    }
  }
  return(result);
}

B32
token_it_try_next_month(Token_Iterator *it, Calendar_Month *month_out)
{
  Assert(month_out != 0);
  
#define MONTH(s, v) { .str = S8Lit(s), .i32[0] = CalendarMonth_##v, }
  Tagged_String8 month_strs[] =
  {
    MONTH("jan", January),   MONTH("january",   January),
    MONTH("feb", February),  MONTH("february",  February),
    MONTH("mar", March),     MONTH("march",     March),
    MONTH("apr", April),     MONTH("april",     April),
    MONTH("may", May),       MONTH("may",       May),
    MONTH("jun", June),      MONTH("june",      June),
    MONTH("jul", July),      MONTH("july",      July),
    MONTH("aug", August),    MONTH("august",    August),
    MONTH("sep", September), MONTH("september", September),
    MONTH("oct", October),   MONTH("october",   October),
    MONTH("nov", November),  MONTH("november",  November),
    MONTH("dec", December),  MONTH("december",  December),
  };
#undef MONTH
  B32 result = false;
  Index_U32 match = token_it_next_closest_match(it, month_strs, ArraySize(month_strs), 2);
  if(match.exists)
  {
    *month_out = (Calendar_Month)month_strs[match.value].i32[0];
    result = true;
  }
  return(result);
}

B32
token_it_try_next_weekday(Token_Iterator *it, Calendar_Week_Day *weekday_out)
{
#define WEEKDAY(s, v) { .str = S8Lit(s), .i32[0] = CalendarWeekDay_##v, }
  Tagged_String8 weekday_strs[] =
  {
    WEEKDAY("sun", Sunday),    WEEKDAY("sunday",    Sunday),
    WEEKDAY("mon", Monday),    WEEKDAY("monday",    Monday),
    WEEKDAY("tue", Tuesday),   WEEKDAY("tuesday",   Tuesday),
    WEEKDAY("wed", Wednesday), WEEKDAY("wednesday", Wednesday),
    WEEKDAY("thu", Thursday),  WEEKDAY("thursday",  Thursday),
    WEEKDAY("fri", Friday),    WEEKDAY("friday",    Friday),
    WEEKDAY("sat", Saturday),  WEEKDAY("saturday",  Saturday),
  };
#undef WEEKDAY
  
  B32 result = false;
  Index_U32 match = token_it_next_closest_match(it, weekday_strs, ArraySize(weekday_strs), 2);
  if(match.exists)
  {
    *weekday_out = (Calendar_Week_Day)weekday_strs[match.value].i32[0];
    result = true;
  }
  return(result);
}

B32
token_it_try_next_num_optional_postfix(Token_Iterator *it, U32 *out_num)
{
  Assert(out_num != 0);
  
  Token *token = token_it_next_if_kind(it, TokenKind_NumLiteral);
  B32 result = (token != 0);
  
  // NOTE(erb): postifx
  if(token != 0)
  {
    token_it_push_current_location(it);
    
    Token *postfix_iden = token_it_next_if_kind(it, TokenKind_Identifier);
    if(postfix_iden != 0)
    {
      String8 expected_postfix = {0};
      U32 last_two_digits = (token->u32 % 100);
      if(last_two_digits == 11 || 
         last_two_digits == 12 || 
         last_two_digits == 13)
      {
        expected_postfix = S8("th");
      }
      else
      {
        U32 last_digit = (token->u32 % 10);
        switch(last_digit)
        {
          case  1: expected_postfix = S8("st"); break;
          case  2: expected_postfix = S8("nd"); break;
          case  3: expected_postfix = S8("rd"); break;
          default: expected_postfix = S8("th"); break;
        }
      }
      
      B32 failed = !str8_equals(postfix_iden->str8, expected_postfix);
      token_it_pop_or_restore_last_location(it, failed);
    }
  }
  
  // TODO(erb): validate the day
  if(result)
  {
    *out_num = token->u32;
  }
  return(result);
}

B32
token_it_try_next_num_or_num_word(Token_Iterator *it, U32 *out_num)
{
  B32 result = false;
  Token *num = token_it_next_if_kind(it, TokenKind_NumLiteral);
  if(num != 0)
  {
    *out_num = num->u32;
    result = true;
  }
  else
  {
#define NUM(s, n) { .str = S8Lit(s), .i32[0] = n, }
    Tagged_String8 num_words_strs[] = 
    {
      NUM("a",         1),
      NUM("one",       1), NUM("two",       2),
      NUM("three",     3), NUM("four",      4),
      NUM("five",      5), NUM("six",       6),
      NUM("seven",     7), NUM("eight",     8),
      NUM("nine",      9), NUM("ten",      10),
      NUM("eleven",   11), NUM("twelve",   12),
      NUM("thirteen", 13), NUM("fourteen", 14),
      NUM("thirty",   30),
    };
#undef NUM
    Index_U32 match = token_it_next_closest_match(it, num_words_strs,
                                                  ArraySize(num_words_strs), 1);
    if(match.exists)
    {
      *out_num = (U32)num_words_strs[match.value].i32[0];
      result = true;
    }
  }
  return(result);
}

B32
token_it_try_next_duration_kind(Token_Iterator *it, Time_Duration_Kind *out_kind)
{
#define DUR(s, m) { .str = S8Lit(s), .i32[0] = TimeDurationKind_##m, }
  Tagged_String8 kind_strs[] = 
  {
    DUR("minute", Minute), DUR("minutes", Minute), DUR("mins", Minute), DUR("min", Minute), DUR("m", Minute),
    DUR("hour",   Hour),   DUR("hours",   Hour),   DUR("hrs",  Hour),   DUR("hr",  Hour),   DUR("h", Hour),
    DUR("day",    Day),    DUR("days",    Day),
    DUR("week",   Week),   DUR("weeks",   Week),
    DUR("month",  Month),  DUR("months",  Month),
    DUR("year",   Year),   DUR("years",   Year),
  };
#undef DUR
  
  Index_U32 match = token_it_next_closest_match(it, kind_strs, ArraySize(kind_strs), 3);
  B32 result = match.exists;
  if(match.exists)
  {
    *out_kind = (Time_Duration_Kind)kind_strs[match.value].i32[0];
  }
  return(result);
}

B32
token_it_try_next_duration_op(Token_Iterator *it, Time_Duration *out_op)
{
  // NOTE(erb): duration := [number=1] dur_kind
  // number = one | two | 1 | 2 | ...
  // metric = minutes | hours | days | weeks | months | ...
  
  // NOTE(erb): number
  token_it_push_current_location(it);
  U32 num = 0;
  B32 has_num = token_it_try_next_num_or_num_word(it, &num);
  token_it_pop_or_restore_last_location(it, !has_num);
  
  out_op->value = (has_num ? num : 1);
  
  // NOTE(erb): kind
  B32 result = token_it_try_next_duration_kind(it, &out_op->kind);
  return(result);
}

typedef struct Date_Time_Parse Date_Time_Parse;
struct Date_Time_Parse
{
  Date_Time date_time;
  B32 has_date;
  B32 has_time;
};

Date_Time_Parse
date_time_from_tokens(Token_Iterator *it, Calendar_Date ref_date, String8 *error)
{
  I32 ref_date_days = cal_date_to_days(ref_date);
  
  // NOTE(erb): duration := [number=1] dur_kind
  // number = one | two | 1 | 2 | ...
  // metric = minutes | hours | days | weeks | months | ...
  
  // NOTE(erb): prefix_op := duration direction 
  // direction = before | after | from
  // Examples:
  // - 1 month before
  // - day after
  // - 2 weeks from
  Time_Duration prefix_op = {0};
  B32 has_prefix_op = false;
  {
    B32 failed = false;
    token_it_push_current_location(it);
    
    // NOTE(erb): duration
    if(!failed)
    {
      failed = !token_it_try_next_duration_op(it, &prefix_op);
    }
    
    // NOTE(erb): direction
    B32 is_positive = true;
    if(!failed)
    {
#define DIR(s, v) { .str = S8Lit(s), .i32[0] = v, }
      Tagged_String8 dir_strs[] = 
      {
        DIR("from",   true),
        DIR("after",  true),
        DIR("before", false),
      };
#undef DIR
      Index_U32 match = token_it_next_closest_match(it, dir_strs,
                                                    ArraySize(dir_strs), 3);
      failed = !match.exists;
      if(match.exists)
      {
        is_positive = (dir_strs[match.value].i32[0]);
      }
    }
    
    token_it_pop_or_restore_last_location(it, failed);
    if(!failed)
    {
      has_prefix_op = true;
      if(!is_positive)
      {
        prefix_op.value = -prefix_op.value;
      }
    }
  }
  
  //////////////////////////////////////////////////////
  // NOTE(erb): base date parsing
  B32 has_base_date = false;
  Calendar_Date base_date = {0};
  String8 base_date_fmt = {0};
  {
    
    // NOTE(erb): certain := today | tomorrow | yesterday | date_literal
    // Examples:
    // - today
    // - tomorrow
    // - 02/18/1988
    // - 1988-02-18
    if(!has_base_date)
    {
      token_it_push_current_location(it);
      
      // NOTE(erb): try dd/mm/yy - dd/mm/yyyy - dd/mm
      Date_Lit_Result date_lit = token_it_next_date_literal(it);
      if(date_lit.success)
      {
        has_base_date = true;
        base_date = date_lit.date;
        if(date_lit.has_year && 
           digit_count_u64(base_date.year, 10) <= 2)
        {
          base_date.year += (ref_date.year/100) * 100;
        }
        else
        {
          base_date.year = ref_date.year;
        }
      }
      else
      {
        token_it_pop_or_restore_last_location(it, true);
        
#define D(s, v) { .str = S8Lit(s), .i32[0] = v, }
        Tagged_String8 certain_strs[] = 
        {
          D("yesterday", -1),
          D("today",      0),
          D("tomorrow",   1),
        };
#undef D
        Index_U32 match = token_it_next_closest_match(it, certain_strs, ArraySize(certain_strs), 3);
        if(match.exists)
        {
          has_base_date = true;
          I32 days = (ref_date_days + certain_strs[match.value].i32[0]);
          base_date = cal_date_from_days(days);
        }
      }
      
      if(has_base_date)
      {
        base_date_fmt = S8("certain := today | tomorrow | yesterday | date_literal");
      }
    }
    
    // NOTE(erb): month_day := month day_of_month [year]
    // Examples:
    // - march 14
    // - sep 1st
    // - february 18 1988
    // - february 18, 1988
    if(!has_base_date)
    {
      token_it_push_current_location(it);
      B32 failed = false;
      
      // NOTE(erb): month
      Calendar_Month month = 0;
      {
        failed = !token_it_try_next_month(it, &month);
      }
      
      // NOTE(erb): day
      U32 day = 0;
      if(!failed)
      {
        // TODO(erb): validate the day & prefix
        // TODO(erb): error for prefix correction
        failed = !token_it_try_next_num_optional_postfix(it, &day);
      }
      
      // NOTE(erb): year
      B32 has_year = false;
      I32 year = 0;
      if(!failed)
      {
        token_it_next_if_op_str8(it, S8(",")); // NOTE(erb): skip ','
        
        Token *year_token = token_it_next_if_kind(it, TokenKind_NumLiteral);
        if(year_token != 0)
        {
          year = year_token->u32;
          has_year = true;
        }
      }
      
      token_it_pop_or_restore_last_location(it, failed);
      if(!failed)
      {
        if(!has_year)
        {
          year = ref_date.year;
        }
        else if(digit_count_u64(year, 10) <= 2)
        {
          year += 100*(ref_date.year/100);
        }
        
        
        if(!is_day_of_month_valid(year, month, day))
        {
          // TODO(erb): signal error here
          InvalidCodePath;
        }
        base_date = cal_date(year, month, day); // NOTE(erb): default current year
        has_base_date = true;
        base_date_fmt = S8("month_day := month day_of_month [year]");
      }
    }
    
    // NOTE(erb): day_month := day_of_month month [year]
    // Examples: 
    // - 1st sep
    // - 14 march
    // - 18 february 1988
    if(!has_base_date)
    {
      token_it_push_current_location(it);
      B32 failed = false;
      
      // NOTE(erb): day
      U32 day = 0;
      if(!failed)
      {
        // TODO(erb): validate the day & prefix
        // TODO(erb): error for prefix correction
        failed = !token_it_try_next_num_optional_postfix(it, &day);
      }
      
      // NOTE(erb): month
      Calendar_Month month = 0;
      if(!failed)
      {
        failed = !token_it_try_next_month(it, &month);
      }
      
      // NOTE(erb): year
      B32 has_year = false;
      I32 year = 0;
      if(!failed)
      {
        token_it_next_if_op_str8(it, S8(",")); // NOTE(erb): skip ','
        
        Token *year_token = token_it_next_if_kind(it, TokenKind_NumLiteral);
        if(year_token != 0)
        {
          year = year_token->u32;
          has_year = true;
        }
      }
      
      token_it_pop_or_restore_last_location(it, failed);
      if(!failed)
      {
        if(!has_year)
        {
          year = ref_date.year;
        }
        else if(digit_count_u64(year, 10) <= 2)
        {
          year += 100*(ref_date.year/100);
        }
        
        if(!is_day_of_month_valid(year, month, day))
        {
          // TODO(erb): signal error here
          InvalidCodePath;
        }
        base_date = cal_date(year, month, day); // NOTE(erb): default current year
        has_base_date = true;
        base_date_fmt = S8("month_day := month day_of_month [year]");
      }
    }
    
    
    // NOTE(erb): reference_weekday := reference weekday [in month]
    // reference = relative | position
    // relative = this | leading | trailing | next | previous
    // position = first | 1st | second | 2nd | ... | last 
    // Examples:
    // - first monday
    // - second tuesday
    // - last sunday in october
    // - next monday
    // - previous tuesday
    // - next monday in march
    // - 1st sat
    if(!has_base_date)
    {
      B32 failed = false;
      token_it_push_current_location(it);
      
      // NOTE(erb): reference
      typedef enum Reference_Kind
      {
        ReferenceKind_This,
        ReferenceKind_Leading,
        ReferenceKind_Trailing,
        ReferenceKind_Next,
        ReferenceKind_Previous,
        ReferenceKind_Position,
        ReferenceKind_Last,
      } Reference_Kind;
      
      I32 position = 0;
      Reference_Kind kind = ReferenceKind_This;
      if(!failed)
      {
        // NOTE(erb): relative
        {
#define R(s, v) { .str = S8Lit(s), .i32[0] = v, }
          Tagged_String8 relative_strs[] = 
          {
            R("this",  ReferenceKind_This),
            R("last",  ReferenceKind_Last),
            R("next",  ReferenceKind_Next),
            R("prev",  ReferenceKind_Previous), R("previous", ReferenceKind_Previous), 
            R("trail", ReferenceKind_Trailing), R("trailing", ReferenceKind_Trailing), 
            R("lead",  ReferenceKind_Leading),  R("leading",  ReferenceKind_Leading),  
          };
#undef R
          Index_U32 match = token_it_next_closest_match(it, relative_strs, ArraySize(relative_strs), 2);
          failed = !match.exists;
          if(match.exists)
          {
            kind = (Reference_Kind)relative_strs[match.value].i32[0];
          }
        }
        
        // NOTE(erb): position
        if(failed)
        {
          // TODO(erb): validate postfix
          // TODO(erb): error for prefix correction
          U32 test_pos = 0;
          if(token_it_try_next_num_optional_postfix(it, &test_pos))
          {
            position = test_pos;
            kind = ReferenceKind_Position;
          }
          else
          {
            failed = true;
          }
          
          if(failed)
          {
            // NOTE(erb): first, second, third ...
#define R(s, v) { .str = S8Lit(s), .i32[0] = v, }
            Tagged_String8 position_strs[] = 
            {
              R("first",   1), R("second",  2),
              R("third",   3), R("fourth",  4),
            };
#undef R
            Index_U32 match = token_it_next_closest_match(it, position_strs, ArraySize(position_strs), 2);
            failed = !match.exists;
            if(match.exists)
            {
              position = position_strs[match.value].i32[0];
              kind = ReferenceKind_Position;
            }
            else
            {
              // TODO(erb): have some kind of error here for fifth, sixth, seventh, etc
            }
          }
        }
      }
      
      // NOTE(erb): weekday
      Calendar_Week_Day weekday = 0;
      if(!failed)
      {
        failed = !token_it_try_next_weekday(it, &weekday);
      }
      
      // NOTE(erb): [in month] - optional
      B32 has_month = false;
      Calendar_Month month = 0;
      if(!failed && (kind == ReferenceKind_Position || 
                     kind == ReferenceKind_Last))
      {
        if(token_it_next_if_iden_str8(it, S8("in")) != 0)
        {
          if(token_it_try_next_month(it, &month))
          {
            has_month = true;
          }
          else
          {
            failed = true;
          }
        }
      }
      
      token_it_pop_or_restore_last_location(it, failed);
      if(!failed)
      {
        switch(kind)
        {
          case ReferenceKind_This: 
          {
            // NOTE(erb): weekday within the current week regardless of the day
            Calendar_Week_Day current_weekday = weekday_from_days(ref_date_days);
            I32 diff = (I32)weekday - (I32)current_weekday;
            base_date = cal_date_from_days(ref_date_days + diff);
          } break;
          
          case ReferenceKind_Trailing: 
          {
            // NOTE(erb): nearest previous occurence, excluding today
            Calendar_Week_Day current_weekday = weekday_from_days(ref_date_days);
            I32 diff = (weekday == current_weekday ? -7 : 
                        -(I32)weekday_difference(current_weekday, weekday));
            base_date = cal_date_from_days(ref_date_days + diff);
          } break;
          
          case ReferenceKind_Leading: 
          {
            // NOTE(erb): nearest next occurence, excluding today
            Calendar_Week_Day current_weekday = weekday_from_days(ref_date_days);
            I32 diff = (weekday == current_weekday ? 7 : 
                        (I32)weekday_difference(current_weekday, weekday));
            base_date = cal_date_from_days(ref_date_days + diff);
          } break;
          
          case ReferenceKind_Next: 
          case ReferenceKind_Previous: 
          {
            // NOTE(erb): this ignores the month with [in month]
            Calendar_Week_Day current_weekday = weekday_from_days(ref_date_days);
            I32 diff = (I32)weekday - (I32)current_weekday;
            diff += (kind == ReferenceKind_Next ? 7 : -7);
            base_date = cal_date_from_days(ref_date_days + diff);
          } break;
          
          case ReferenceKind_Position: 
          case ReferenceKind_Last: 
          {
            I32 year = ref_date.year;
            if(!has_month)
            {
              month = ref_date.month;
            }
            if(kind == ReferenceKind_Position)
            {
              base_date = cal_date_nth_weekday_month_year(position, weekday, month, year);
            }
            else
            {
              // TODO(erb): error here in case of asking for '5th' and there is only a '4th'
              base_date = cal_date_last_weekday_month_year(weekday, month, year);
            }
          } break;
          
          InvalidDefaultCase;
        }
        has_base_date = true;
      }
      
      if(has_base_date)
      {
        base_date_fmt = S8("reference_weekday := reference weekday [in month]");
      }
    }
  }
  
  // NOTE(erb): trailing_op := op_kind duration
  // op_kind = + | plus | - | minus
  // Examples:
  // - + 9 days
  // - plus 2 weeks
  // - - 1 month
  // - minus 3 days
  B32 has_trailing_op = false;
  Time_Duration trailing_op = {0};
  {
    B32 failed = false;
    token_it_push_current_location(it);
    
    // NOTE(erb): operator
    B32 is_sum = false;
    {
      // NOTE(erb): try '+' '-'
      Token *op_token = token_it_next_if_kind(it, TokenKind_Operator);
      if(op_token != 0)
      {
        if(str8_equals(op_token->str8, S8("+")))
        {
          is_sum = true;
        }
        else if(str8_equals(op_token->str8, S8("-")))
        {
          is_sum = false;
        }
        else
        {
          failed = true;
        }
      }
      else
      {
#define OP(s, v) { .str = S8Lit(s), .i32[0] = v, }
        Tagged_String8 op_strs[] =
        {
          OP("sum",   true),
          OP("plus",  true),
          OP("sub",   false),
          OP("minus", false),
        };
#undef OP
        Index_U32 match = token_it_next_closest_match(it, op_strs, ArraySize(op_strs), 1);
        failed = !match.exists;
        if(match.exists)
        {
          is_sum = op_strs[match.value].i32[0];
        }
      }
    }
    
    // NOTE(erb): duration
    if(!failed)
    {
      failed = !token_it_try_next_duration_op(it, &trailing_op);
    }
    
    token_it_pop_or_restore_last_location(it, failed);
    if(!failed)
    {
      has_trailing_op = true;
      if(!is_sum)
      {
        trailing_op.value = -trailing_op.value;
      }
    }
  }
  
  // NOTE(erb): Complete examples
  // - march 14                          -> month_day
  // - next friday                       -> relative_weekday
  // - today + 9 days                    -> certain + trailing_op
  // - next monday in march plus 2 weeks -> relative_weekday + trailing_op
  // - 1 week before sep 1st             -> prefix_op_expr
  // - 3 weeks from today                -> prefix_op_expr
  // - one month after last friday       -> prefix_op_expr + relative_weekday
  // - 1st sep weekend + a week          -> weekend_expr + trailing_op
  
  
  //////////////////////////////////////////////////////////////////////
  // NOTE(erb): parse time at the end
  B32 has_time = false;
  Clock_Time time = {0};
  {
    B32 failed = false;
    token_it_push_current_location(it);
    token_it_next_if_iden_str8(it, S8("at")); // NOTE(erb): skip 'at'
    
    // NOTE(erb): first value
    Token *num1 = token_it_next_if_kind(it, TokenKind_NumLiteral);
    failed = (num1 == 0 || num1->u32 > 23);
    
    // NOTE(erb): delim
    Token *num2 = 0;
    if(!failed)
    {
      // NOTE(erb): 'h' postifx
      token_it_push_current_location(it);
      Token *delim_token = token_it_peek(it);
      if(delim_token != 0)
      {
        B32 has_delim_num2 = (str8_equals(delim_token->str8, S8(":")) || 
                              str8_equals(delim_token->str8, S8("h")));
        if(has_delim_num2)
        {
          token_it_next(it);
          num2 = token_it_next_if_kind(it, TokenKind_NumLiteral);
          failed = (num2 == 0);
        }
      }
      token_it_pop_or_restore_last_location(it, failed);
    }
    
    B32 is_am = true;
    if(!failed)
    {
      token_it_push_current_location(it);
      Token *am_pm_token = token_it_next_if_kind(it, TokenKind_Identifier);
      if(am_pm_token != 0)
      {
        B32 has_am_or_pm = true;
        if(str8_equals(am_pm_token->str8, S8("am")))
        {
          is_am = true;
        }
        else if(str8_equals(am_pm_token->str8, S8("pm")))
        {
          is_am = false;
        }
        else
        {
          has_am_or_pm = false;
        }
        
        token_it_pop_or_restore_last_location(it, !has_am_or_pm);
      }
    }
    
    if(!failed)
    {
      has_time = true;
      time.hour = (is_am ? num1->u32 : num1->u32+12);
      if(num2 != 0)
      {
        time.minute = num2->u32;
      }
    }
    else
    {
      token_it_push_current_location(it);
    }
  }
  
  
  //////////////////////////////////////////////////////////////////////
  // NOTE(erb): calculation
  Date_Time_Parse result = {0};
  
  result.has_time = has_time;
  if(has_time)
  {
    result.date_time.time = time;
  }
  
  result.has_date = has_base_date;
  if(has_base_date)
  {
    result.date_time.date = base_date;
    if(has_prefix_op)
    {
      result.date_time = date_time_add_duration(result.date_time, prefix_op);
    }
    if(has_trailing_op)
    {
      result.date_time = date_time_add_duration(result.date_time, trailing_op);
    }
  }
  else
  {
    *error = S8("no_base_date");
  }
  return(result);
}

void
update_date_time_range_from_tokens(Token_Iterator *it, Calendar_Date ref_date, Date_Time *start, Date_Time *end, String8 *error)
{
  Scratch scratch = scratch_begin(0, 0);
  
  // NOTE(erb): date_range := min_and_max | start_and_duration
  // min_and_max = [from] date_time (to|until|-) date_time
  // start_and_duration = date_time (for|during) duration1 duration2 ...
  // Examples:
  // - feb 18, 1988 - feb 29, 2024
  // - from tomorrow to last week of august
  // - next monday for 1 week
  
  // NOTE(erb): [from]
  B32 is_min_max_range = false;
  if(error->value == 0)
  {
    is_min_max_range = (token_it_next_if_iden_str8(it, S8("from")) != 0);
  }
  
  // NOTE(erb): min date time
  Date_Time_Parse min_dt_parse = {0};
  if(error->value == 0)
  {
    min_dt_parse = date_time_from_tokens(it, ref_date, error);
  }
  
  // NOTE(erb): duration connector
  B32 has_duration = false;
  if(error->value == 0 && !is_min_max_range)
  {
#define DUR(s) { .str = S8Lit(s), }
    Tagged_String8 duration_word_strs[] = 
    {
      DUR("for"),
      DUR("during"),
    };
#undef DUR
    Index_U32 match = token_it_next_closest_match(it, duration_word_strs, ArraySize(duration_word_strs), 1);
    has_duration = match.exists;
  };
  
  // NOTE(erb): duration
  typedef struct Time_Duration_Node Time_Duration_Node;
  struct Time_Duration_Node
  {
    Time_Duration op;
    Time_Duration_Node *next;
  };
  
  Time_Duration_Node *first_duration_op = 0;
  if(error->value == 0 && has_duration)
  {
    for(;;)
    {
      if(first_duration_op != 0)
      {
        token_it_next_if_op_str8(it, S8("+"));
      }
      
      token_it_push_current_location(it);
      Time_Duration op = {0};
      B32 has_op = token_it_try_next_duration_op(it, &op);
      token_it_pop_or_restore_last_location(it, !has_op);
      if(has_op)
      {
        Time_Duration_Node *node = PushStruct(scratch.arena, Time_Duration_Node);
        node->op = op;
        SLLStackPush(first_duration_op, node);
      }
      else
      {
        break;
      }
    }
    
    if(first_duration_op == 0)
    {
      *error = S8("expected duration");
    }
  }
  
  // NOTE(erb): range conector (for|until|-)
  if(error->value == 0)
  {
#define C(s) { .str = S8Lit(s), }
    Tagged_String8 conector_strs[] = 
    {
      C("to"),
      C("until"),
      C("-"),
    };
#undef C
    Index_U32 match = token_it_next_closest_match(it, conector_strs, ArraySize(conector_strs), 1);
    if(!is_min_max_range && match.exists)
    {
      is_min_max_range = true;
    }
    else if(is_min_max_range && !match.exists)
    {
      *error = S8("expected for, until or '-' (dash)");
    }
  }
  
  // NOTE(erb): max date time
  Date_Time_Parse max_dt_parse = {0};
  if(error->value == 0 && is_min_max_range)
  {
    max_dt_parse = date_time_from_tokens(it, ref_date, error);
  }
  
  if(error->value == 0)
  {
    if(min_dt_parse.has_date)
    {
      start->date = min_dt_parse.date_time.date;
    }
    if(min_dt_parse.has_time)
    {
      start->time = min_dt_parse.date_time.time;
    }
    
    if(is_min_max_range)
    {
      if(max_dt_parse.has_date)
      {
        end->date = max_dt_parse.date_time.date;
      }
      if(max_dt_parse.has_time)
      {
        end->time = max_dt_parse.date_time.time;
      }
      
      if(!max_dt_parse.has_time && min_dt_parse.has_time)
      {
        end->time = min_dt_parse.date_time.time;
      }
      else if(max_dt_parse.has_time && !min_dt_parse.has_time)
      {
        start->time = max_dt_parse.date_time.time;
      }
    }
    else
    {
      *end = *start;
      for(EachNode(Time_Duration_Node, node, first_duration_op))
      {
        *end = date_time_add_duration(*end, node->op);
      }
    }
  }
  
  scratch_end(&scratch);
}
