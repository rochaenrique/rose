// NOTE(erb): headers
#include "base/base.h"
#include "img/img.h"
#include "gfx/gfx.h"
#include "fnt/fnt.h"
#include "gfx/q2d/q2d.h"
#include "app_bridge.h"

#include "ui/ui.h"
#include "token/token.h"
#include "expr/expr.h"
#include "rose/rose.h"

// NOTE(erb): implementation
#include "base/base.c"
#include "img/img.c"
#include "gfx/gfx.c"
#include "fnt/fnt.c"
#include "gfx/q2d/q2d.c"

#include "ui/ui.c"
#include "token/token.c"
#include "expr/expr.c"
#include "rose/rose.c"

void
render_ui_cmds(Q2D_State *render_state, UI_Draw_Cmd_List *draw_commands)
{
  for(EachNode(UI_Draw_Cmd, cmd, draw_commands->first))
  {
    switch(cmd->kind)
    {
      case UIDrawKind_Text:
      {
        Assert(cmd->text.font->height_px != 0);
        
        F32 scale = (cmd->text.font_size / cmd->text.font->height_px);
        F32 ascent = (cmd->text.font->data.ascent_em * cmd->text.font->data.scale_em_to_pixels * scale);
        V2f top_left = v2f(cmd->range.min.x, cmd->range.min.y + ascent);
        
        q2d_render_string_baseline_px(render_state, cmd->text.content, top_left, 
                                      cmd->text.font, cmd->text.font_size, cmd->text.blank_spacing, 
                                      cmd->text.char_spacing, cmd->text.color, true);
      } break;
      
      case UIDrawKind_Rect:
      case UIDrawKind_RectLines:
      {
        F32 edge_softness = lerp(0, 2, cmd->rect.corner_radius/150.0f);
        F32 border_thickness = (cmd->kind == UIDrawKind_RectLines ? cmd->rect.border_thickness : 0.0f);
        
        Quad_Colors colors = {0};
        if(cmd->kind == UIDrawKind_RectLines)
        {
          colors = quad_colors_single(cmd->color);
        }
        else
        {
          Orientation2 orient = cmd->background.orientation;
          V4f color0 = cmd->background.color0;
          V4f color1 = cmd->background.color1;
          switch(orient)
          {
            case Orientation2_NS:
            {
              colors.top_left = colors.top_right = color0;
              colors.bottom_left = colors.bottom_right = color1;
            } break;
            
            case Orientation2_WE:
            {
              colors.top_left = colors.bottom_left = color0;
              colors.top_right = colors.bottom_right = color1;
            } break;
            
            case Orientation2_NESW:
            {
              colors.top_right = color0;
              colors.bottom_left = color1;
              colors.top_left = colors.bottom_right = v4f_lerp(color0, color1, 0.5f);
            } break;
            
            case Orientation2_NWSE:
            {
              colors.top_left = color0;
              colors.bottom_right = color1;
              colors.top_right = colors.bottom_left = v4f_lerp(color0, color1, 0.5f);
            } break;
          }
        }
        q2d_render_rect_color_ex(render_state, cmd->range, colors, 
                                 cmd->rect.corner_radius, edge_softness, border_thickness);
      } break;
      
      case UIDrawKind_ClipBegin:
      {
        Range_V2i rect = rng_v2i_from_inclusive_rng_v2f(cmd->range);
        q2d_scissor_begin(render_state, rect);
      } break;
      
      case UIDrawKind_ClipEnd:
      {
        q2d_scissor_end(render_state);
      } break;
      
      InvalidDefaultCase;
    }
  }
}

void
save_state(App_State *app_state)
{
  String8 file_path = app_state->save_to_file_path;
  if(file_path.size > 0)
  {
    Scratch scratch = scratch_begin(0, 0);
    String8_Builder sb = str8_builder_make_default();
    
    str8_builderf(scratch.arena, &sb, "global_id_counter: %llu\n", app_state->global_id_counter);
    
    for(EachNode(Event, event, app_state->event_list.first))
    {
      Scratch title_scratch = scratch_begin(&scratch.arena, 1);
      str8_builderf(scratch.arena, &sb,
                    "event:\n"
                    "{\n"
                    "  id  : %llu\n"
                    "  name: \"%S\"\n"
                    "  start:\n"
                    "  {\n"
                    "    date: \"%02u/%02u/%u\"\n"
                    "    time: \"%02u:%02u\"\n"
                    "  }\n"
                    "  end:\n"
                    "  {\n"
                    "    date: \"%02u/%02u/%u\"\n"
                    "    time: \"%02u:%02u\"\n"
                    "  }\n"
                    , event->id,
                    str8_from_name_chunk_list(title_scratch.arena, &event->title_chunk_list),
                    event->start.date.day, event->start.date.month, event->start.date.year,
                    event->start.time.hour, event->start.time.minute, 
                    event->end.date.day, event->end.date.month, event->end.date.year,
                    event->end.time.hour, event->end.time.minute);
      
      for(U32Each(idx, event->tag_count))
      {
        str8_builderf(scratch.arena, &sb, "   tag: %llu\n", event->tags[idx]);
      }
      str8_builder_str(scratch.arena, &sb, S8("}\n"));
      
      scratch_end(&title_scratch);
    }
    
    for(EachElement(Event_Tag, tag, app_state->tags))
    {
      if(tag->used)
      {
        str8_builderf(scratch.arena, &sb, 
                      "tag:\n"
                      "{\n"
                      "  id   : %llu\n"
                      "  name : \"%S\"\n"
                      "  color: \"%x\"\n"
                      "}\n", 
                      id_from_tag(app_state, tag),
                      S8Struct(tag->name, tag->name_size), 
                      rgb_to_packed_u32(rgb_from_rgba(tag->color)));
      }
    }
    
    Assert(app_state->write_entire_file_str8 != 0);
    app_state->write_entire_file_str8(file_path, push_str8_from_builder(scratch.arena, &sb));
    scratch_end(&scratch);
  }
}

void
ui_calendar(UI_Ctx *ui, Calendar_Date start_date, V2i grid_size, Range_V2f space, Event_List *event_list)
{
  Scratch scratch = scratch_begin(0, 0);
  V2f space_size = rng_v2f_size(space);
  
  I32 color_space = (grid_size.y + grid_size.x);
  V2f cell_size = v2f_hada_div(space_size, v2fi(grid_size));
  V2f cell_cursor = space.min;
  for(U32Each(y, (U32)grid_size.y))
  {
    cell_cursor.x = space.min.x;
    for(U32Each(x, (U32)grid_size.x))
    {
      F32 t = (F32)(x+y)/color_space;
      V4f color = v4f_lerp(color_red, color_blue, t);
      
      U32 cell_day_offset = (U32)grid_size.x*y + x;
      Calendar_Date cell_date = cal_date_add_days(start_date, cell_day_offset);
      
      Range_V2f range = rng_v2f_point_add(cell_cursor, cell_size);
      
      // NOTE(erb): cell
      ui_set_next_rect(ui, range);
      ui_set_next_layout_axis(ui, Axis2_Y);
      ui_set_next_background_color(ui, color);
      ui_set_next_child_gap(ui, 0);
      ui_set_next_padding(ui, pad2_equal(3));
      UI_Node *day_cell = ui_nodef(ui, 
                                   UIFlags_Floating|
                                   UIFlags_DrawBackGround|
                                   UIFlags_Comm|
                                   UIFlags_DrawBorder, 
                                   S8("calendar_day_%u_%u"), x, y);
      if(ui_is_top_nav_interface(ui, day_cell->key))
      {
        day_cell->border_color = color_blue;
        day_cell->border_thickness = 1.0f;
      }
      ui_parent(ui, day_cell)
      {
        
        // NOTE(erb): date label
        ui_set_next_debug_str(ui, S8("date-label"));
        ui_sizing_y(ui, ui_fit())
          ui_align_x(ui, UIAlign_End)
          ui_padding(ui, pad2_xy(5, 0))
          ui_parentf(ui, 0, S8Z())
        {
          ui_labelf(ui, S8("%u/%u"), cell_date.day, cell_date.month);
        }
        
        ui_set_next_debug_str(ui, S8("event-list-container"));
        ui_set_next_layout_axis(ui, Axis2_Y);
        ui_parentf(ui, 0, S8Z())
          ui_padding(ui, pad2_equal(0))
          ui_font_size(ui, 18.0f)
          ui_font_color(ui, color_black)
          ui_padding(ui, pad2_xy(4.0f, 0))
        {
          // TODO(erb): make this filtering smarter!
          for(EachNode(Event, event, event_list->first))
          {
            if(event->start.date.day   == cell_date.day &&
               event->start.date.month == cell_date.month && 
               event->start.date.year  == cell_date.year)
            {
              ui_set_next_debug_str(ui, S8(event));
              ui_set_next_background_color(ui, v4f_xyz(v3ff(0.8f), 1.0f));
              ui_set_next_sizing(ui, ui_grow(), ui_fit());
              ui_set_next_corner_radius_abs(ui, 2.0f);
              UI_Node *e = ui_nodef(ui, 
                                    UIFlags_DrawBackGround
                                    |UIFlags_DrawRounded
                                    |UIFlags_Comm, 
                                    S8("cal-event-%d"), event->id);
              ui_parent(ui, e)
              {
                Scratch task_scratch = arena_get_scratch(scratch.arena);
                String8 title = str8_from_name_chunk_list(task_scratch.arena, &event->title_chunk_list);
                ui_labelf(ui, S8("%u %S"), hash_from_event_id(event->id), title);
                if(ui_comm_from_node(ui, e).left_clicked)
                {
                  //app_state->selected_event_id = event->id;
                }
                scratch_end(&task_scratch);
              }
            }
          }
        }
      }
      cell_cursor.x += cell_size.x;
    }
    cell_cursor.y += cell_size.y;
  }
  scratch_end(&scratch);
}

void
calendar_panel(App_State *app_state, View_ID id)
{
  Assert(view_id_is_valid(id));
  View *view = view_from_id(app_state, id);
  Assert(view);
  Assert(view->kind == ViewKind_Calendar);
  
  if(!view->is_initialized)
  {
    view->is_initialized = true;
    view->calendar_start = os_get_local_date_time().date;
    view->calendar_grid_size = v2i(1, 5);
  }
  
  UI_Ctx *ui = &app_state->ui;
  
  Scratch scratch = scratch_begin(0, 0);
  
  ui_sizing(ui, ui_grow(), ui_grow())
    ui_padding(ui, pad2_equal(0))
    ui_background_color(ui, color_magenta)
    ui_seed_key(ui, ui_key_make(hash_from_view_id(id)))
  {
    // NOTE(erb): this only allows one panel, this should have seed by the panel Id or something
    UI_Node *calendar_panel = ui_nodef(ui, UIFlags_DrawBackGround|UIFlags_Comm, S8("calendar_%uul"), id);
    ui_parent(ui, calendar_panel)
    {
      UI_Comm out = ui_comm_from_node(ui, calendar_panel);
      if(out.node != 0)
      {
        F32 scroll_k = 0.01f;
        if(key_down(ui, GFXKey_Control)) // NOTE(erb): scroll x when ctrl pressed
        {
          out.scroll.x = out.scroll.y;
          out.scroll.y = 0;
        }
        
        V2i scroll_size_delta = v2f_floor_to_v2i(v2f_scale(out.scroll, scroll_k));
        view->calendar_grid_size.x = Cap(view->calendar_grid_size.x + scroll_size_delta.x, 1, 7);
        view->calendar_grid_size.y = Cap(view->calendar_grid_size.y + scroll_size_delta.y, 1, 10);
        
        Range_V2f inner = node_inner_rng_v2f(out.node);
        ui_calendar(ui, view->calendar_start, view->calendar_grid_size, inner, &app_state->event_list);
      }
    }
  }
  
  scratch_end(&scratch);
}

typedef struct Table_Layout Table_Layout;
struct Table_Layout
{
  F32 left;
  V2f cursor;
  F32 max_row_height;
};

Table_Layout
table_init(V2f top_left, Column *cols)
{
  Table_Layout layout = {0};
  layout.cursor = top_left;
  layout.left = top_left.x;
  return(layout);
}

void
table_row(Table_Layout *layout)
{
  Assert(layout);
  layout->cursor.x = layout->left;
  layout->cursor.y += layout->max_row_height;
  layout->max_row_height = 0;
}

void
table_cell_end(Table_Layout *layout, V2f cell_size)
{
  layout->cursor.x += cell_size.x;
  layout->max_row_height = Max(layout->max_row_height, cell_size.y);
}

typedef enum Expr_Cmd_Kind
{
  ExprCmdKind_Primary,
  ExprCmdKind_BinaryBegin,
  ExprCmdKind_BinaryOp,
  ExprCmdKind_BinaryEnd,
  ExprCmdKind_FunctionBegin,
  ExprCmdKind_FunctionEnd,
} Expr_Cmd_Kind;

typedef struct Expr_Cmd Expr_Cmd;
struct Expr_Cmd
{
  Expr_Cmd_Kind kind;
  Expr *expr;
  Expr_Cmd *next;
};

typedef struct Expr_Cmd_List Expr_Cmd_List;
struct Expr_Cmd_List
{
  Expr_Cmd *first;
  Expr_Cmd *last;
};

Expr_Cmd *
expr_append_cmd(Arena *arena, Expr_Cmd_List *list, Expr *expr, Expr_Cmd_Kind kind)
{
  Expr_Cmd *cmd = PushStruct(arena, Expr_Cmd);
  SLLQueueAppend(list->first, list->last, cmd);
  cmd->kind = kind;
  cmd->expr = expr;
  return(cmd);
}

void
expr_append_cmd_from_expr(Arena *arena, Expr_Cmd_List *list, Expr *node)
{
  if(node != 0)
  {
    switch(node->kind)
    {
      case ExprKind_Primary:
      {
        expr_append_cmd(arena, list, node, ExprCmdKind_Primary);
      } break;
      
      case ExprKind_Binary:
      {
        expr_append_cmd(arena, list, node, ExprCmdKind_BinaryBegin);
        {
          expr_append_cmd_from_expr(arena, list, node->bin.left);
          expr_append_cmd(arena, list, node, ExprCmdKind_BinaryOp);
          expr_append_cmd_from_expr(arena, list, node->bin.right);
        }
        expr_append_cmd(arena, list, node, ExprCmdKind_BinaryEnd);
      } break;
      
      case ExprKind_Function:
      {
        expr_append_cmd(arena, list, node, ExprCmdKind_FunctionBegin);
        for(EachNode(Expr, child, node->func.args.first))
        {
          expr_append_cmd_from_expr(arena, list, child);
        }
        expr_append_cmd(arena, list, node, ExprCmdKind_FunctionEnd);
      } break;
      
      InvalidDefaultCase;
    }
  }
}


Expr_Cmd_List
expr_cmd_list_from_expr(Arena *arena, Expr *node)
{
  Expr_Cmd_List result = {0};
  expr_append_cmd_from_expr(arena, &result, node);
  return(result);
}

Fancy_String8_List
push_fancy_strs_from_expr_cmd_list(Arena *arena, Expr_Cmd_List *cmd_list)
{
  Fancy_String8_List result = {0};
  
  for(EachNode(Expr_Cmd, cmd, cmd_list->first))
  {
    Expr *expr = cmd->expr;
    Assert(expr != 0);
    switch(cmd->kind)
    {
      case ExprCmdKind_Primary:
      {
        String8 str = {0};
        V4f color = {0};
        switch(cmd->expr->prim.kind)
        {
          case ExprPrimaryKind_Identifier: 
          {
            color = color_blue; 
            str = expr->prim.str8;
          } break;
          
          case ExprPrimaryKind_NumLiteral: 
          {
            color = color_green; 
            str = push_str8f(arena, "%u", expr->prim.u32);
          } break;
          
          case ExprPrimaryKind_StrLiteral: 
          {
            color = v4f(0.1f, 0.5f, 0.1f, 1.0f); 
            str = push_str8f(arena, "`%S`", expr->prim.str8);
          } break;
          
          InvalidDefaultCase;
        }
        fancy_str8_push(arena, &result, str, color);
      } break;
      
      case ExprCmdKind_BinaryOp:
      {
        String8 str = {0};
        switch(expr->bin.kind)
        {
          case ExprBinaryKind_Sum: str = S8("+"); break;
          case ExprBinaryKind_Mul: str = S8("*"); break;
          case ExprBinaryKind_Sub: str = S8("-"); break;
          case ExprBinaryKind_Div: str = S8("/"); break;
          case ExprBinaryKind_Mod: str = S8("%"); break;
          
          case ExprBinaryKind_Eq: str = S8("=="); break;
          case ExprBinaryKind_Gt: str = S8(">");  break;
          case ExprBinaryKind_Ge: str = S8(">="); break;
          case ExprBinaryKind_Lt: str = S8("<");  break;
          case ExprBinaryKind_Le: str = S8("<="); break;
          
          case ExprBinaryKind_BitAnd  : str = S8("&");  break;
          case ExprBinaryKind_BitOr   : str = S8("|");  break;
          case ExprBinaryKind_LogicAnd: str = S8("&&"); break;
          case ExprBinaryKind_LogicOr : str = S8("||"); break;
          case ExprBinaryKind_Access  : str = S8(".");  break;
          
          InvalidDefaultCase;
        }
        fancy_str8_push(arena, &result, str, color_red);
      } break;
      
      case ExprCmdKind_BinaryBegin:
      {
        fancy_str8_push(arena, &result, S8("("), color_white);
      } break;
      
      case ExprCmdKind_FunctionBegin:
      {
        fancy_str8_push(arena, &result, expr->func.name, color_blue);
        fancy_str8_push(arena, &result, S8("("), color_white);
      } break;
      
      case ExprCmdKind_BinaryEnd:
      case ExprCmdKind_FunctionEnd:
      {
        fancy_str8_push(arena, &result, S8(")"), color_white);
      } break;
      
      InvalidDefaultCase;
    }
  }
  return(result);
}

void
cell_init_from_expr_value_leaf(Arena *arena, UI_Cell *cell, Expr_Value *value)
{
  Assert(cell != 0);
  
  cell->data = (void *)value;
  switch(value->type)
  {
    case ExprType_B32:
    {
      cell->label = (value->b32 ? S8("true") : S8("false"));
    } break;
    
    case ExprType_U32: 
    {
      cell->label = push_str8f(arena, "%u", value->u32);
    } break;
    
    case ExprType_Str8: 
    {
      cell->label = value->str8;
    } break;
    
    case ExprType_DateTime: 
    {
      cell->flags = UICellFlags_FancyStrLabel;
      fancy_str8_push(arena, &cell->label_fancy_str, 
                      push_str8f(arena, "%02u/%02u/%02u - %02u:%02u:%02u", 
                                 value->dt.date.day, value->dt.date.month, value->dt.date.year, 
                                 value->dt.time.hour, value->dt.time.minute, value->dt.time.second),
                      color_yellow);
    } break;
    
    case ExprType_Event:
    {
      Scratch scratch = scratch_begin(&arena, 1);
      
      Assert(value->event != 0);
      Calendar_Date start = value->event->start.date;
      Calendar_Date end = value->event->end.date;
      String8 title = str8_from_name_chunk_list(arena, &value->event->title_chunk_list);
      
      cell->flags = UICellFlags_FancyStrLabel;
      fancy_str8_push(arena, &cell->label_fancy_str, push_str8f(arena, "#%u ", value->event->id), color_yellow);
      fancy_str8_push(arena, &cell->label_fancy_str, title, color_white);
      fancy_str8_push(arena, &cell->label_fancy_str, push_str8f(arena, " %02u/%02u/%u", start.day, start.month, start.year), color_blue);
      fancy_str8_push(arena, &cell->label_fancy_str, S8(" - "), color_white);
      fancy_str8_push(arena, &cell->label_fancy_str, push_str8f(arena, "%02u/%02u/%u", end.day, end.month, end.year), v4f(0.2f, 0.2f, 0.8f, 1.0f));
      
      cell->filter_needle = title;
      
      scratch_end(&scratch);
    } break;
    
    case ExprType_List:
    {
      String8 expr_type_str_table[] = 
      {
        [ExprType_None    ] = S8Lit("?"),
        [ExprType_B32     ] = S8Lit("b32"),
        [ExprType_U32     ] = S8Lit("u32"),
        [ExprType_Str8    ] = S8Lit("str8"),
        [ExprType_DateTime] = S8Lit("datetime"),
        [ExprType_Event   ] = S8Lit("event"),
        [ExprType_List    ] = S8Lit("list"),
      };
      Assert(ArraySize(expr_type_str_table) == ExprType_COUNT);
      String8 sub_type_str = expr_type_str_table[value->sub_type];
      
      cell->flags = UICellFlags_FancyStrLabel;
      fancy_str8_push(arena, &cell->label_fancy_str, S8("list<"), color_white);
      fancy_str8_push(arena, &cell->label_fancy_str, sub_type_str, color_blue);
      fancy_str8_push(arena, &cell->label_fancy_str, S8(">["), color_white);
      fancy_str8_push(arena, &cell->label_fancy_str, push_str8f(arena, "%d", value->list.count), color_green);
      fancy_str8_push(arena, &cell->label_fancy_str, S8("]"), color_white);
    } break;
  }
}

void
cell_arr_append_from_expr_value_recur(Arena *arena, UI_Cell *cell_arr, U32 *cell_count, U32 cell_capacity, 
                                      Expr_Value *value)
{
  if(*cell_count + 1 <= cell_capacity)
  {
    UI_Cell *heading_cell = (cell_arr + (*cell_count)++);
    cell_init_from_expr_value_leaf(arena, heading_cell, value);
    if(value->type == ExprType_List)
    {
      for(EachNode(Expr_Value, v, value->list.first))
      {
        cell_arr_append_from_expr_value_recur(arena, cell_arr, cell_count, cell_capacity, v);
      }
    }
  }
}

U32 
expr_value_total_node_count_rec(Expr_Value *value)
{
  U32 result = 1;
  if(value->type == ExprType_List)
  {
    for(EachNode(Expr_Value, v, value->list.first))
    {
      result += expr_value_total_node_count_rec(v);
    }
  }
  return(result);
}

Expr_Variable *
expr_env_push_var(Expr_Environment *env, Arena *arena)
{
  Expr_Variable *result = PushStruct(arena, Expr_Variable);
  SLLStackPush(env->first_var, result);
  return(result);
}

Expr_Variable *
expr_env_append_var_value(Expr_Environment *env, Arena *arena, String8 iden, Expr_Value value)
{
  Expr_Variable *var = expr_env_push_var(env, arena);
  var->iden = iden;
  var->is_func = false;
  var->value = value;
  return(var);
}

Expr_Variable *
expr_env_append_var_func(Expr_Environment *env, Arena *arena, String8 iden, Expr_Function_Impl_Func *func)
{
  Expr_Variable *var = expr_env_push_var(env, arena);
  var->iden = iden;
  var->is_func = true;
  var->only_expr_args = false;
  var->func = func;
  return(var);
}


Expr_Variable *
expr_env_append_var_expr_func(Expr_Environment *env, Arena *arena, String8 iden, Expr_Function_Impl_Func *func)
{
  Expr_Variable *var = expr_env_append_var_func(env, arena, iden, func);
  var->only_expr_args = true;
  return(var);
}

EXPR_FUNCTION_IMPL_FUNC(app_get_events_expr) // Arena *arena, Expr_Environment *env, Expr_Value_List *arg_values, Expr_List *arg_exprs, void *data
{
  Assert(env->data != 0);
  
  App_State *app_state = (App_State *)env->data;
  
  Expr_Value_List list = {0};
  for(EachNode(Event, event, app_state->event_list.first))
  {
    Expr_Value *v = expr_value_list_push(arena, &list);
    v->type = ExprType_Event;
    v->event = event;
  }
  
  Expr_Func_Impl_Result result = {0};
  result.value.list = list;
  result.value.sub_type = ExprType_Event;
  result.value.type = ExprType_List;
  return(result);
}

EXPR_FUNCTION_IMPL_FUNC(app_get_event_from_id_expr) // Arena *arena, Expr_Environment *env, Expr_Value_List *arg_values, Expr_List *arg_exprs, void *data
{
  Assert(env->data != 0);
  
  String8 error = {0};
  if(arg_values->count != 1)
  {
    error = S8("expected one argument");
  }
  
  
  // TODO(erb): use computed value of the arguments!!!!!!
  Event_ID id = {0};
  if(error.value == 0)
  {
    Expr_Value *arg = arg_values->first;
    Assert(arg != 0);
    
    if(arg->type == ExprType_U32)
    {
      id = event_id_make(arg->u32);
    }
    else
    {
      error = S8("expected a number literal argument");
    }
  }
  
  App_State *app_state = (App_State *)env->data;
  
  Event *event = 0;
  if(error.value == 0)
  {
    for(EachNode(Event, test, app_state->event_list.first))
    {
      if(event_id_equals(test->id, id))
      {
        event = test;
        break;
      }
    }
  }
  
  Expr_Func_Impl_Result result = {0};
  result.error = error;
  if(error.value == 0)
  {
    if(event != 0)
    {
      result.value.type = ExprType_Event;
      result.value.event = event;
    }
    else
    {
      result.error = S8("no event with id");
    }
  }
  return(result);
}


EXPR_FUNCTION_IMPL_FUNC(app_get_tags_expr) // Arena *arena, Expr_Environment *env, Expr_Value_List *arg_values, Expr_List *arg_exprs, void *data
{
  Assert(env->data != 0);
  
  App_State *app_state = (App_State *)env->data;
  
  Expr_Value_List list = {0};
  for(EachElement(Event_Tag, tag, app_state->tags))
  {
    if(tag->used)
    {
      Expr_Value *v = expr_value_list_push(arena, &list);
      v->type = ExprType_Str8;
      v->str8 = S8Struct(tag->name, tag->name_size);
    }
  }
  
  Expr_Func_Impl_Result result = {0};
  result.value.list = list;
  result.value.sub_type = ExprType_Str8;
  result.value.type = ExprType_List;
  return(result);
}

EXPR_FUNCTION_IMPL_FUNC(app_ui_rule_calendar) // Arena *arena, Expr_Value_List *arg_values, Expr_List *arg_exprs, void *data
{
  Expr_Func_Impl_Result result = {0};
  
  Assert(arg_values != 0);
  if(result.error.value == 0 && 
     arg_values->count != 1)
  {
    result.error = S8("expected one argument");
  }
  
  Expr_Value *arg = 0;
  if(result.error.value == 0)
  {
    arg = arg_values->first;
    Assert(arg != 0);
    if(!(arg->type == ExprType_List && 
         arg->sub_type == ExprType_DateTime))
    {
      result.error = S8("expected a date time list");
    }
  }
  
  if(result.error.value == 0)
  {
    result.value = *arg;
    result.value.ui_rule = ExprUIRule_Calendar;
  }
  return(result);
}

EXPR_FUNCTION_IMPL_FUNC(app_ui_rule_raw) // Arena *arena, Expr_Value_List *arg_values, Expr_List *arg_exprs, void *data
{
  Expr_Func_Impl_Result result = {0};
  
  Assert(arg_values != 0);
  if(result.error.value == 0 && 
     arg_values->count != 1)
  {
    result.error = S8("expected a single argument");
  }
  
  Expr_Value *arg = 0;
  if(result.error.value == 0)
  {
    arg = arg_values->first;
    Assert(arg != 0);
    if(arg->ui_rule != ExprUIRule_None)
    {
      result.error = S8("allready decorated?");
      InvalidCodePath;
    }
  }
  if(result.error.value == 0)
  {
    result.value = *arg;
    result.value.ui_rule = ExprUIRule_Raw;
  }
  return(result);
}

EXPR_FUNCTION_IMPL_FUNC(app_ui_rule_focus) // Arena *arena, Expr_Value_List *arg_values, Expr_List *arg_exprs, void *data
{
  Expr_Func_Impl_Result result = {0};
  
  Assert(arg_values != 0);
  if(arg_values->count != 1)
  {
    result.error = S8("expected one argument");
  }
  
  Expr_Value *arg = 0;
  if(result.error.value == 0)
  {
    arg = arg_values->first;
    Assert(arg != 0);
    if(arg->type != ExprType_Event)
    {
      result.error = S8("expected a date time list");
    }
  }
  
  if(result.error.value == 0)
  {
    result.value = *arg;
    result.value.ui_rule = ExprUIRule_FocusEvent;
  }
  return(result);
}

EXPR_FUNCTION_IMPL_FUNC(app_expand_arg_columns_ui_rule) // Arena *arena, Expr_Environment *env, Expr_Value_List *arg_values, Expr_List *arg_exprs, void *data
{
  Scratch scratch = scratch_begin(&arena, 1);
  
  String8 error = {0};
  
  Assert(arg_exprs != 0);
  if(arg_exprs->count < 2)
  {
    error = S8("expected at least two arguments");
  }
  
  // NOTE(erb): first arg as an expression and evaluate it
  Expr_Value first_arg_value = {0};
  if(error.value == 0)
  {
    Expr *head_expr = arg_exprs->first;
    Assert(head_expr != 0);
    first_arg_value = expr_value_from_expr_with_environment(scratch.arena, head_expr, env, &error);
  }
  
  Expr_Value_List table = {0};
  if(error.value == 0)
  {
    Expr_List column_args = {0};
    column_args.first = arg_exprs->first->next;
    column_args.last = arg_exprs->last;
    Assert(column_args.first != 0 && column_args.last != 0);
    
    Expr_Value *first_context_value = (first_arg_value.type == ExprType_List ? 
                                       first_arg_value.list.first : 
                                       &first_arg_value);
    
    for(EachNode(Expr_Value, context_value, first_context_value))
    {
      Expr_Environment context_env = {0};
      expr_env_append_var_value(&context_env, scratch.arena, S8("$"), *context_value);
      
      Expr_Value_List row_eval = expr_value_list_from_expr_list(arena, &context_env, &column_args, &error);
      
      Expr_Value *store = expr_value_list_push(arena, &table);
      store->list = row_eval;
      store->type = ExprType_List;
    }
  }
  
  Expr_Func_Impl_Result result = {0};
  result.error = error;
  if(error.value == 0)
  {
    result.value.list = table;
    result.value.type = ExprType_List;
    result.value.ui_rule = ExprUIRule_Columns;
  }
  scratch_end(&scratch);
  return(result);
}


Tag_ID 
create_event_tag(App_State *app_state, String8 name, V4f color)
{
  Tag_ID result = push_event_tag(app_state, name, color);
  save_state(app_state);
  return(result);
}

void
event_add_tag(App_State *app_state, Event *event, Tag_ID tag)
{
  event_push_tag(event, tag);
  save_state(app_state);
}

void
ui_focus_event(UI_Ctx *ui, App_State *app_state, Event *event, B32 *is_color_picker_open, V4f *new_tag_color, 
               Date_Input_State *date_input_state)
{
  Scratch scratch = scratch_begin(0, 0);
  V4f label_color = with_alpha(v4ff(.35f), 1.0f);
  
  ui_set_next_sizing(ui, ui_grow(), ui_grow());
  ui_set_next_layout_axis(ui, Axis2_Y);
  ui_set_next_background_color(ui, color_black);
  ui_set_next_padding(ui, pad2_equal(10.0f));
  ui_parentf(ui, UIFlags_DrawBackGround, S8Z())
  {
    
    V2f color_picker_pos = {0};
    
    // NOTE(erb): title
    {
#if 0
      ui_set_next_font_size(ui, 64.0f); // TODO(erb): fix this!!!!!!
#endif
      ui_set_next_font_color(ui, label_color);
      ui_labelf(ui, S8("title"));
      
      UI_Line_Input_State *title_input = ui_line_input(ui, ui_keyf("create_event_input"));
      if(ui_line_input_created(ui, title_input))
      {
        String8 title = str8_from_name_chunk_list(scratch.arena, &event->title_chunk_list);
        str8_writer_str(&title_input->writer, title);
      }
      if(ui_line_input_changed(ui, title_input))
      {
        name_chunk_list_free_all(&event->title_chunk_list, 
                                 &app_state->first_free_name_chunk);
        event->title_chunk_list = push_name_chunk_list(&app_state->name_chunk_arena, 
                                                       &app_state->first_free_name_chunk,
                                                       title_input->writer.str);
      }
    }
    
    // NOTE(erb): tags
    {
      ui_set_next_font_color(ui, label_color);
      ui_parentf(ui, UIFlags_DrawText, S8("tags"));
      
      // NOTE(erb): list tags
      ui_set_next_layout_axis(ui, Axis2_X);
      ui_set_next_sizing(ui, ui_grow(), ui_fit());
      ui_parentf(ui, 0, S8Z())
      {
        for(U32Each(idx, event->tag_count))
        {
          Event_Tag *tag = event_tag_from_id(app_state, event->tags[idx]);
          ui_set_next_font_color(ui, tag->color);
          ui_labelf(ui, S8Struct(tag->name, tag->name_size));
        }
      }
      
      // NOTE(erb): add tag
      {
        UI_Line_Input_State *tag_input = 0;
        
        ui_set_next_sizing(ui, ui_grow(), ui_fit());
        ui_parentf(ui, 0, S8Z())
        {
          tag_input = ui_line_input(ui, ui_keyf("event_tag_input"));
          
          ui_set_next_seed_key(ui, ui_keyf("color_picker_seed"));
          ui_set_next_sizing(ui, ui_fixed(30), ui_grow());
          UI_Comm trigger = ui_color_picker_trigger(ui, *is_color_picker_open, *new_tag_color);
          if(trigger.left_clicked)
          {
            *is_color_picker_open = !(*is_color_picker_open);
          }
          if(trigger.node)
          {
            Range_V2f rng = node_rng_v2f(trigger.node);
            color_picker_pos = v2f(rng.min.x, rng.max.y+5);
          }
          
          if(ui_buttonf(ui, S8("New Tag##new_tag_button")).left_clicked)
          {
            Tag_ID tag_id = create_event_tag(app_state, tag_input->writer.str, *new_tag_color);
            event_add_tag(app_state, event, tag_id);
            
            // NOTE(erb): clear input
            *new_tag_color = color_red;
            str8_writer_reset(&tag_input->writer);
          }
        }
        
        ui_set_next_layout_axis(ui, Axis2_X);
        ui_set_next_sizing(ui, ui_grow(), ui_fit());
        ui_parentf(ui, 0, S8Z())
        {
          // NOTE(erb): tag search
          for(EachElement(Event_Tag, tag, app_state->tags))
          {
            if(tag->used)
            {
              String8 name = S8Struct(tag->name, tag->name_size);
              Tag_ID tag_id = id_from_tag(app_state, tag);
              
              if(!event_has_tag(event, tag_id) && 
                 str8_find(name, tag_input->writer.str, 
                           MatchFlags_CaseInsens).exists)
              {
                ui_set_next_background_color(ui, tag->color);
                if(ui_buttonf(ui, S8("%S##tag_button_%d"), name, tag_id).left_clicked)
                {
                  event_add_tag(app_state, event, tag_id);
                }
              }
            }
          }
        }
      }
    }
    
    // NOTE(erb): date
    {
      ui_set_next_font_color(ui, label_color);
      ui_labelf(ui, S8("date"));
      
      ui_set_next_sizing(ui, ui_grow(), ui_fit());
      ui_date_time_line_input(ui, date_input_state, &event->start, &event->end);
      
      ui_set_next_background_color(ui, with_alpha(v4ff(0.2f), 1.0f));
      ui_set_next_sizing(ui, ui_grow(), ui_fit());
      ui_date_time_range_picker(ui, date_input_state, &event->start, &event->end);
      
      String8 error = date_input_state->last_error;
      if(error.value != 0)
      {
        ui_set_next_font_color(ui, color_red);
        ui_labelf(ui, S8("error: %S"), error);
      }
    }
    
    // NOTE(erb): floating color picker
    if(*is_color_picker_open)
    {
      ui_color_picker_popup(ui, color_picker_pos, is_color_picker_open, new_tag_color);
    }
  }
  scratch_end(&scratch);
}

void
watch_panel(App_State *app_state, View_ID id)
{
  Assert(view_id_is_valid(id));
  View *view = view_from_id(app_state, id);
  Assert(view);
  Assert(view->kind == ViewKind_Watch);
  
  if(!view->is_initialized)
  {
    view->is_initialized = true;
    view->expr_writer_buffer_size = 64;
    arena_init(&view->expr_state_arena, view->expr_state_buffer, ArraySize(view->expr_state_buffer));
  }
  
  UI_Ctx *ui = &app_state->ui;
  UI_Key input_key = ui_keyf("watch_expr_input");
  
  // NOTE(erb): expr selection
  {
    I32 new_cursor = view->selection_cursor;
    if(key_pressed(ui, GFXKey_Up))
    {
      new_cursor -= 1;
    }
    if(key_pressed(ui, GFXKey_Down))
    {
      new_cursor += 1;
    }
    if(new_cursor < 0)
    {
      new_cursor = (I32)view->expr_state_count;
    }
    if(new_cursor > (I32)view->expr_state_count)
    {
      new_cursor = 0;
    }
    view->selection_cursor = new_cursor;
  }
  
  B32 activate_edit_field = key_pressed(ui, GFXKey_F2);
  B32 exit_edit_field = key_pressed(ui, GFXKey_Escape);
  B32 toggle_expr_expanded = key_pressed(ui, GFXKey_Enter);
  
  ui_set_next_sizing(ui, ui_grow(), ui_grow());
  ui_set_next_layout_axis(ui, Axis2_Y);
  ui_set_next_background_color(ui, with_alpha(v4ff(0.1f), 1));
  ui_set_next_child_gap(ui, 0);
  ui_parentf(ui, UIFlags_DrawBackGround, S8Z())
  {
    UI_Line_Input_State *input = ui_line_input_state_from_key(ui, input_key);
    
    // NOTE(erb): ON enter new expr
    if(input->writer.str.size != 0 && toggle_expr_expanded)
    {
      Expr_State *node = view->first_free_expr_state;
      if(node != 0)
      {
        SLLStackPop(view->first_free_expr_state);
      }
      else
      {
        node = PushStruct(&view->expr_state_arena, Expr_State);
        U8 *mem = PushArray(&view->expr_state_arena, view->expr_writer_buffer_size, U8);
        str8_writer_init(&node->input_state.writer, mem, view->expr_writer_buffer_size);
      }
      SLLQueueAppend(view->first_expr_state, view->last_expr_state, node);
      view->expr_state_count += 1;
      str8_writer_str(&node->input_state.writer, input->writer.str);
      node->input_state.key = ui_keyf("expr_edit_input_%p", node);
      
      view->selection_cursor = (I32)view->expr_state_count;
      
      str8_writer_reset(&input->writer);
    }
    
    {
      // NOTE(erb): ui for each expr
      Scratch view_scratch = scratch_begin(0, 0);
      I32 expr_idx = 0;
      for(EachNode(Expr_State, expr_state, view->first_expr_state))
      {
        String8 expr_str = expr_state->input_state.writer.str;
        Scratch scratch = arena_get_scratch(view_scratch.arena);
        
        B32 is_selected = (view->selection_cursor == expr_idx);
        
        if(is_selected)
        {
          if(activate_edit_field)
          {
            expr_state->is_editing = true;
          }
          if(exit_edit_field)
          {
            expr_state->is_editing = false;
          }
          if(toggle_expr_expanded)
          {
            expr_state->is_expanded = !expr_state->is_expanded;
          }
        }
        
        // NOTE(erb): tokenizing
        String8 error = {0};
        Token_List tokens = token_list_from_str(scratch.arena, expr_str, &error);
        
        // NOTE(erb): parsing
        Expr *expr = 0;
        if(error.value == 0)
        {
          Token_Iterator token_it = {0};
          token_it.current = tokens.first;
          expr = expr_from_token_it(scratch.arena, &token_it, &error);
        }
        
        // NOTE(erb): evaluating
        {
          Expr_Value value = {0};
          if(error.value == 0)
          {
            // NOTE(erb): get global vars
            Expr_Environment env = {0};
            env.data = (void *)app_state;
            
            expr_env_append_var_func(&env, scratch.arena, S8("events"), app_get_events_expr);
            expr_env_append_var_func(&env, scratch.arena, S8("event"), app_get_event_from_id_expr);
            expr_env_append_var_func(&env, scratch.arena, S8("tags"), app_get_tags_expr);
            
            expr_env_append_var_expr_func(&env, scratch.arena, S8("columns"), app_expand_arg_columns_ui_rule);
            
            expr_env_append_var_func(&env, scratch.arena, S8("calendar"), app_ui_rule_calendar);
            expr_env_append_var_func(&env, scratch.arena, S8("raw"), app_ui_rule_raw);
            expr_env_append_var_func(&env, scratch.arena, S8("focus"), app_ui_rule_focus);
            
            Expr_Value now_value = {0};
            now_value.dt = os_get_local_date_time();
            now_value.type = ExprType_DateTime;
            expr_env_append_var_value(&env, scratch.arena, S8("now"), now_value);
            
            value = expr_value_from_expr_with_environment(scratch.arena, expr, &env, &error);
          }
          
          if(error.value != 0)
          {
            value.str8 = error;
            value.type = ExprType_Str8;
          }
          
          // NOTE(erb): ui rendering
          switch(value.ui_rule)
          {
            case ExprUIRule_None:
            case ExprUIRule_Raw:
            case ExprUIRule_Columns:
            {
              UI_Flags header_row_flags = 0;
              if(is_selected)
              {
                header_row_flags |= UIFlags_DrawBackGround;
                ui_set_next_background_color(ui, with_alpha(color_blue, 0.2f));
              }
              ui_set_next_sizing(ui, ui_grow(), ui_fit());
              ui_set_next_layout_axis(ui, Axis2_X);
              ui_set_next_child_gap(ui, 0);
              ui_parentf(ui, header_row_flags, S8Z())
              {
                // NOTE(erb): header
                {
                  UI_Cell cell = {0};
                  if(expr_state->is_editing)
                  {
                    cell.flags = UICellFlags_LineInput;
                    cell.line_input = &expr_state->input_state;
                  }
                  else
                  {
                    cell.flags = UICellFlags_FancyStrLabel | (expr_state->is_expanded ? 
                                                              UICellFlags_DrawExpandArrowOpen : 
                                                              UICellFlags_DrawExpandArrowClose);
                    fancy_str8_push(scratch.arena, &cell.label_fancy_str, expr_str, color_red);
                  }
                  ui_cell(ui, &cell, ui_keyf("expr_label_cell_%S", expr_str));
                }
                
                // NOTE(erb): leaf
                {
                  UI_Cell cell = {0};
                  cell_init_from_expr_value_leaf(scratch.arena, &cell, &value);
                  ui_cell(ui, &cell, ui_keyf("table_cell_expr_ui"));
                }
              }
              
              if(expr_state->is_expanded && value.type == ExprType_List)
              {
                if(value.ui_rule == ExprUIRule_Columns)
                {
                  for(EachNode(Expr_Value, row_expr, value.list.first))
                  {
                    if(row_expr->type == ExprType_List)
                    {
                      ui_set_next_layout_axis(ui, Axis2_X);
                      ui_set_next_sizing_x(ui, ui_grow());
                      ui_parentf(ui, 0, S8Z())
                      {
                        for(EachNode(Expr_Value, col_expr, row_expr->list.first))
                        {
                          UI_Cell *cell = PushStruct(scratch.arena, UI_Cell);
                          cell_init_from_expr_value_leaf(scratch.arena, cell, col_expr);
                          ui_cell(ui, cell, ui_keyf("table_cell_expr_ui_%p", cell));
                        }
                      }
                    }
                    else
                    {
                      ui_labelf(ui, S8("expected rows to be list type"));
                      break;
                    }
                  }
                }
                else
                {
                  UI_Cell *cell_arr = PushArray(scratch.arena, value.list.count, UI_Cell);
                  U32 cell_idx = 0;
                  for(EachNode(Expr_Value, child, value.list.first))
                  {
                    UI_Cell *cell = (cell_arr + cell_idx++);
                    cell_init_from_expr_value_leaf(scratch.arena, cell, child);
                  }
                  ui_cell_lister_items(ui, cell_arr, value.list.count, S8Z(), 0, true);
                }
              }
            } break;
            
            case ExprUIRule_Calendar:
            {
              ui_labelf(ui, S8("calendar!!!!!!!"));
              if(value.type == ExprType_List && 
                 value.sub_type == ExprType_DateTime)
              {
                ui_set_next_sizing(ui, ui_grow(), ui_grow());
                UI_Node *calendar_container = ui_nodef(ui, 0, S8("watch_calendar_container"));
                ui_parent(ui, calendar_container);
                
                UI_Node *cached_cal_container = ui_node_from_key(ui, calendar_container->key);
                if(cached_cal_container != 0)
                {
                  Range_V2f space = node_inner_rng_v2f(cached_cal_container);
                  if(value.list.count > 0)
                  {
                    Assert(value.list.first != 0);
                    Assert(value.list.first->type == ExprType_DateTime);
                    Calendar_Date start_date = value.list.first->dt.date;
                    
                    U32 weeks_count = CapBottom(value.list.count/7, 1);
                    
                    ui_calendar(ui, start_date, v2i(7, weeks_count), space, &app_state->event_list);
                  }
                  else
                  {
                    ui_labelf(ui, S8("empty calendar"));
                  }
                }
              }
              else
              {
                ui_labelf(ui, S8("expected list of dates for calendar view"));
                InvalidCodePath;
              }
            } break;
            
            case ExprUIRule_FocusEvent:
            {
              ui_labelf(ui, S8("focus event"));
              
              if(value.type == ExprType_Event)
              {
                Assert(value.event != 0);
                B32 is_color_picker_open = false;
                V4f new_tag_color = color_red;
                Date_Input_State date_input_state = {0};
                ui_focus_event(ui, app_state, value.event, 
                               &is_color_picker_open, &new_tag_color, &date_input_state);
              }
              else
              {
                ui_labelf(ui, S8("expected event for focus view"));
                InvalidCodePath;
              }
            } break;
            
            InvalidDefaultCase;
            
          }
        }
        
        expr_idx += 1;
      }
      
      if(view->selection_cursor == (I32)view->expr_state_count)
      {
        ui_set_next_border_color(ui, color_red);
      }
      ui_line_input(ui, input_key);
      scratch_end(&view_scratch);
    }
  }
}

void
table_panel(App_State *app_state, View_ID id)
{
  Assert(view_id_is_valid(id));
  View *view = view_from_id(app_state, id);
  Assert(view != 0);
  Assert(view->kind == ViewKind_Table);
  
  if(!view->is_initialized)
  {
    view->is_initialized = true;
    columns_append_and_resize(view, 0.5f);
    columns_equal_size(view);
  }
  
  UI_Ctx *ui = &app_state->ui;
  
  Scratch scratch = scratch_begin(0, 0);
  
  ui_set_next_sizing(ui, ui_grow(), ui_grow());
  ui_set_next_layout_axis(ui, Axis2_Y);
  ui_set_next_background_color(ui, with_alpha(v4ff(0.2f), 1.0f));
  UI_Node *table_space = ui_nodef(ui, UIFlags_DrawBackGround, S8("table_space_%ull"), id);
  ui_parent(ui, table_space)
  {
    Range_V2f table_rect = {0};
    UI_Node *cached = ui_node_from_key(ui, table_space->key);
    if(cached) 
    {
      table_rect = node_inner_rng_v2f(cached);
    }
    
    F32 row_width = rng_v2f_size(table_rect).x;
    
    UI_Key last_row_key = ui_keyf("last_row_%u", id);
    UI_Node *last_row_node = ui_node_from_key(ui, last_row_key);
    F32 last_row_max_y = 0; 
    if(last_row_node != 0)
    {
      last_row_max_y = node_rng_v2f(last_row_node).max.y;
    }
    F32 table_max_y = Max(last_row_max_y, table_rect.min.y);
    
    B32 did_hover_resize = false;
    
    // NOTE(erb): borders
    
    V2f cursor = table_rect.min;
    
    for(U32 col_idx = 1; col_idx < view->column_count; ++col_idx)
    {
      Column *min_col = (view->columns + col_idx-1);
      Column *max_col = (view->columns + col_idx);
      
      cursor.x += min_col->pct_of_row*row_width;
      
      Range_V2f rect = {0};
      rect.min = v2f(cursor.x-8.0f, cursor.y);
      rect.max = v2f(cursor.x+8.0f, table_max_y);
      
      ui_set_next_rect(ui, rect);
      ui_set_next_hover_cursor(ui, GFXCursorKind_ArrowWE);
      UI_Node *border = ui_nodef(ui, 
                                 UIFlags_Floating
                                 |UIFlags_Comm,
                                 S8("column_resize_border_%u_%u"), 
                                 id, col_idx);
      ui_parent(ui, border);
      UI_Comm comm = ui_comm_from_node(ui, border);
      
      // TODO(erb): very very hacky stuff to avoid event bubbling
      if(comm.hovered)
      {
        did_hover_resize = true;
      }
      if(comm.left_dragging)
      {
        F32 drag_delta = ui->input.cursor_delta.x;
        F32 min_col_px__pre_drag = min_col->pct_of_row * row_width;
        F32 max_col_px__pre_drag = max_col->pct_of_row * row_width;
        F32 min_col_px__post_drag = min_col_px__pre_drag + drag_delta;
        F32 max_col_px__post_drag = max_col_px__pre_drag - drag_delta;
        Assert(row_width > 0);
        min_col->pct_of_row = min_col_px__post_drag/row_width;
        max_col->pct_of_row = max_col_px__post_drag/row_width;
      }
      else if(comm.right_clicked)
      {
        columns_equal_size(view);
      }
    }
    
    
    Table_Layout layout = table_init(table_rect.min, view->columns);
    
    // NOTE(erb): header
    B32 is_first = true;
    for(U32Each(col_idx, view->column_count))
    {
      Column *col = (view->columns + col_idx);
      
      F32 width = col->pct_of_row*row_width;
      ui_set_next_floating_pos(ui, layout.cursor);
      ui_set_next_sizing(ui, ui_fixed(width), ui_fit());
      ui_set_next_border_abs(ui, color_white, 1, 0);
      
      UI_Flags flags = UIFlags_Floating|UIFlags_DrawBorder|UIFlags_Comm;
      UI_Key cell_key = {0};
      if(is_first && app_state->event_list.first == 0)
      {
        Assert(app_state->event_list.last == 0);
        is_first = false;
        cell_key = last_row_key;
        flags |= UIFlags_DontSeedKey;
      }
      else
      {
        cell_key = ui_keyf("col_header_cell_%llu_%u", id, col_idx);
      }
      UI_Node *cell = ui_node_make(ui, flags, cell_key, S8Z());
      ui_parent(ui, cell)
      {
        if(col->label.size > 0)
        {
          ui_labelf(ui, col->label);
        }
        else
        {
          UI_Key key = ui_keyf("column_input_key_%u", col_idx);
          UI_Line_Input_State *input = ui_line_input(ui, key);
          col->eval_str = input->writer.str;
        }
      }
      
      // TODO(erb): this is an extreme hack to avoid event bubbling for the drag events.
      if(!did_hover_resize)
      {
        if(ui_comm_from_node(ui, cell).right_clicked)
        {
          columns_append_and_resize(view, 0.3f);
        }
      }
      
      UI_Node *cached_cell = ui_node_from_key(ui, cell->key);
      F32 height = (cached_cell != 0 ? cached_cell->size.y : 0);
      table_cell_end(&layout, v2f(width, height));
    }
    
    // NOTE(erb): rows
    for(EachNode(Event, event, app_state->event_list.first))
    {
      table_row(&layout);
      is_first = true;
      for(U32Each(col_idx, view->column_count))
      {
        Column *col = (view->columns + col_idx);
        F32 width = col->pct_of_row*row_width;
        
        UI_Flags flags = (UIFlags_Floating|
                          UIFlags_DrawBorder|
                          UIFlags_Comm);
        UI_Key cell_key = {0};
        if(is_first && (event == app_state->event_list.last))
        {
          is_first = false;
          cell_key = last_row_key;
          flags |= UIFlags_DontSeedKey;
        }
        else
        {
          cell_key = ui_keyf("col_event_cell_%u_%llu_%u", event->id, id, col_idx);
        }
        ui_set_next_floating_pos(ui, layout.cursor);
        ui_set_next_sizing(ui, ui_fixed(width), ui_fit());
        ui_set_next_border_abs(ui, color_white, 1, 0);
        UI_Node *cell_container = ui_node_make(ui, flags, cell_key, S8Z());
        if(ui_comm_from_node(ui, cell_container).hovered ||
           ui_is_top_nav_interface(ui, cell_container->key))
        {
          cell_container->border_color = color_blue;
          cell_container->border_thickness = 1.0f;
        }
        ui_parent(ui, cell_container)
        {
          Scratch cell_scratch = arena_get_scratch(scratch.arena);
          
          String8 error = {0};
          
          Expr_Environment env = {0};
          Expr_Value event_value = {0};
          event_value.event = event;
          event_value.type = ExprType_Event;
          expr_env_append_var_value(&env, scratch.arena, S8("$"), event_value);
          
          Expr_Value value = expr_value_from_str8_with_environment(cell_scratch.arena, col->eval_str, &env, &error);
          if(error.value != 0)
          {
            value.str8 = error;
            value.type = ExprType_Str8;
          }
          
          // TODO(erb): make this call cleaner
          UI_Cell cell = {0};
          cell_init_from_expr_value_leaf(scratch.arena, &cell, &value);
          ui_cell(ui, &cell, ui_keyf("table_cell_expr_ui"));
          
          scratch_end(&cell_scratch);
        }
        
        UI_Node *cached_cell = ui_node_from_key(ui, cell_container->key);
        F32 height = (cached_cell != 0 ? cached_cell->size.y : 0);
        table_cell_end(&layout, v2f(width, height));
      }
    }
  }
  
  scratch_end(&scratch);
}

void
load_state(App_State *app_state)
{
  String8 file_path = app_state->load_from_file_path;
  if(file_path.size > 0)
  {
    Scratch scratch = scratch_begin(0, 0);
    
    Assert(app_state->read_entire_file_str8 != 0);
    String8 contents = app_state->read_entire_file_str8(scratch.arena, file_path);
    
    SRL_Project_File project = srl_project_from_str(scratch.arena, contents);
    
    if(project.error.value == 0)
    {
      app_state->global_id_counter = project.global_id_counter;
      
      for(EachNode(SRL_Tag, srl_tag, project.first_tag))
      {
        push_event_tag(app_state, srl_tag->name, srl_tag->color);
      }
      
      for(EachNode(SRL_Event, e, project.first_event))
      {
        Event *event = insert_event(app_state, event_id_make(e->id), e->name, e->start, e->end);
        for(EachNode(SRL_Tag_Ref, tag, e->first_tag))
        {
          event_push_tag(event, tag_id_make(tag->id));
        }
      }
    }
    else InvalidCodePath;
    
    scratch_end(&scratch);
  }
}

Event *
make_event(App_State *app_state, String8 title, Date_Time start, Date_Time end)
{
  Assert(app_state);
  Event *new_event = push_event(&app_state->event_arena, &app_state->event_list, 
                                &app_state->name_chunk_arena,
                                &app_state->first_free_name_chunk, 
                                title, 
                                event_id_make(app_state->global_id_counter++),
                                start, end);
  save_state(app_state);
  return(new_event);
}

void
remove_event(App_State *app_state, Event *event)
{
  DLLRemove(app_state->event_list.first, app_state->event_list.last, event);
  app_state->event_list.count--;
  save_state(app_state);
}

void
lister_panel(App_State *app_state, Panel *panel, View_ID id)
{
  Assert(view_id_is_valid(id));
  View *view = view_from_id(app_state, id);
  Assert(view != 0);
  Assert(view->kind == ViewKind_Lister);
  
  UI_Ctx *ui = &app_state->ui;
  
  Scratch scratch = scratch_begin(0, 0);
  
  UI_Cell *cell_arr = PushArray(scratch.arena, app_state->event_list.count, UI_Cell);
  U32 cell_count = 0;
  
  for(EachNode(Event, event, app_state->event_list.first))
  {
    UI_Cell *cell = (cell_arr + cell_count++);
    cell->label = str8_from_name_chunk_list(scratch.arena, &event->title_chunk_list);
  }
  
  ui_cell_lister(ui, S8("search"), ui_keyf("lister_panel"), 
                 cell_arr, cell_count);
  
  scratch_end(&scratch);
}

void
tags_panel(App_State *app_state, View_ID id)
{
  Assert(view_id_is_valid(id));
  View *view = view_from_id(app_state, id);
  Assert(view != 0);
  Assert(view->kind == ViewKind_Tags);
  
  if(!view->is_initialized)
  {
    view->is_initialized = true;
    view->new_tag_color = color_red;
  }
  
  UI_Ctx *ui = &app_state->ui;
  
  ui_set_next_sizing(ui, ui_grow(), ui_grow());
  ui_set_next_layout_axis(ui, Axis2_Y);
  ui_set_next_background_color(ui, color_black);
  ui_set_next_padding(ui, pad2_equal(10.0f));
  ui_parentf(ui, UIFlags_DrawBackGround, S8Z())
    ui_seed_key(ui, ui_keyf("tags_panel_%d", id))
  {
    V2f color_picker_pos = {0};
    
    UI_Line_Input_State *tag_input = 0;
    if(tag_input != 0)
    {
      ui_set_next_sizing(ui, ui_grow(), ui_fit());
      ui_parentf(ui, 0, S8Z())
      {
        tag_input = ui_line_input(ui, ui_keyf("event_tag_input_%d", id));
        
        // NOTE(erb): color picker trigger
        ui_set_next_seed_key(ui, ui_keyf("color_picker_seed_%d", id));
        ui_set_next_sizing(ui, ui_fixed(30), ui_grow());
        UI_Comm trigger = ui_color_picker_trigger(ui, view->is_color_picker_open, view->new_tag_color);
        
        if(trigger.left_clicked)
        {
          view->is_color_picker_open = !view->is_color_picker_open;
          if(trigger.node)
          {
            Range_V2f rng = node_rng_v2f(trigger.node);
            color_picker_pos = v2f(rng.min.x, rng.max.y+5);
          }
        }
        
        if(ui_buttonf(ui, S8("New Tag##new_tag_button_%d"), id).left_clicked)
        {
          create_event_tag(app_state, tag_input->writer.str, view->new_tag_color);
          
          // NOTE(erb): clear input
          view->new_tag_color = color_red;
          str8_writer_reset(&tag_input->writer);
          
        }
      }
    }
    
    ui_sizing_x(ui, ui_grow())
    {
      // NOTE(erb): tag search
      for(EachElement(Event_Tag, tag, app_state->tags))
      {
        if(tag->used)
        {
          Tag_ID tag_id = id_from_tag(app_state, tag);
          
          String8 name = S8Struct(tag->name, tag->name_size);
          if(str8_find(name, tag_input->writer.str, 
                       MatchFlags_CaseInsens).exists)
          {
            ui_set_next_background(ui, grad2(with_alpha(tag->color, 0.8f), v4ff(0), Orientation2_NWSE));
            ui_set_next_padding(ui, pad2_xy(5, 2));
            ui_parentf(ui, UIFlags_DrawBackGround, S8Z())
              ui_seed_key(ui, ui_keyf("tag_panel_tag_item_%d", tag_id))
            {
              ui_labelf(ui, name);
              
              // NOTE(erb): tag color picker trigger
              ui_set_next_align_x(ui, UIAlign_End);
              ui_set_next_sizing(ui, ui_grow(), ui_grow());
              ui_parentf(ui, 0, S8Z())
              {
                B32 is_open = (tag_id_equals(view->open_color_picker_tag, tag_id) && view->tag_color_picker_is_open);
                
                ui_set_next_sizing(ui, ui_fixed(30), ui_grow());
                UI_Comm comm = ui_color_picker_trigger(ui, is_open, tag->color);
                if(comm.left_clicked)
                {
                  if(tag_id_equals(view->open_color_picker_tag, tag_id))
                  {
                    view->tag_color_picker_is_open = !view->tag_color_picker_is_open;
                  }
                  else
                  {
                    view->open_color_picker_tag = tag_id;
                    view->tag_color_picker_is_open = true;
                  }
                  if(comm.node)
                  {
                    Range_V2f rng = node_rng_v2f(comm.node);
                    color_picker_pos = v2f(rng.min.x, rng.max.y+5);
                  }
                }
              }
            }
          }
        }
      }
    }
    
    // NOTE(erb): tag color picker
    if(tag_id_is_valid(view->open_color_picker_tag) && view->tag_color_picker_is_open)
    {
      Event_Tag *tag = event_tag_from_id(app_state, view->open_color_picker_tag);
      Assert(tag != 0);
      ui_color_picker_popup(ui, color_picker_pos, &view->tag_color_picker_is_open, &tag->color);
    }
    
    // NOTE(erb): floating color picker
    if(view->is_color_picker_open)
    {
      ui_color_picker_popup(ui, color_picker_pos, &view->is_color_picker_open, &view->new_tag_color);
    }
  }
}

void
event_focus_panel(App_State *app_state, View_ID id)
{
  Assert(view_id_is_valid(id));
  View *view = view_from_id(app_state, id);
  Assert(view != 0);
  Assert(view->kind == ViewKind_EventFocus);
  
  Event *event = 0;
  if(!view->is_initialized)
  {
    view->is_initialized = true;
    view->new_tag_color = color_red;
    if(event_id_is_null(view->event_id))
    {
      Date_Time dt = {0};
      dt.date = os_get_local_date_time().date;
      event = make_event(app_state, S8Z(), dt, dt);
      view->event_id = event->id;
    }
  }
  else
  {
    // TODO(erb): make this lookup better?
    Assert(!event_id_is_null(view->event_id));
    for(EachNode(Event, test_ev, app_state->event_list.first))
    {
      if(event_id_equals(test_ev->id, view->event_id))
      {
        event = test_ev;
        break;
      }
    }
    Assert(event != 0);
  }
  
  UI_Ctx *ui = &app_state->ui;
  ui_seed_key(ui, ui_keyf("event_focus_panel_%d", id))
  {
    ui_focus_event(ui, app_state, event, 
                   &view->is_color_picker_open, 
                   &view->new_tag_color,
                   &view->date_input_state);
  }
}

void
spawn_test_tasks(App_State *app_state)
{
  Date_Time today = os_get_local_date_time();
  {
    make_event(app_state, S8("ONE task this is a very long task very very long very very very very long long long so we try to do some breaking!!!!!!!!!!!!!!!"), today, today);
  }
  {
    Date_Time dt = {0};
    dt.date = cal_date_add_days(today.date, 1);
    make_event(app_state, S8("TWO task"), 
               dt, dt);
  }
  {
    Date_Time start = today;
    start.date = cal_date_add_days(today.date, 2);
    
    Date_Time end = {0};
    end.time = start.time;
    end.date = cal_date_add_days(start.date, 1);
    
    make_event(app_state, S8("THREE task"), start, end);
  }
}

void
toggle(UI_Ctx *ui, String8 label, B32 *value)
{
  Assert(ui);
  Assert(label.size > 0);
  Assert(value);
  ui_set_next_padding(ui, pad2_xy(10, 3));
  ui_set_next_sizing_x(ui, ui_grow());
  ui_set_next_background_color(ui, (*value ? color_green : color_red));
  ui_set_next_font_color(ui, (*value ? color_black : color_white));
  if(ui_buttonf(ui, label).left_clicked)
  {
    *value = !*value;
  }
}

#define toggle_var(ui, debug_ctx, value_label) toggle((ui), S8(#value_label), &(debug_ctx)->value_label)

void
debug_context(App_State *app_state)
{
  UI_Ctx *ui = &app_state->ui;
  
  V2f pos = v2f(ui->root->pos.x, ui->root->pos.y+ui->root->size.y-50);
  ui_set_next_layout_axis(ui, Axis2_Y);
  ui_set_next_floating_pos(ui, pos);
  ui_set_next_background_color(ui, with_alpha(v4ff(0.2f), 1));
  ui_set_next_padding(ui, pad2_equal(5));
  UI_Node *container = ui_nodef(ui, UIFlags_DrawBackGround
                                |UIFlags_Comm
                                |UIFlags_Floating, 
                                S8("debug_%p"), &app_state->debug_ctx);
  UI_Comm out = ui_comm_from_node(ui, container);
  if(out.node != 0)
  {
    container->pos = out.node->pos; // NOTE(erb): keep the position through frames
    if(out.left_dragging)
    {
      container->pos = v2f_add(container->pos ,ui->input.cursor_delta);
    }
  }
  
  ui_parent(ui, container) ui_seed_key(ui, container->key)
  {
    Debug_Ctx *debug = &app_state->debug_ctx;
    
    ui_set_next_layout_axis(ui, Axis2_X);
    ui_set_next_align_y(ui, UIAlign_Center);
    ui_parentf(ui, 0, S8Z())
    {
      ui_set_next_font_color(ui, color_white);
      ui_labelf(ui, S8("Debug Context"));
      toggle(ui, S8("Expand"), &debug->is_expanded);
    }
    
    if(debug->is_expanded)
    {
      ui_sizing_x(ui, ui_grow())
      {
        toggle_var(ui, debug, draw_text_highlight);
        toggle_var(ui, debug, draw_outer_size_line);
        toggle_var(ui, debug, draw_inner_size_line);
        
        if (ui_buttonf(ui, S8("spawn")).left_clicked)
        {
          spawn_test_tasks(app_state);
        }
      }
    }
  }
}

typedef struct Panel_Split_Result Panel_Split_Result;
struct Panel_Split_Result
{
  Panel *inherit_child;
  Panel *new_child;
};

Panel_Split_Result
panel_split_inherit(Arena *arena, Panel **first_free, Panel *parent, Direction2 dest_direction)
{
  Panel *min = push_panel(arena, first_free, parent);
  Panel *max = push_panel(arena, first_free, parent);
  min->pct_of_parent = max->pct_of_parent = 0.5f;
  parent->split_axis = dir2_axis(dest_direction);
  
  Panel_Split_Result result = {0};
  if(dest_direction==Direction2_North || dest_direction==Direction2_West)
  {
    result.new_child = min;
    result.inherit_child = max;
  }
  else
  {
    result.new_child = max;
    result.inherit_child = min;
  }
  result.inherit_child->current_view = parent->current_view;
  for(EachNode(Tab, tab, parent->first_tab))
  {
    panel_push_unique_tab(result.inherit_child, tab->view);
  }
  mem_zero((U8 *)parent->tab_storage, ArraySize(parent->tab_storage)*sizeof(Tab));
  parent->first_tab = 0;
  parent->last_tab = 0;
  parent->first_free_tab = 0;
  parent->current_view = view_id_null();
  parent->is_picking_new_tab = false;
  return(result);
}

void
parent_panel_borders(App_State *app_state, Range_V2f client_rect, F32 split_region_half_width)
{
  UI_Ctx *ui = &app_state->ui;
  
  for(Panel *panel = app_state->root_panel; panel != 0; 
      panel = panel_rec_depth_first_pre_order(panel).next)
  {
    Range_V2f panel_rect = rect_from_panel(panel, client_rect);
    V2f panel_rect_size = rng_v2f_size(panel_rect);
    F32 panel_rect_axis_size = panel_rect_size.c[panel->split_axis];
    
    for(EachNode(Panel, child, panel->first))
    {
      if(child->next == 0)
      {
        break;
      }
      Range_V2f child_rect = rect_from_panel_child_rect(child, panel_rect);
      
      // NOTE(erb): regulard drag borders
      Range_V2f split_region_rect = child_rect;
      split_region_rect.min.c[panel->split_axis] = split_region_rect.max.c[panel->split_axis];
      split_region_rect.min.c[panel->split_axis] -= split_region_half_width;
      split_region_rect.max.c[panel->split_axis] += split_region_half_width;
      
      ui_set_next_rect(ui, split_region_rect);
      UI_Node *split_region = ui_nodef(ui, UIFlags_Comm|UIFlags_Floating, S8("split_region_%p"), child);
      ui_parent(ui, split_region);
      
      UI_Comm comm = ui_comm_from_node(ui, split_region);
      if(comm.hovered)
      {
        GFX_Cursor_Kind cursor_kind = (panel->split_axis==Axis2_X ? GFXCursorKind_ArrowWE : GFXCursorKind_ArrowNS);
        ui->cursor_api.set_func(cursor_kind, ui->cursor_api.data);
      }
      if(comm.left_dragging) // NOTE(erb): drag
      {
        F32 drag_delta = ui->input.cursor_delta.c[panel->split_axis];
        Panel *min_child = child;
        Panel *max_child = child->next;
        F32 min_child_px__pre_drag = min_child->pct_of_parent * panel_rect_axis_size;
        F32 max_child_px__pre_drag = max_child->pct_of_parent * panel_rect_axis_size;
        F32 min_child_px__post_drag = min_child_px__pre_drag + drag_delta;
        F32 max_child_px__post_drag = max_child_px__pre_drag - drag_delta;
        min_child->pct_of_parent = min_child_px__post_drag/panel_rect_axis_size;
        max_child->pct_of_parent = max_child_px__post_drag/panel_rect_axis_size;
      }
    }
  }
}

UI_Comm
ui_rect_indicator(UI_Ctx *ui, UI_Key key, Range_V2f rect, 
                  Range_V2f rect_hover, V4f bg_color, V4f border_color, 
                  F32 border_thickness,  F32 corner_radius, F32 corner_radius_hover)
{
  ui_set_next_rect(ui, rect);
  ui_set_next_background_color(ui, bg_color);
  ui_set_next_border_abs(ui, border_color, border_thickness, corner_radius);
  UI_Node *node = ui_node_make(ui, 
                               UIFlags_DrawBackGround
                               |UIFlags_DrawBorder
                               |UIFlags_Comm
                               |UIFlags_DrawRounded
                               |UIFlags_Floating, 
                               key, S8Z());
  ui_parent(ui, node);
  UI_Comm comm = ui_comm_from_node(ui, node);
  if(comm.hovered)
  {
    node->pos = rect_hover.min;
    node->size = rng_v2f_size(rect_hover);
    node->corner_radius = rad_abs(corner_radius_hover);
  }
  return(comm);
}

void
panel_move_ui_and_do(App_State *app_state, Range_V2f client_rect, F32 split_region_half_width)
{
  UI_Ctx *ui = &app_state->ui;
  
  V4f bg_color = with_alpha(color_red, 0.2f);
  V4f border_color = color_red;
  F32 border_thickness = 5.0f;
  F32 corner_radius = 8.0f;
  F32 corner_radius_hover = 20.0f;
  F32 size_pct_hover = 0.8f;
  
  Panel *dest_panel = 0;
  Panel *source_panel = 0;
  
  B32 do_move = view_id_is_valid(app_state->moving_view) && mouse_button_released(ui, GFXMouseCode_Left);
  
  if(view_id_is_valid(app_state->moving_view))
  {
    for(Panel *panel = app_state->root_panel; panel != 0; 
        panel = panel_rec_depth_first_pre_order(panel).next)
    {
      if(dest_panel != 0 && source_panel != 0)
      {
        break;
      }
      
      Range_V2f panel_rect = rect_from_panel(panel, client_rect);
      if(panel->first == 0)
      {
        V2f center = rng_v2f_center(panel_rect);
        V2f panel_rect_size = rng_v2f_size(panel_rect);
        V2f size = v2ff(Max(ui->root->size.x, ui->root->size.y)*0.05f); // NOTE(erb): relative to ui root
        V2f size_hover = v2f_scale(panel_rect_size, size_pct_hover);
        
        B32 contains_move_view = (panel_tab_from_view(panel, app_state->moving_view) != 0);
        B32 center_panel_hovered = false;
        if(!contains_move_view)
        {
          UI_Comm center_panel = ui_rect_indicator(ui, ui_keyf("center_panel_leaf_indicator_%p", panel),
                                                   rng_v2f_center_size(center, size), rng_v2f_center_size(center, size_hover), 
                                                   bg_color, border_color, border_thickness, corner_radius, corner_radius_hover);
          center_panel_hovered = center_panel.hovered;
        }
        
        B32 has_panel_direction = false;
        Direction2 panel_direction = 0;
        for(U32Each(dir, Direction2_COUNT))
        {
          F32 padding = 5.0f;
          Axis2 axis = dir2_axis(dir);
          B32 should_align_min = (dir==Direction2_North || dir==Direction2_West);
          
          F32 offset = size.c[axis]+padding;
          F32 length = panel_rect_size.c[axis]*0.4f;
          
          V2f region_center = center;
          region_center.c[axis] += (should_align_min ? -offset : +offset);
          
          Range_V2f rect_hover = panel_rect;
          if(should_align_min)
          {
            rect_hover.max.c[axis] = panel_rect.min.c[axis] + length;
          }
          else
          {
            rect_hover.min.c[axis] = panel_rect.max.c[axis] - length;
          }
          UI_Comm comm = ui_rect_indicator(ui, ui_keyf("dir_panel_leaf_indicator_%p_%d", panel, dir),
                                           rng_v2f_center_size(region_center, size), rect_hover, 
                                           bg_color, border_color, border_thickness, corner_radius, corner_radius_hover);
          if(comm.hovered)
          {
            // NOTE(erb): destination
            Assert(!has_panel_direction);
            has_panel_direction = true;
            panel_direction = dir;
          }
        }
        
        if(do_move)
        {
          // NOTE(erb): determine destination and source panels
          if(has_panel_direction)
          {
            Assert(dest_panel == 0);
            Assert(source_panel == 0);
            Panel_Split_Result split = panel_split_inherit(&app_state->panel_arena, &app_state->first_free_panel,
                                                           panel, panel_direction);
            source_panel = split.inherit_child;
            dest_panel = split.new_child;
          }
          else
          {
            B32 might_be_target = (rng_v2f_contains_inclusive(panel_rect, ui->input.cursor) 
                                   && panel->first_tab == 0);
            if(center_panel_hovered || view_id_is_valid(panel->move_view_ref) || might_be_target)
            {
              AssertImplies(center_panel_hovered || view_id_is_valid(panel->move_view_ref), 
                            dest_panel == 0);
              dest_panel = panel;
            } 
            if(contains_move_view)
            {
              Assert(source_panel == 0);
              source_panel = panel;
            }
          }
        }
      }
      else
      {
#if 0
        F32 size_pct = 0.3f;
        B32 is_last_repeat = false;
        // NOTE(erb): panel to panel indicators
        for(EachNode(Panel, child, panel->first))
        {
          repeat_draw:;
          
          Range_V2f child_rect = rect_from_panel_child_rect(child, panel_rect);
          
          V2f child_rect_center = rng_v2f_center(child_rect);
          V2f child_rect_size = rng_v2f_size(child_rect);
          
          Axis2 axis = panel->split_axis;
          Axis2 cross = (panel->split_axis+1)%2;
          
          // NOTE(erb): draw the indicator at the left (at child rect min) always, and
          // in the end also draw it on the right so we get all the borders
          V2f center = {0};
          center.c[axis] = (is_last_repeat ? child_rect.max.c[axis] : child_rect.min.c[axis]);
          center.c[cross] = child_rect_center.c[cross];
          
          V2f size = {0};
          size.c[axis] = ui->root->size.c[axis]*0.1f; // NOTE(erb): relative to ui root
          size.c[cross] = child_rect_size.c[cross]*size_pct;
          
          V2f size_hover = {0};
          size_hover.c[axis] = ui->root->size.c[axis]*0.15f;
          size_hover.c[cross] = child_rect_size.c[cross]*size_pct_hover;
          
          ui_rect_indicator(ui, ui_keyf(S8("child_border_indicator_%p_%d"), child, is_last_repeat),
                            rng_v2f_center_size(center, size), rng_v2f_center_size(center, size_hover), 
                            bg_color, border_color, border_thickness, corner_radius, corner_radius_hover);
          
          // NOTE(erb): repeat one more time at the very end so we get the right-most border
          if(!is_last_repeat && child->next == 0)
          {
            is_last_repeat = true;
            goto repeat_draw;
          }
        }
#endif
      }
    }
  }
  
  if(dest_panel && source_panel)
  {
    // NOTE(erb): add to destination
    B32 moved = panel_move_or_insert_view_set_current(dest_panel, app_state->moving_view, 
                                                      dest_panel->move_view_ref, 
                                                      dest_panel->move_view_before_ref);
    // NOTE(erb): remove from source
    if(!moved)
    {
      panel_remove_view_reset_current(source_panel, app_state->moving_view);
    }
  }
  
  if(do_move)
  {
    Assert(view_id_is_valid(app_state->moving_view));
    app_state->moving_view = view_id_null();
  }
}
typedef enum Tab_Button_Kind
{
  TabButtonKind_Regular,
  TabButtonKind_DragDest,
  TabButtonKind_DragSource,
} Tab_Button_Kind;

typedef struct Tab_Button_Result Tab_Button_Result;
struct Tab_Button_Result
{
  B32 select_or_drag;
  B32 remove;
};

Tab_Button_Result
draw_tab_button_kind(Tab_Button_Kind kind, View_ID view, App_State *app_state, Panel *panel, 
                     UI_Key tab_button_container_key, B32 is_current_selected)
{
  UI_Ctx *ui = &app_state->ui;
  
  V4f font_color = {0};
  UI_Node *container = 0;
  
  switch(kind)
  {
    case TabButtonKind_Regular:
    {
      font_color = (is_current_selected ? color_black : color_white);
      V4f bg_color = (is_current_selected ? color_yellow : with_alpha(v4ff(0.4f), 1.0f));
      
      ui_set_next_background_color(ui, bg_color);
      ui_set_next_debug_str(ui, S8("regular-tab-container"));
      container = ui_node_make(ui, 
                               UIFlags_DrawBackGround
                               |UIFlags_DontSeedKey, 
                               tab_button_container_key, S8Z());
    } break;
    
    case TabButtonKind_DragSource:
    {
      font_color = with_alpha(v4ff(0.3f), 0.8f);
      ui_set_next_border_abs(ui, color_yellow, 4.0f, 3.0f);
      ui_set_next_background_color(ui, with_alpha(v4ff(0.6f), 0.6f));
      ui_set_next_debug_str(ui, S8("original-ghost-tab-container"));
      container = ui_node_make(ui, 
                               UIFlags_DrawBackGround
                               |UIFlags_DrawBorder
                               |UIFlags_DrawRounded
                               |UIFlags_DontSeedKey, 
                               tab_button_container_key, S8Z());
    } break;
    
    case TabButtonKind_DragDest:
    {
      font_color = color_blank;
      ui_set_next_border_abs(ui, color_blue, 5.0f, 5.0f);
      ui_set_next_background_color(ui, color_white);
      container = ui_nodef(ui, 
                           UIFlags_DrawBackGround
                           |UIFlags_DrawBorder
                           |UIFlags_DrawRounded, 
                           S8("tab_dest_ghost_%llu"), view);
    } break;
    
    InvalidDefaultCase;
  }
  
  Tab_Button_Result result = {0};
  
  ui_parent(ui, container)
    ui_font_color(ui, font_color)
    ui_border_abs(ui, v4ff(0), 0, 0) 
    ui_background_color(ui, color_blank)
  {
    View_Kind view_kind = view_kind_from_id(app_state, view);
    
    UI_Comm label = ui_buttonf(ui, S8("%S##tab_button_%llu"), label_from_view_kind(view_kind), view);
    UI_Comm remove = ui_buttonf(ui, S8("[X]##tab_del_%llu"), view);
    
    result.select_or_drag = (label.left_clicked || label.left_dragging);
    result.remove = remove.left_clicked;
  }
  return(result);
}

void
panel_leaf_ui(App_State *app_state, Range_V2f client_rect, F32 split_region_half_width)
{
  UI_Ctx *ui = &app_state->ui;
  Scratch scratch = scratch_begin(0, 0);
  
  for(Panel *panel = app_state->root_panel; panel != 0; 
      panel = panel_rec_depth_first_pre_order(panel).next)
  {
    if(panel->first == 0)
    {
      V2f new_tab_button_bottom_left = {0};
      
      AssertImplies(panel->first_tab == 0, panel->last_tab == 0);
      AssertImplies(panel->last_tab == 0, panel->first_tab == 0);
      
      panel->move_view_ref = view_id_null();
      
      Range_V2f panel_rect = rect_from_panel(panel, client_rect);
      B32 cursor_in_panel = rng_v2f_contains_inclusive(panel_rect, ui->input.cursor);
      
      ui_set_next_border_color(ui, color_blank);
      ui_set_next_rect(ui, rng_v2f_center_sub(panel_rect, v2ff(split_region_half_width)));
      ui_set_next_layout_axis(ui, Axis2_Y);
      ui_set_next_child_gap(ui, 0);
      UI_Node *panel_space = ui_nodef(ui, 
                                      UIFlags_Floating
                                      |UIFlags_ClipRectangle
                                      |UIFlags_DrawBorder
                                      |UIFlags_Comm, S8("panel_%p"), panel);
      if(ui_is_top_nav_interface(ui, panel_space->key))
      {
        panel_space->border_color = color_red;
        panel_space->border_thickness = 2.0f;
      }
      
      B32 was_picking_new_tab = panel->is_picking_new_tab;
      UI_Comm new_tab_comm = {0};
      
      ui_parent(ui, panel_space)
        ui_font_color(ui, color_white) 
        ui_seed_key(ui, panel_space->key)
      {
        // NOTE(erb): tab buttons
        ui_set_next_sizing_x(ui, ui_grow());
        ui_set_next_child_gap(ui, 4);
        ui_parentf(ui, 0, S8Z())
        {
          B32 mouse_is_past_middle = false;
          B32 mouse_is_before_middle = false;
          B32 mouse_is_past_prev_middle = false;
          
          if(panel->first_tab == 0)
          {
            if(view_id_is_valid(app_state->moving_view))
            {
              draw_tab_button_kind(TabButtonKind_DragDest, app_state->moving_view, app_state, panel, 
                                   ui_keyf("fake_tab_move_dest_%p", panel), false);
            }
          }
          else
          {
            for(Tab *tab = panel->first_tab; tab;)
            {
              View_ID view = tab->view;
              Assert(view_id_is_valid(view));
              
              B32 remove = false;
              ui_set_next_padding(ui, pad2_equal(0));
              ui_set_next_child_gap(ui, 0);
              ui_parentf(ui, 0, S8Z())
                ui_seed_key(ui, ui_keyf("tab_buttons_%p", tab));
              {
                UI_Key tab_button_container_key = ui_keyf("tab_button_container_%p", tab);
                
                B32 is_not_dragging_tab = (view_id_is_valid(app_state->moving_view) && !view_id_equals(view, app_state->moving_view));
                
                if(view_id_is_valid(app_state->moving_view) && cursor_in_panel)
                {
                  UI_Node *cached = ui_node_from_key(ui, tab_button_container_key);
                  if(cached)
                  {
                    Range_V2f rect = node_rng_v2f(cached);
                    Range_F32 tab_container_y = rng_v2f_axis(rect, Axis2_Y);
                    if(rng_f32_contains_inclusive(tab_container_y, ui->input.cursor.y))
                    {
                      F32 half_point_x = rng_v2f_center(rect).x;
                      mouse_is_past_prev_middle = (tab == panel->first_tab || mouse_is_past_middle);
                      mouse_is_past_middle = (ui->input.cursor.x >= half_point_x);
                      mouse_is_before_middle = (ui->input.cursor.x <= half_point_x);
                    }
                  }
                }
                
                B32 is_selected_tab = view_id_equals(panel->current_view, view);
                
                // NOTE(erb): draw the dragging tab destination - BEFORE this button
                // regular case
                if(is_not_dragging_tab
                   && (tab->prev == 0 || view_id_equals(app_state->moving_view, tab->prev->view))
                   && mouse_is_before_middle
                   && mouse_is_past_prev_middle)
                {
                  panel->move_view_ref = tab->view;
                  panel->move_view_before_ref = true;
                  draw_tab_button_kind(TabButtonKind_DragDest, view, app_state, panel, 
                                       tab_button_container_key, is_selected_tab);
                }
                
                // NOTE(erb): draw the actuall tab (or original drag ghost)
                {
                  Tab_Button_Kind tab_kind = (view_id_equals(app_state->moving_view, view)
                                              ? TabButtonKind_DragSource : TabButtonKind_Regular);
                  Tab_Button_Result button_result = draw_tab_button_kind(tab_kind, view, app_state, panel, 
                                                                         tab_button_container_key, is_selected_tab);
                  // NOTE(erb): start global view/tab drag
                  if(button_result.select_or_drag)
                  {
                    panel->current_view = view;
                    app_state->moving_view = view;
                  }
                  remove = button_result.remove;
                }
                
                // NOTE(erb): draw the dragging tab destination - AFTER this button 
                // special case
                if(is_not_dragging_tab
                   && mouse_is_past_middle
                   && tab == panel->last_tab)
                {
                  panel->move_view_ref = tab->view;
                  panel->move_view_before_ref = false;
                  draw_tab_button_kind(TabButtonKind_DragDest, view, app_state, panel, 
                                       tab_button_container_key, is_selected_tab);
                }
                
              }
              
              tab = tab->next;
              if(remove) // NOTE(erb): removal
              {
                panel_remove_view_reset_current(panel, view);
              }
            }
          }
          
          // NOTE(erb): new tab button
          {
            if(panel->is_picking_new_tab)
            {
              ui_set_next_background_color(ui, with_alpha(v4ff(0.2f), 1.0f));
            }
            new_tab_comm = ui_buttonf(ui, S8("New##new_tab"), panel);
            if(new_tab_comm.left_clicked)
            {
              panel->is_picking_new_tab = true;
            }
            if(new_tab_comm.node != 0)
            {
              Range_V2f rng = node_rng_v2f(new_tab_comm.node);
              new_tab_button_bottom_left = v2f(rng.min.x, rng.max.y);
            }
          }
        }
        
        // NOTE(erb): leaf ui
        ui_set_next_layout_axis(ui, Axis2_Y);
        ui_set_next_border_abs(ui, v4f_lerp(color_white, color_yellow, 0.5f), 1, 5);
        ui_set_next_sizing(ui, ui_grow(), ui_grow());
        ui_parentf(ui, 
                   UIFlags_DrawBackGround
                   |UIFlags_DrawRounded
                   |UIFlags_DrawBorder, S8Z())
          ui_border_color(ui, color_white)
          ui_background_color(ui, color_black)
        {
          // NOTE(erb): view
          AssertImplies(panel->first_tab != 0, view_id_is_valid(panel->current_view));
          if(view_id_is_valid(panel->current_view))
          {
            Assert(panel_tab_from_view(panel, panel->current_view) != 0);
            View_Kind kind = view_kind_from_id(app_state, panel->current_view);
            switch(kind)
            {
              case ViewKind_Lister    :      lister_panel(app_state, panel, panel->current_view); break;
              case ViewKind_Calendar  :    calendar_panel(app_state, panel->current_view); break;
              case ViewKind_Table     :       table_panel(app_state, panel->current_view); break;
              case ViewKind_Watch     :       watch_panel(app_state, panel->current_view); break;
              case ViewKind_EventFocus: event_focus_panel(app_state, panel->current_view); break;
              case ViewKind_Tags      :        tags_panel(app_state, panel->current_view); break;
            }
          }
          
          if(panel->first_tab == 0 && panel->parent != 0)
          {
            Assert(panel->last_tab == 0);
            if(ui_buttonf(ui, S8("Close")).left_clicked)
            {
              release_panel(panel, &app_state->first_free_panel);
            }
          }
        }
      }
      
      B32 popup_open_event = new_tab_comm.left_clicked || (key_pressed(ui, GFXKey_T) && key_down(ui, GFXKey_Control));
      if(!panel->is_picking_new_tab && popup_open_event)
      {
        panel->is_picking_new_tab = true;
      }
      
      UI_Key tab_popup_key = ui_keyf("tab_panel_popup_%p", panel);
      
      if(panel->is_picking_new_tab)
      {
        UI_Cell *cell_arr = PushArray(scratch.arena, ViewKind_COUNT, UI_Cell);
        U32 cell_idx = 0;
        for(U32Each(kind, ViewKind_COUNT))
        {
          UI_Cell *cell = (cell_arr + cell_idx++);
          cell->label = label_from_view_kind((View_Kind)kind);
          cell->filter_needle = cell->label;
          cell->flags = UICellFlags_Clickable;
          cell->data = (void *)(U64)kind;
        }
        
        if(!was_picking_new_tab)
        {
          ui_push_nav_interface_key(ui, tab_popup_key);
        }
        
        UI_Lister_Popup_Result result = ui_cell_lister_floating_popup(ui, new_tab_button_bottom_left, tab_popup_key, cell_arr, ViewKind_COUNT, S8("Tab"));
        
        if(result.lister.selected != 0)
        {
          // NOTE(erb): actually create the tab
          View_Kind picked_kind = (View_Kind)(U64)result.lister.selected->data;
          View_ID view = push_view(app_state, (View_Kind)picked_kind);
          panel_append_view_set_current(panel, view);
          panel->is_picking_new_tab = false;
        }
        if(result.pressed_out)
        {
          panel->is_picking_new_tab = false;
          ui_pop_nav_interface_key(ui);
        }
      }
    }
  }
  scratch_end(&scratch);
}

typedef struct UI_Node_Rec UI_Node_Rec;
struct UI_Node_Rec
{
  UI_Node *next;
  U32 push_count;
  U32 pop_count;
};

UI_Node_Rec
ui_node_rec_depth_first_pre_order(UI_Node *node)
{
  UI_Node_Rec rec = {0};
  if(node != 0)
  {
    if(node->next != 0)
    {
      rec.next = node->first_child;
      rec.push_count += 1;
    }
    else for(UI_Node *n = node; n != 0; n = n->parent)
    {
      if(n->next != 0)
      {
        rec.next = n->next;
        break;
      }
      rec.pop_count += 1;
    }
  }
  return(rec);
}

Directory_Entry_List *
dir_entr_ht_get(Directory_Entries_Hash_Table *ht, String8 path)
{
  Directory_Entry_List *result = 0;
  U32 idx = (U32)(str8_hash(path) & (ArraySize(ht->slots)-1));
  Directory_Entry_List *first = (ht->slots + idx);
  for(EachNode(Directory_Entry_List, slot, first))
  {
    if(str8_equals(slot->path, path))
    {
      result = slot;
      break;
    }
  }
  return(result);
}

Directory_Entry_List *
dir_entr_ht_add(Arena *arena, Directory_Entries_Hash_Table *ht, Directory_Entry_List *new_entries)
{
  Directory_Entry_List *result = 0;
  U32 idx = (U32)(str8_hash(new_entries->path) & (ArraySize(ht->slots)-1));
  Directory_Entry_List *first = (ht->slots + idx);
  
  for(EachNode(Directory_Entry_List, slot, first))
  {
    if(str8_equals(slot->path, new_entries->path))
    {
      result = slot;
      break;
    }
  }
  
  if(result == 0)
  {
    if(first->path.size != 0)
    {
      first = first->next = PushStruct(arena, Directory_Entry_List);
    }
    *first = *new_entries;
    result = first;
  }
  else InvalidCodePath;
  return(result);
}

B32
cmd_kind_is_valid(Command_Kind kind)
{
  B32 result = (kind > CommandKind_None && kind < CommandKind_COUNT);
  return(result);
}

Key_Combo_Set *
key_map_from_cmd_kind(App_State *app_state, Command_Kind kind)
{
  Assert(cmd_kind_is_valid(kind));
  Key_Combo_Set *set = (app_state->key_maps + kind);
  return(set);
}

////////////////////////////////////////////////////////////////
// NOTE(erb): commands

String8
ui_file_lister(App_State *app_state, Arena *arena)
{
  String8 result = {0};
  
  Scratch scratch = scratch_begin(&arena, 1);
  
  UI_Ctx *ui = &app_state->ui;
  
  ui_center_popup(ui)
  {
    ui_cell_lister_begin(ui, ui_keyf("file_lister"));
    {
      UI_Line_Input_State *input = ui_cell_lister_input(ui, S8("open"));
      if(ui_line_input_created(ui, input))
      {
        String8 dir = app_state->current_directory;
        str8_writer_str(&input->writer, dir);
        if(dir.value[dir.size] != os_path_separator_char)
        {
          str8_writer_char(&input->writer, os_path_separator_char, 1);
        }
      }
      
      // NOTE(erb): parse path and search
      String8 input_str = input->writer.str;
      Index_U64 term_sep_idx = str8_index(input_str, os_path_separator_char, 
                                          IndexFlags_ValuePlusOne|IndexFlags_Backward);
      String8 path_term = str8_prefix(input_str, term_sep_idx.value);
      String8 search_term = str8_postfix(input_str, term_sep_idx.value);
      
      // NOTE(erb): get or add to cache
      Directory_Entry_List *entries = dir_entr_ht_get(&app_state->dir_entries_cache, path_term);
      if(entries == 0)
      {
        Directory_Entry_List new_entries = os_get_directory_entries(&app_state->directory_arena, path_term);
        entries = dir_entr_ht_add(&app_state->directory_arena, 
                                  &app_state->dir_entries_cache, &new_entries);
      }
      
      UI_Cell *cell_arr = PushArray(scratch.arena, entries->count, UI_Cell);
      U32 cell_count = 0;
      for(EachNode(Directory_Entry, entry, entries->first))
      {
        // NOTE(erb): this filtering "leaks" the two entries, 
        // so the cell array is not one to one, shouldn't matter
        if(!str8_equals(entry->path, S8(".")) && 
           !str8_equals(entry->path, S8("..")))
        {
          UI_Cell *cell = (cell_arr + cell_count++);
          
          cell->label = entry->path;
          cell->data = entry;
          cell->flags = UICellFlags_Clickable;
          cell->filter_needle = entry->path;
          if(entry->kind == FileKind_Directory)
          {
            cell->flags |= UICellFlags_DrawExpandArrowClose;
          }
        }
      }
      
      UI_Key scroll_key = ui_keyf("scroll_movable");
      UI_Node *scroll = ui_node_from_key(ui, scroll_key);
      F32 scroll_offset = (scroll != 0) ? scroll->scroll_offset : 0.0f;
      
      UI_Cell *selected = 0;
      ui_cell_lister_content_begin(ui);
      {
        UI_Lister_Items_Result items_result = ui_cell_lister_items(ui, cell_arr, cell_count, search_term, scroll_offset, false);
        selected = items_result.selected;
        ui_cell_lister_scroll(ui, scroll_key, 
                              items_result.view_height, 
                              items_result.list_total_height, 
                              items_result.list_item_height,
                              items_result.scroll_clicks_y);
      }
      ui_cell_lister_content_end(ui);
      
      if(selected != 0)
      {
        Directory_Entry *entry = (Directory_Entry *)selected->data;
        Assert(entry != 0);
        
        // NOTE(erb): edit the path
        if(entry->kind == FileKind_Directory)
        {
          str8_writer_reset(&input->writer);
          str8_writer_str(&input->writer, path_term);
          str8_writer_str(&input->writer, entry->path);
          str8_writer_char(&input->writer, os_path_separator_char, 1);
        }
        else if(entry->kind == FileKind_Text)
        {
          result = str8_concat(arena, path_term, entry->path);
        }
        else InvalidCodePath;
      }
      else if(key_pressed(ui, GFXKey_Enter))
      {
        result = push_str8_copy(arena, input->writer.str);
      }
    }
    ui_cell_lister_end(ui);
  }
  scratch_end(&scratch);
  return(result);
}

void
app_run_command(App_State *app_state, Command_Kind kind)
{
  // NOTE(erb): check if its allready processing
  B32 exists_and_running = false;
  for(EachNode(Command_Event, event, app_state->first_event))
  {
    if(event->kind == kind && event->not_first_time)
    {
      exists_and_running = true;
      break;
    }
  }
  
  if(!exists_and_running)
  {
    Command_Event *event = app_state->first_free_event;
    if(event != 0)
    {
      SLLStackPop(app_state->first_free_event);
      ZeroStruct(event);
    }
    else
    {
      event = PushStruct(&app_state->event_arena, Command_Event);
    }
    DLLQueueAppend(app_state->first_event, app_state->last_event, event);
    event->kind = kind;
  }
}

COMMAND_FUNC(load_file)
{
  UI_Ctx *ui = &app_state->ui;
  B32 handled = false;
  Scratch scratch = scratch_begin(0, 0);
  
  if(key_pressed(ui, GFXKey_Escape))
  {
    handled = true;
  }
  else
  {
    String8 path_result = ui_file_lister(app_state, scratch.arena);
    if(path_result.size != 0)
    {
      // NOTE(erb): dump the current state
      app_state->load_from_file_path = path_result;
      load_state(app_state);
      handled = true;
    }
  }
  scratch_end(&scratch);
  return(handled);
}

COMMAND_FUNC(save_to_file)
{
  UI_Ctx *ui = &app_state->ui;
  B32 handled = false;
  Scratch scratch = scratch_begin(0, 0);
  
  if(key_pressed(ui, GFXKey_Escape))
  {
    handled = true;
  }
  else
  {
    String8 path_result = ui_file_lister(app_state, scratch.arena);
    if(path_result.size != 0)
    {
      app_state->save_to_file_path = path_result;
      save_state(app_state);
      handled = true;
    }
  }
  scratch_end(&scratch);
  return(handled);
}


COMMAND_FUNC(generate_stub_file)
{
  Scratch scratch = scratch_begin(0, 0);
  
  String8_Builder sb = str8_builder_make_default();
  str8_builderf(scratch.arena, &sb, "global_id_counter: %llu\n", 1);
  
  Date_Time today = os_get_local_date_time();
  
  for(U32Each(idx, 20))
  {
    str8_builderf(scratch.arena, &sb,
                  "event:\n"
                  "{\n"
                  "  id  : %llu\n"
                  "  name: \"%S-%d\"\n"
                  "  start:\n"
                  "  {\n"
                  "    date: \"%02u/%02u/%u\"\n"
                  "    time: \"%02u:%02u\"\n"
                  "  }\n"
                  "  end:\n"
                  "  {\n"
                  "    date: \"%02u/%02u/%u\"\n"
                  "    time: \"%02u:%02u\"\n"
                  "  }\n"
                  , idx,
                  S8("test"), idx,
                  today.date.day, today.date.month, today.date.year,
                  today.time.hour, today.time.minute, 
                  today.date.day, today.date.month, today.date.year,
                  today.time.hour, today.time.minute);
    
#if 0
    for(U32Each(idx, event->tag_count))
    {
      str8_builderf(scratch.arena, &sb, "   tag: %llu\n", event->tags[idx]);
    }
#endif
    str8_builder_str(scratch.arena, &sb, S8("}\n"));
  }
  
#if 0
  for(EachElement(Event_Tag, tag, app_state->tags))
  {
    if(tag->used)
    {
      str8_builderf(scratch.arena, &sb, 
                    "tag:\n"
                    "{\n"
                    "  id   : %llu\n"
                    "  name : \"%S\"\n"
                    "  color: \"%x\"\n"
                    "}\n", 
                    id_from_tag(app_state, tag),
                    S8Struct(tag->name, tag->name_size), 
                    rgb_to_packed_u32(rgb_from_rgba(tag->color)));
    }
  }
#endif
  
  String8 file_path = S8("stub_project.rose");
  app_state->write_entire_file_str8(file_path, push_str8_from_builder(scratch.arena, &sb));
  
  B32 handled = true;
  scratch_end(&scratch);
  return(handled);
}

Command_Info *cmd_info_from_kind(Command_Kind kind);

COMMAND_FUNC(command_launcher)
{
  UI_Ctx *ui = &app_state->ui;
  B32 handled = false;
  
  if(key_pressed(ui, GFXKey_Escape))
  {
    handled = true;
  }
  else
  {
    Scratch scratch = scratch_begin(0, 0);
    UI_Cell *cell_arr = PushArray(scratch.arena, CommandKind_COUNT, UI_Cell);
    U32 cell_count = 0;
    for(EachCmdKind(kind))
    {
      UI_Cell *cell = (cell_arr + cell_count++);
      Command_Info *cmd = cmd_info_from_kind(kind);
      
      cell->label = cmd->name;
      cell->sub_label = cmd->description;
      cell->flags = (UICellFlags_Clickable|
                     UICellFlags_DrawSubLabel|
                     UICellFlags_MoreSpace|
                     UICellFlags_DrawKeyMaps);
      cell->data = (void *)(U64)kind;
      cell->key_map = key_map_from_cmd_kind(app_state, kind);
      cell->filter_needle = str8_concat(scratch.arena, cell->label, cell->sub_label);
    }
    
    ui_center_popup(ui)
    {
      UI_Lister_Result result = ui_cell_lister(ui, S8("run"), ui_keyf("command_lister"), cell_arr, cell_count);
      if(result.selected != 0)
      {
        Command_Kind cmd = (Command_Kind)(U64)result.selected->data;
        Assert(cmd_kind_is_valid(cmd));
        app_run_command(app_state, cmd);
        handled = true;
      }
    }
    scratch_end(&scratch);
  }
  return(handled);
}

///////////////////////////////////////////////////////////
// NOTE(erb): command array

#define INF(kind, _func, _desc, _key, _mods) [CommandKind_##kind] = { \
.func = _func, \
.name = S8Lit(#_func), \
.description = S8Lit(_desc), \
.default_bind = { .key = _key, .mods = _mods }, \
}
Command_Info commands_arr[] = 
{
  INF(LoadFile,         load_file,          "Load events from .rose file",       GFXKey_O, GFXModifiers_Control),
  INF(SaveToFile,       save_to_file,       "Save current events to .rose file", GFXKey_S, GFXModifiers_Control),
  INF(CommandLauncher,  command_launcher,   "Open the command launcher",         GFXKey_F1, 0),
  INF(GenerateStubFile, generate_stub_file, "Generate a stub project.rose",      GFXKey_F2, 0),
};
#undef INF

Command_Info *
cmd_info_from_kind(Command_Kind kind)
{
  Assert(cmd_kind_is_valid(kind));
  Command_Info *info = (commands_arr + kind);
  return(info);
}

UPDATE_AND_RENDER_FUNC(update_and_render)
// App_Memory *memory, Q2D_State *render_state, 
// Q2D_Font *font, V2i client_size, GFX_Event_List *events,
// Platform_API platform_api,
// F32 delta_time, 
// String8 current_directory,
{
  App_State *app_state = (App_State *)memory->buffer;
  {
    if(memory->first_attach)
    {
      memory->first_attach = false;
      B32 os_initted = os_init();
      Assert(os_initted);
      thread_context_init(&app_state->thread_context);
      os_thread_context_set(&app_state->thread_context);
    }
    if(!app_state->initialized)
    {
      // NOTE(erb): init ui
      app_state->initialized = true;
      arena_init(&app_state->arena, (U8 *)memory->buffer+sizeof(App_State), 
                 memory->size-sizeof(App_State));
      app_state->font_face.font = font;
      app_state->font_face.blank_spacing = 10.0f;
      app_state->font_face.char_spacing = 0.0f;
      app_state->font_face.line_spacing = 1.0f;
      app_state->font_face.size = 28.0f;
      ui_init(&app_state->ui, &app_state->arena);
      
      // NOTE(erb): file stuff
      app_state->load_from_file_path = S8("project.rose");
      app_state->read_entire_file_str8 = platform_api.read_entire_file_str8;
      app_state->write_entire_file_str8 = platform_api.write_entire_file_str8;
      Assert(platform_api.read_entire_file_str8 != 0);
      Assert(platform_api.write_entire_file_str8 != 0);
      
      // NOTE(erb): init dir entry cache
      app_state->directory_arena = push_sub_arena(&app_state->arena, sizeof(Directory_Entry_List)*128);
      
      // NOTE(erb): init name chunkgs
      app_state->name_chunk_arena = push_sub_arena(&app_state->arena, 
                                                   (sizeof(Name_Chunk)+NAME_CHUNK_PAYLOAD_SIZE)*128);
      
      // NOTE(erb): init event context
      app_state->global_id_counter = 1;
      app_state->event_arena = push_sub_arena(&app_state->arena, sizeof(Event)*64);
      
      // NOTE(erb): init panels
      app_state->panel_arena = push_sub_arena(&app_state->arena, sizeof(Panel)*32);
      app_state->first_free_panel = 0;
      app_state->root_panel = push_panel(&app_state->panel_arena, &app_state->first_free_panel, 0);
      
      // NOTE(erb): init view free list
      {
        for(EachElement(View, view, app_state->views))
        {
          view->next = view+1;
        }
        app_state->first_free_view = app_state->views;
      }
      
      // NOTE(erb): init tag free list
      {
        for(EachElement(Event_Tag, tag, app_state->tags))
        {
          tag->next = tag+1;
        }
        app_state->first_free_tag = app_state->tags;
      }
      
      // NOTE(erb): key maps
      {
        for(EachCmdKind(kind))
        {
          Command_Info *info = cmd_info_from_kind(kind);
          if(info->default_bind.key != GFXKey_None)
          {
            Key_Combo_Set *set = (app_state->key_maps + kind);
            set->combos[set->count++] = info->default_bind;
          }
        }
      }
      
      // NOTE(erb): test import
      load_state(app_state);
      
      // NOTE(erb): main panel
      panel_append_view_set_current(app_state->root_panel, push_view(app_state, ViewKind_Watch));
      
#if 0
      // NOTE(erb): test events
      spawn_test_tasks(app_state);
      
      // NOTE(erb): test tags
      create_event_tag(app_state, S8("cat blue"), color_blue);
      create_event_tag(app_state, S8("cat red"), color_red);
      create_event_tag(app_state, S8("cat yellow"), color_yellow);
#endif
    }
    
#if DEBUG
    global_debug_ctx = &app_state->debug_ctx;
#endif
  }
  
  app_state->current_directory = current_directory;
  
  Scratch scratch = scratch_begin(0, 0);
  
  // NOTE(erb): ui
  UI_Ctx *ui = &app_state->ui;
  ui_set_clipboard_api(ui, platform_api.clipboard);
  ui_set_cursor_api(ui, platform_api.cursor);
  
  Range_V2f client_rect = rng_v2f_point_add(v2ff(0), v2fi(client_size));
  ui_begin_frame(ui, app_state->font_face, events, client_rect, delta_time);
  ui_font_color(ui, color_black)
  {
    
    // NOTE(erb): collected command events
    for(EachCmdKind(kind))
    {
      B32 triggered = false;
      Key_Combo_Set *key_map = key_map_from_cmd_kind(app_state, kind);
      for(U32Each(idx, key_map->count))
      {
        Key_Combo *combo = (key_map->combos + idx);
        if(key_pressed(ui, combo->key))
        {
          GFX_Key mods[3] = {0};
          U32 count = 0;
          if(combo->mods & GFXModifiers_Shift)
          {
            mods[count++] = GFXKey_Shift;
          }
          if(combo->mods & GFXModifiers_Control)
          {
            mods[count++] = GFXKey_Control;
          }
          if(combo->mods & GFXModifiers_Alt)
          {
            mods[count++] = GFXKey_Alt;
          }
          triggered = true;
          for(U32Each(key_idx, count))
          {
            if(!key_down(ui, mods[key_idx]))
            {
              triggered = false;
              break;
            }
          }
        }
      }
      
      if(triggered)
      {
        app_run_command(app_state, kind);
      }
    }
    
    // NOTE(erb): run command events
    for(Command_Event *event = app_state->first_event; event;)
    {
      event->not_first_time = true;
      Command_Info *cmd = cmd_info_from_kind(event->kind);
      B32 handled = cmd->func(app_state);
      if(handled)
      {
        DLLRemove(app_state->first_event, app_state->last_event, event);
        Command_Event *to_remove = event;
        event = event->next;
        SLLStackPush(app_state->first_free_event, to_remove);
      }
      else
      {
        event = event->next;
      }
    }
    
    // NOTE(erb): exit navigatoin
    if(key_pressed(ui, GFXKey_Escape) &&
       !ui_key_is_null(ui_top_nav_interface(ui)))
    {
      ui_pop_nav_interface_key(ui);
    }
    
    // NOTE(erb): collect navigation
    if(key_pressed(ui, GFXKey_Tab))
    {
      if(key_down(ui, GFXKey_Shift))
      {
        ui_pop_nav_interface_key(ui);
      }
      else
      {
        UI_Node *nav_root = ui_node_from_key(ui, ui_top_nav_interface(ui));
        if(nav_root == 0)
        {
          nav_root = ui_node_from_key(ui, ui->root->key);
        }
        
        if(nav_root != 0)
        {
          UI_Node *next_nav = 0;
          for(UI_Node *n = nav_root->first_child; n != 0;
              n = ui_node_rec_depth_first_pre_order(n).next)
          {
            if(n->flags & UIFlags_Comm)
            {
              next_nav = n;
              break;
            }
          }
          if(next_nav != 0)
          {
            ui_push_nav_interface_key(ui, next_nav->key);
          }
        }
      }
    }
    
    F32 split_region_half_width = 2.0f;
    
    // NOTE(erb): panel parent ui
    parent_panel_borders(app_state, client_rect, split_region_half_width);
    
    // NOTE(erb): panel leaf ui
    panel_leaf_ui(app_state, client_rect, split_region_half_width);
    
    // NOTE(erb): panel moving
    if(view_id_is_valid(app_state->moving_view))
    {
      panel_move_ui_and_do(app_state, client_rect, split_region_half_width);
    }
    
    if(ui->popup != 0)
    {
      ui_parent(ui, ui->popup);
    }
    
    // NOTE(erb): context
    debug_context(app_state);
  }
  UI_Draw_Cmd_List draw_commands = ui_layout_and_draw(ui, scratch.arena);
  render_ui_cmds(render_state, &draw_commands);
  ui_end_frame(ui);
  
  scratch_end(&scratch);
}