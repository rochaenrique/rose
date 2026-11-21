// NOTE(erb): headers
#include "base/base.h"
#include "img/img.h"
#include "gfx/gfx.h"
#include "fnt/fnt.h"
#include "gfx/q2d/q2d.h"
#include "app_bridge.h"

#include "ui/ui.h"
#include "token/token.h"
#include "ast/ast.h"   // NOTE(erb): new
#include "ir/ir.h"     // NOTE(erb): new
#include "rose/rose.h"

// NOTE(erb): implementation
#include "base/base.c"
#include "img/img.c"
#include "gfx/gfx.c"
#include "fnt/fnt.c"
#include "gfx/q2d/q2d.c"

#include "ui/ui.c"
#include "token/token.c"
#include "ast/ast.c"   // NOTE(erb): new
#include "ir/ir.c"     // NOTE(erb): new
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
      
      for(EachU32(idx, event->tag_count))
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

typedef enum ROSE_Meta_Type_Kind
{
  ROSEMetaTypeKind_U32,
  ROSEMetaTypeKind_B32,
  ROSEMetaTypeKind_Calendar_Date,
  ROSEMetaTypeKind_COUNT,
} ROSE_Meta_Type_Kind;

typedef struct ROSE_Meta_Field ROSE_Meta_Field;
struct ROSE_Meta_Field
{
  String8 name;
  U32 byte_offset;
  ROSE_Meta_Type_Kind type_kind;
};

#define ROSE_META_UI_RULE_FIELD_NAME _ui_rule_
String8 rose_meta_ui_rule_field_name_str8 = S8Lit(StringifyAfter(ROSE_META_UI_RULE_FIELD_NAME));

#define ROSEMetaViewStructFieldsName(Struct_Name) Struct_Name##_rose_meta_fields

#define ROSEMetaField(Struct_Name, field_name, _type_kind) {  \
.byte_offset = offsetof(Struct_Name, field_name), \
.name = S8Lit(Stringify(field_name)),  \
.type_kind = _type_kind }

#define ROSE_META_VIEW_STRUCT_DEFINE3(Struct_Name, Type1, field_name1, type_kind1, Type2, field_name2, type_kind2, Type3, field_name3, type_kind3) \
typedef struct Struct_Name Struct_Name; \
struct Struct_Name \
{ \
ROSE_UI_Rule ROSE_META_UI_RULE_FIELD_NAME; \
Type1 field_name1; \
Type2 field_name2; \
Type3 field_name3; \
}; \
ROSE_Meta_Field ROSEMetaViewStructFieldsName(Struct_Name)[] = \
{ \
ROSEMetaField(Struct_Name, ROSE_META_UI_RULE_FIELD_NAME, ROSEMetaTypeKind_U32), \
ROSEMetaField(Struct_Name, field_name1, type_kind1), \
ROSEMetaField(Struct_Name, field_name2, type_kind2), \
ROSEMetaField(Struct_Name, field_name3, type_kind3), \
}; \
StaticAssert(offsetof(Struct_Name, ROSE_META_UI_RULE_FIELD_NAME) == 0, Struct_Name##_has_ui_rule_at_offset0);

ROSE_META_VIEW_STRUCT_DEFINE3(ROSE_Calendar_View,
                              Calendar_Date, calendar_start , ROSEMetaTypeKind_Calendar_Date,
                              U32          , calendar_grid_x, ROSEMetaTypeKind_U32,
                              U32          , calendar_grid_y, ROSEMetaTypeKind_U32)

void
ui_calendar(UI_Ctx *ui, Calendar_Date start_date, V2i grid_size, Range_V2f space, Event_List *event_list)
{
  Scratch scratch = scratch_begin(0, 0);
  V2f space_size = rng_v2f_size(space);
  
  I32 color_space = (grid_size.y + grid_size.x);
  V2f cell_size = v2f_hada_div(space_size, v2fi(grid_size));
  V2f cell_cursor = space.min;
  for(EachU32(y, (U32)grid_size.y))
  {
    cell_cursor.x = space.min.x;
    for(EachU32(x, (U32)grid_size.x))
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
ui_calendar_view(App_State *app_state, Buffer *memory, E_Value *value)
{
  Assert(ir_type_id_equals(app_state->ir_type_id__calendar_view, value->type));
  
  ROSE_Calendar_View *cal_view = EValueGetPtr(memory, value, ROSE_Calendar_View);
  Calendar_Date *calendar_start = &cal_view->calendar_start;
  U32 *calendar_grid_x = &cal_view->calendar_grid_x;
  U32 *calendar_grid_y = &cal_view->calendar_grid_y;
  
  UI_Ctx *ui = &app_state->ui;
  
  Scratch scratch = scratch_begin(0, 0);
  
  ui_set_next_sizing(ui, ui_grow(), ui_fixed(200.0f));
  ui_set_next_padding(ui, pad2_equal(0));
  ui_set_next_background_color(ui, color_magenta);
  UI_Node *calendar_panel = ui_nodef(ui, UIFlags_DrawBackGround|UIFlags_Comm, S8("calendar_panel"));
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
      *calendar_grid_x = Cap(*calendar_grid_x + scroll_size_delta.x, 1, 7);
      *calendar_grid_y = Cap(*calendar_grid_y + scroll_size_delta.y, 1, 10);
      
      Range_V2f inner = ui_node_rng_v2f_inner(out.node);
      ui_calendar(ui, *calendar_start, v2i(*calendar_grid_x, *calendar_grid_y), inner, &app_state->event_list);
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
        for(EachU32(idx, event->tag_count))
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
            Range_V2f rng = ui_node_rng_v2f(trigger.node);
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

AST_Node *
ast_from_text(Arena *arena, String8 text, String8 *error)
{
  Scratch scratch = scratch_begin(&arena, 1);
  Token_List tokens = token_list_from_str(scratch.arena, text, error);
  
  AST_Node *ast = 0;
  if(error->value == 0)
  {
    ast = ast_from_token_list(arena, &tokens, error);
  }
  scratch_end(&scratch);
  return(ast);
}

E_Value_Interp
e_value_from_text(Arena *arena, String8 text, IR_Type_Table *type_table, 
                  E_Global_Iden *top_global_iden, IR_Symbol_Type *top_symbol_type, 
                  String8 *error)
{
  Scratch scratch = scratch_begin(&arena, 1);
  AST_Node *ast = ast_from_text(scratch.arena, text, error);
  
  IR_Op_List op_list = {0}; 
  if(error->value == 0)
  {
    op_list = ir_op_list_from_ast(scratch.arena, ast, type_table, top_symbol_type, error);
  }
  
  E_Value_Interp result = {0}; 
  if(error->value == 0)
  {
    result = e_interp_from_ir_ops(arena, &op_list, type_table, top_global_iden);
  }
  scratch_end(&scratch);
  return(result);
}

void
fancy_str8_list_from_ir_type_recur(Arena *arena, Fancy_String8_List *fstr, IR_Type *type, 
                                   IR_Type_Table *type_table, U32 depth, U32 max_depth)
{
  if(depth+1 >= max_depth)
  {
    fancy_str8_push(arena, fstr, S8("T"), color_red);
  }
  else switch(type->kind)
  {
    case IRTypeKind_U32: fancy_str8_push(arena, fstr, S8("U32"), color_cyan); break;
    case IRTypeKind_B32: fancy_str8_push(arena, fstr, S8("B32"), color_cyan); break;
    case IRTypeKind_U8 : fancy_str8_push(arena, fstr, S8("U8" ), color_cyan); break;
    
    case IRTypeKind_AddressOf:
    {
      fancy_str8_list_from_ir_type_recur(arena, fstr, 
                                         ir_type_from_id(type_table, type->sub_type), 
                                         type_table, depth+1, max_depth);
      fancy_str8_push(arena, fstr, S8(" *"), color_white);
    } break;
    
    case IRTypeKind_Function:
    {
      fancy_str8_list_from_ir_type_recur(arena, fstr, 
                                         ir_type_from_id(type_table, type->sub_type), 
                                         type_table, depth+1, max_depth);
      fancy_str8_push(arena, fstr, S8("(*)("), color_white);
      for(EachU32(idx, type->field_count))
      {
        IR_Field_Type *field = (type->fields + idx);
        fancy_str8_list_from_ir_type_recur(arena, fstr, 
                                           ir_type_from_id(type_table, field->type), 
                                           type_table, depth+1, max_depth);
        if(idx+1 < type->field_count)
        {
          fancy_str8_push(arena, fstr, S8(", "), color_white);
        }
      }
      fancy_str8_push(arena, fstr, S8(")"), color_white);
    } break;
    
    case IRTypeKind_Record:
    {
      if(type->is_array_record)
      {
        IR_Type_ID data_type = ir_array_data_type(type_table, type);
        fancy_str8_list_from_ir_type_recur(arena, fstr, 
                                           ir_type_from_id(type_table, data_type), 
                                           type_table, depth+1, max_depth);
        fancy_str8_push(arena, fstr, S8("[]"), color_white);
      }
      else
      {
        fancy_str8_push(arena, fstr, type->record_name, color_cyan);
      }
    } break;
    
    InvalidDefaultCase;
  }
}

void
fancy_str8_list_from_e_value_recur(Arena *arena, Fancy_String8_List *fstr, E_Value *value, 
                                   Buffer *value_memory, IR_Type_Table *type_table, U32 depth, U32 max_depth)
{
  if(depth < max_depth)
  {
    IR_Type *type = ir_type_from_id(type_table, value->type);
    Assert(type != 0);
    
    switch(type->kind)
    {
      case IRTypeKind_U32:
      {
        U32 u32 = *EValueGetPtr(value_memory, value, U32);
        fancy_str8_push(arena, fstr, push_str8f(arena, "%d", u32), color_green);
      } break;
      
      case IRTypeKind_B32:
      {
        B32 b32 = *EValueGetPtr(value_memory, value, B32);
        fancy_str8_push(arena, fstr, (b32 ? S8("true") : S8("false")), color_green);
      } break;
      
      case IRTypeKind_U8:
      {
        fancy_str8_push(arena, fstr, push_str8f(arena, "'%c'", *EValueGetPtr(value_memory, value, U8)), color_green); 
      } break;
      
      case IRTypeKind_AddressOf:
      {
        U64 u64 = *EValueGetPtr(value_memory, value, U64);
        fancy_str8_push(arena, fstr, S8("("), color_white);
        fancy_str8_list_from_ir_type_recur(arena, fstr, type, type_table, depth, max_depth);
        fancy_str8_push(arena, fstr, S8(")"), color_white);
        fancy_str8_push(arena, fstr, push_str8f(arena, "%#x", u64), color_green);
      } break;
      
      case IRTypeKind_Function:
      {
        fancy_str8_push(arena, fstr, S8("?"), color_cyan);
        fancy_str8_list_from_ir_type_recur(arena, fstr, type, type_table, depth, max_depth);
        fancy_str8_push(arena, fstr, S8("?"), color_cyan);
      } break;
      
      case IRTypeKind_Record:
      {
        // NOTE(erb): special case str8
        if(ir_type_id_equals(value->type, ir_builtin_type_id_str8)) 
        {
          IR_Array *array = e_array_from_value_cast_unchecked(value_memory, value);
          String8 s = S8Struct(array->data, array->count);
          fancy_str8_push(arena, fstr, S8("{"), color_white);
          fancy_str8_push(arena, fstr, push_str8f(arena, "%d", array->count), color_green);
          fancy_str8_push(arena, fstr, S8(", "), color_white);
          fancy_str8_push(arena, fstr, push_str8f(arena, "\"%S\"", s), color_green);
          fancy_str8_push(arena, fstr, S8("}"), color_white);
        }
        else if(type->is_array_record)
        {
          IR_Array *array = e_array_from_value_cast_unchecked(value_memory, value);
          IR_Type *data_type = ir_type_from_id(type_table, ir_array_data_type(type_table, type));
          Buffer array_memory = buffer_make(array->data, array->count * data_type->byte_count);
          
          fancy_str8_push(arena, fstr, S8("["), color_white);
          for(EachU32(idx, array->count))
          {
            E_Value child = e_value_from_index_offset(data_type, idx);
            fancy_str8_list_from_e_value_recur(arena, fstr, &child, &array_memory, type_table, depth+1, max_depth);
            if(idx+1 < array->count)
            {
              fancy_str8_push(arena, fstr, S8(", "), color_white);
            }
          }
          fancy_str8_push(arena, fstr, S8("]"), color_white);
        }
        else
        {
          fancy_str8_push(arena, fstr, S8("{"), color_white);
          if(depth+1 < max_depth)
          {
            for(EachU32(idx, type->field_count))
            {
              IR_Field_Type *field = (type->fields + idx);
              IR_Type *field_type = ir_type_from_id(type_table, field->type);
              E_Value child = e_value_slice_as(value, field_type, field->byte_offset);
              fancy_str8_list_from_e_value_recur(arena, fstr, &child, value_memory, type_table, depth+1, max_depth);
              
              if(idx+1 < type->field_count)
              {
                fancy_str8_push(arena, fstr, S8(", "), color_white);
              }
            }
          }
          fancy_str8_push(arena, fstr, S8("}"), color_white);
        }
      } break;
    }
  }
}

U32
ast_depth_recur(AST_Node *ast)
{
  U32 result = 0;
  if(ast != 0)
  {
    if(ast->kind == ASTKind_Binary)
    {
      result = Max(ast_depth_recur(ast->bin.left), ast_depth_recur(ast->bin.right));
    }
    else if(ast->kind == ASTKind_Function)
    {
      for(EachNode(AST_Node, arg, ast->func.args.first))
      {
        result = Max(result, ast_depth_recur(arg));
      }
      result += 1;
    }
  }
  return(result);
}

U32
hash_from_watch_id(Eval_Watch_ID id)
{
  U32 hash = (((id.parent+1) << 16) + (id.idx+1));
  return(hash);
}

B32
watch_id_is_expanded(Watch_Expansion_State *state, Eval_Watch_ID id)
{
  U32 hash = hash_from_watch_id(id);
  Assert(is_power2_u32(ArraySize(state->slots)));
  
  Watch_Expansion *slot = state->slots[(hash & (ArraySize(state->slots)-1))];
  B32 result = false;
  for(EachNode(Watch_Expansion, exp, slot))
  {
    if(exp->id.parent == id.parent &&
       exp->id.idx == id.idx)
    {
      result = true;
      break;
    }
  }
  return(result);
}

Eval_Watch_ID
watch_id_make(U32 parent, U32 idx)
{
  Eval_Watch_ID result = {0};
  result.parent = parent;
  result.idx = idx;
  return(result);
}

Eval_Block_Tree *
push_block_tree(Arena *arena, String8 str, String8 error, Eval_Watch_ID id)
{
  Eval_Block_Tree *result = PushStruct(arena, Eval_Block_Tree);
  result->str = str;
  result->error = error;
  result->id = id;
  return(result);
}

Eval_Block_Tree *
push_list_block_tree(Arena *arena, Eval_Block_Tree_List *list, String8 str, String8 error, Eval_Watch_ID id)
{
  Eval_Block_Tree *result = push_block_tree(arena, str, error, id);
  SLLQueueAppend(list->first, list->last, result);
  ++list->count;
  return(result);
}

B32 
ir_type_array_contains(IR_Type_ID *array, U32 array_count, IR_Type_ID type)
{
  B32 result = false;
  for(EachPtr(IR_Type_ID, test, array, array_count))
  {
    if(ir_type_id_equals(type, *test))
    {
      result = true;
      break;
    }
  }
  return(result);
}

Eval_Block_Tree *
push_block_tree_expanded(Arena *arena, String8 str, String8 error, Eval_Watch_ID id, 
                         E_Value *value, Buffer *value_memory, IR_Type_Table *type_table, 
                         IR_Type_ID *custom_exp_types, U32 custom_exp_count)
{
  Eval_Block_Tree *block = push_block_tree(arena, str, error, id);
  block->value = *value;
  block->value_memory = *value_memory;
  
  // NOTE(erb): leave it as is if we have a custom expansion
  {
    // NOTE(erb): default expansion for arrays and structs
    IR_Type *type = ir_type_from_id(type_table, block->value.type);
    if(type->kind == IRTypeKind_Record)
    {
      if(type->is_array_record)
      {
        IR_Array *array = e_array_from_value_cast_unchecked(&block->value_memory, &block->value);
        block->expansion_size = array->count;
      }
      else
      {
        block->expansion_size = type->field_count;
      }
      
#if 0
      Eval_Watch_ID child_id = {.parent = hash_from_watch_id(id)};
      for(EachU32(idx, block->expansion_size))
      {
        child_id.idx = idx;
        push_block_tree_expansions_recur(arena, block, child_id, 
                                         expansion_state, type_table, false, 
                                         custom_exp_types, custom_exp_count);
      }
#endif
    }
  }
  return(block);
}

// NOTE(erb): 'is_expansion_not_slice' is basically only for base case where the watch eval
// turns into a node that isnt a slice, its just the thing
void
push_block_tree_expansions_recur(Arena *arena, Eval_Block_Tree *parent, Eval_Watch_ID id, 
                                 Watch_Expansion_State *expansion_state, 
                                 IR_Type_Table *type_table, B32 is_expansion_not_slice, 
                                 IR_Type_ID *custom_exp_types, U32 custom_exp_count)
{
  if(watch_id_is_expanded(expansion_state, id))
  {
    Eval_Block_Tree *block = push_list_block_tree(arena, &parent->children, 
                                                  parent->str, parent->error, id);
    
    if(is_expansion_not_slice) // TODO(erb): this might not be the best check
    {
      block->value = parent->value;
      block->value_memory = parent->value_memory;
    }
    else
    {
      // NOTE(erb): index into parent
      IR_Type *parent_type = ir_type_from_id(type_table, parent->value.type);
      if(parent_type->kind == IRTypeKind_Record)
      {
        if(parent_type->is_array_record)
        {
          IR_Array *array = e_array_from_value_cast_unchecked(&parent->value_memory, &parent->value);
          Assert(id.idx < array->count);
          IR_Type *data_type = ir_type_from_id(type_table, ir_array_data_type(type_table, parent_type));
          
          block->value = e_value_from_index_offset(data_type, id.idx);
          block->value_memory = buffer_make(array->data, array->count * data_type->byte_count);
        }
        else
        {
          // NOTE(erb): access here
          IR_Field_Type *field = (parent_type->fields + id.idx);
          IR_Type *field_type = ir_type_from_id(type_table, field->type);
          block->value = e_value_slice_as(&parent->value, field_type, field->byte_offset);
          block->value_memory = parent->value_memory;
        }
      }
    }
    
    // NOTE(erb): leave it as is if we have a custom expansion
    if(is_expansion_not_slice && 
       ir_type_array_contains(custom_exp_types, custom_exp_count, block->value.type))
    {
      block->expansion_size = 1;
    }
    else
    {
      // NOTE(erb): default expansion for arrays and structs
      IR_Type *type = ir_type_from_id(type_table, block->value.type);
      if(type->kind == IRTypeKind_Record)
      {
        if(type->is_array_record)
        {
          IR_Array *array = e_array_from_value_cast_unchecked(&block->value_memory, &block->value);
          block->expansion_size = array->count;
        }
        else
        {
          block->expansion_size = type->field_count;
        }
        
        Eval_Watch_ID child_id = {.parent = hash_from_watch_id(id)};
        for(EachU32(idx, block->expansion_size))
        {
          child_id.idx = idx;
          push_block_tree_expansions_recur(arena, block, child_id, 
                                           expansion_state, type_table, false, 
                                           custom_exp_types, custom_exp_count);
        }
      }
    }
  }
}

Eval_Block_Range *
push_block_range_from_tree(Arena *arena, Eval_Block_Range_List *list, Eval_Block_Tree *tree, U32 depth)
{
  Eval_Block_Range *result = PushStruct(arena, Eval_Block_Range);
  SLLQueueAppend(list->first, list->last, result);
  ++list->count;
  result->id = tree->id;
  result->error = tree->error;
  result->str = tree->str;
  result->depth = depth;
  result->value = tree->value;
  result->value_memory = tree->value_memory;
  result->is_header = (tree->expansion_size == 0);
  return(result);
}

void
push_block_range_list_from_tree_recur(Arena *arena, Eval_Block_Range_List *list, Eval_Block_Tree *tree, U32 depth)
{
  Eval_Block_Range *first = push_block_range_from_tree(arena, list, tree, depth);
  first->range_in_parent_start = 0;
  first->range_in_parent_size = Cap1(tree->expansion_size);
  
  Eval_Block_Range *prev = first;
  
  U32 child_depth = depth+1;
  for(EachNode(Eval_Block_Tree, child, tree->children.first))
  {
    push_block_range_list_from_tree_recur(arena, list, child, child_depth);
    
    prev->range_in_parent_size = AssertOrUSub(child->id.idx, prev->range_in_parent_start) + 1;
    
    Eval_Block_Range *post = push_block_range_from_tree(arena, list, tree, depth);
    post->range_in_parent_start = (prev->range_in_parent_start + prev->range_in_parent_size);
    post->range_in_parent_size = (tree->expansion_size < post->range_in_parent_start ? 0
                                  : tree->expansion_size - post->range_in_parent_start);
    prev = post;
    
    list->total_expansions += post->range_in_parent_size;
  }
  
  list->total_expansions += first->range_in_parent_size;
}

void
watch_expansion_toggle(Watch_Expansion_State *state, Eval_Watch_ID id)
{
  U32 hash = hash_from_watch_id(id);
  
  U32 idx = hash & (ArraySize(state->slots)-1);
  
  Watch_Expansion **row = (state->slots + idx);
  Watch_Expansion *prev = 0;
  B32 exists = false;
  for(EachNode(Watch_Expansion, test, *row))
  {
    if(test->id.parent == id.parent &&
       test->id.idx == id.idx)
    {
      exists = true;
      break;
    }
    prev = test;
  }
  
  if(exists) // NOTE(erb): remove
  {
    Watch_Expansion *to_remove = 0;
    if(prev == 0)
    {
      to_remove = *row;
      SLLStackPop(*row);
    }
    else
    {
      to_remove = prev->next;
      prev->next = to_remove->next;
    }
    Assert(to_remove != 0);
    SLLStackPush(state->first_free_exp, to_remove);
  }
  else
  {
    Assert(state->first_free_exp != 0);
    Watch_Expansion *slot = state->first_free_exp;
    SLLStackPop(state->first_free_exp);
    SLLStackPush(*row, slot);
    slot->id = id;
  }
}

B32
rose_ir_type_is_view_type(App_State *app_state, IR_Type_ID type)
{
  B32 result = ir_type_array_contains(app_state->ir_view_types, 
                                      app_state->ir_view_types_count, type);
  return(result);
}

void
ui_do_ui_rule_from_e_value(App_State *app_state, Buffer *buffer, E_Value *value)
{
  Assert(rose_ir_type_is_view_type(app_state, value->type));
#if DEBUG
  {
    IR_Type *type = ir_type_from_id(&app_state->type_table, value->type);
    Assert(type->field_count >= 1);
    IR_Field_Type *field = ir_type_field_from_name(type, rose_meta_ui_rule_field_name_str8);
    Assert(field->byte_offset == 0);
  }
#endif
  // NOTE(erb): this assumes that ui_rule field is at offset 0
  ROSE_UI_Rule ui_rule = *(ROSE_UI_Rule *)e_value_get_bytes_from(buffer, value);
  
  // TODO(erb): dispatch table here
  switch(ui_rule)
  {
    case ROSEUIRule_Calendar: ui_calendar_view(app_state, buffer, value); break;
    InvalidDefaultCase;
  }
}

void
ui_watch_window_rows(App_State *app_state, UI_Ctx *ui, Watch_Expansion_State *expansion_state, 
                     IR_Type_Table *type_table, Eval_Block_Range_List *block_list, 
                     I32 selection_cursor, B32 enter_pressed, U32 eval_max_depth,
                     B32 allow_custom_expansions)
{
  Scratch watch_scratch = scratch_begin(0, 0);
  
  U32 idx = 0;
  for(EachNode(Eval_Block_Range, block, block_list->first))
  {
    if(block->range_in_parent_size > 0)
    {
      // NOTE(erb): pre computations
      IR_Type *output_type = 0;
      IR_Type *data_type = 0;
      Buffer expansion_buffer = {0};
      if(block->error.value == 0)
      {
        output_type = ir_type_from_id(type_table, block->value.type);
        Assert(output_type->kind == IRTypeKind_Record);
        AssertImplies(!output_type->is_array_record,
                      block->range_in_parent_start < output_type->field_count && 
                      block->range_in_parent_start + block->range_in_parent_size < output_type->field_count + 1);
        
        if(!block->is_header && output_type->is_array_record)
        {
          data_type = ir_type_from_id(type_table, ir_array_data_type(type_table, output_type));
          IR_Array *array = e_array_from_value_cast_unchecked(&block->value_memory, &block->value);
          expansion_buffer = buffer_make(array->data, array->count * data_type->byte_count);
        }
        else
        {
          expansion_buffer = block->value_memory;
        }
      }
      
      Eval_Watch_ID expansion_id = {0};
      expansion_id.parent = (block->is_header ? block->id.parent : hash_from_watch_id(block->id));
      
      // NOTE(erb): loop each index in expansion
      for(EachU32(expansion_tick, block->range_in_parent_size))
      {
        Scratch scratch = arena_get_scratch(watch_scratch.arena);
        U32 expansion_idx = (expansion_tick + block->range_in_parent_start);
        expansion_id.idx = expansion_idx;
        B32 is_selected = (idx == (U32)selection_cursor);
        
        if(is_selected && enter_pressed)
        {
          watch_expansion_toggle(expansion_state, expansion_id);
        }
        
        // NOTE(erb): produce type & value (& label)
        B32 is_custom_ui_expansion = false;
        B32 is_field = false;
        String8 label_str = {0};
        IR_Type *expansion_type = 0;
        E_Value expansion_value = {0};
        if(block->error.value == 0)
        {
          is_custom_ui_expansion = (allow_custom_expansions && rose_ir_type_is_view_type(app_state, output_type->id));
          
          if(block->is_header || is_custom_ui_expansion)
          {
            expansion_value = block->value;
            label_str = block->str;
            expansion_type = output_type;
          }
          else
          {
            if(output_type->is_array_record)
            {
              expansion_value = e_value_from_index_offset(data_type, expansion_idx);
              expansion_type = data_type;
            }
            else
            {
              is_field = true;
              IR_Field_Type *field = (output_type->fields + expansion_idx);
              IR_Type *field_type = ir_type_from_id(type_table, field->type);
              expansion_value = e_value_slice_as(&block->value, field_type, field->byte_offset);
              label_str = field->name;
              expansion_type = field_type;
            }
          }
        }
        
        
        // NOTE(erb): ui representation
        
        ui_set_next_border_color(ui, (is_selected ? color_red : color_white));
        ui_set_next_border_thickness(ui, 2.0f);
        ui_set_next_layout_axis(ui, Axis2_X);
        ui_sizing(ui, ui_grow(), ui_fit()) 
          ui_parentf(ui, UIFlags_DrawBorder, S8Z())
        {
          if(block->error.value == 0 && 
             !block->is_header && 
             is_custom_ui_expansion)
          {
            ui_do_ui_rule_from_e_value(app_state, &expansion_buffer, &expansion_value);
          }
          else
          {
            // NOTE(erb): value str & type str
            B32 can_expand_type = false;
            Fancy_String8_List value_fancy_str = {0};
            Fancy_String8_List type_fancy_str = {0};
            if(block->error.value == 0)
            {
              can_expand_type = (expansion_type->kind == IRTypeKind_Record);
              fancy_str8_list_from_e_value_recur(scratch.arena, &value_fancy_str, &expansion_value, 
                                                 &expansion_buffer, type_table, 0, eval_max_depth);
              fancy_str8_list_from_ir_type_recur(scratch.arena, &type_fancy_str, expansion_type, 
                                                 type_table, 0, eval_max_depth);
            }
            else
            {
              fancy_str8_push(scratch.arena, &value_fancy_str, block->error, color_red);
              fancy_str8_push(scratch.arena, &type_fancy_str, S8("ERR"), color_yellow);
            }
            
            // NOTE(erb): basic ui rendering
            ui_parentf(ui, 0, S8Z())
            {
              ui_spacer(ui, 20.f*block->depth);
              
              if(can_expand_type)
              {
                B32 is_expanded = watch_id_is_expanded(expansion_state, expansion_id);
                ui_set_next_font_color(ui, color_yellow);
                ui_labelf(ui, (is_expanded ? S8("v") : S8(">")));
              }
              
              if(label_str.size == 0)
              {
                ui_labelf(ui, S8("[%d]"), expansion_idx);
              }
              else if(is_field)
              {
                ui_labelf(ui, S8(".%S"), label_str);
              }
              else
              {
                ui_labelf(ui, S8("%S"), label_str);
              }
            }
            
            ui_parentf(ui, 0, S8Z())
            {
              ui_fancy_str(ui, value_fancy_str);
            }
            
            ui_parentf(ui, 0, S8Z())
            {
              ui_fancy_str(ui, type_fancy_str);
            }
          }
        }
        
        
        scratch_end(&scratch);
        ++idx;
      }
    }
  }
  
  scratch_end(&watch_scratch);
}

Eval_Block_Tree_List
block_tree_list_from_watch_evals(Arena *arena, Watch_Eval *evals, U32 eval_count, 
                                 IR_Type_Table *type_table, IR_Symbol_Type *top_symbol_type, E_Global_Iden *top_global_iden,
                                 Watch_Expansion_State *expansion_state, 
                                 IR_Type_ID *custom_exp_types, U32 custom_exp_count)
{
  Eval_Block_Tree_List block_tree_list = {0};
  Scratch scratch = scratch_begin(&arena, 1);
  
  for(EachU32(idx, eval_count))
  {
    Watch_Eval *state = (evals + idx);
    String8 eval_str = S8Struct(state->str_buffer, state->str_size);
    Assert(state->arena.buffer.base != 0);
    AssertImplies(!state->has_data, state->error.value == 0);
    
    E_Value_Interp interp = {0};
    {
      if(!state->has_data)
      {
        Scratch ast_scratch = arena_get_scratch(scratch.arena);
        AST_Node *ast = ast_from_text(ast_scratch.arena, eval_str, &state->error);
        if(state->error.value == 0)
        {
          state->op_list = ir_op_list_from_ast(&state->arena, ast, type_table, 
                                               top_symbol_type, &state->error);
        }
        scratch_end(&ast_scratch);
        if(state->error.value == 0 && 
           state->op_list.count == 0)
        {
          // NOTE(erb): handle empty op-list: produce an error?
          // TODO(erb): might not be the best way to do this
          state->error = S8("expression produced no operations");
        }
        
        state->has_data = true;
        
        if(state->error.value == 0 && 
           ir_type_array_contains(custom_exp_types, custom_exp_count, state->op_list.first->type))
        {
          state->should_cache_interp = true;
          interp = e_interp_from_ir_ops(&state->arena, &state->op_list, type_table, top_global_iden);
          state->interp = interp;
        }
      }
      
      if(state->error.value != 0)
      {
        arena_release(&state->arena);
      }
      else
      {
        if(!state->should_cache_interp)
        {
          interp = e_interp_from_ir_ops(arena, &state->op_list, type_table, top_global_iden);
        }
        else
        {
          interp = state->interp;
        }
      }
    }
    
    Eval_Block_Tree *block = push_list_block_tree(arena, &block_tree_list, eval_str, 
                                                  state->error, watch_id_make(idx, 0));
    block->value = interp.value;
    block->value_memory = interp.memory;
    push_block_tree_expansions_recur(arena, block, block->id, 
                                     expansion_state, type_table, true,
                                     custom_exp_types, custom_exp_count);
  }
  return(block_tree_list);
}

void
watch_eval_init(Watch_Eval *eval, String8 str)
{
  Assert(str.size <= (U64)eval->str_buffer);
  eval->str_size = safe_truncate_U64_to_U32(str.size);
  ArrayCopy(str.value, eval->str_buffer, str.size);
  
  arena_init(&eval->arena, eval->memory, ArraySize(eval->memory));
}

void
watch_expansion_state_init(Watch_Expansion_State *state)
{
  for(EachElement(Watch_Expansion, s, state->expansions))
  {
    s->next = s+1;
  }
  state->first_free_exp = state->expansions;
}

void
watch_panel(App_State *app_state, View *view)
{
  Assert(view != 0);
  Assert(view->is_watch);
  
  if(!view->is_watch_first_time)
  {
    view->is_watch_first_time = true;
    
    view->eval_max_depth = 3;
    watch_expansion_state_init(&view->expansion_state);
    
    // NOTE(erb): debug stuff
    {
      Assert(view->eval_count+1 < ArraySize(view->evals));
      Watch_Eval *new_eval = (view->evals + view->eval_count++);
      watch_eval_init(new_eval, S8("calendar()"));
    }
  }
  
  UI_Ctx *ui = &app_state->ui;
  UI_Key input_key = ui_keyf("watch_expr_input");
  
  // NOTE(erb): update max eval depth
  if(key_down(ui, GFXKey_Control))
  {
    I32 new_max_depth = view->eval_max_depth;
    if(key_pressed(ui, GFXKey_Right))
    {
      new_max_depth += 1;
    }
    if(key_pressed(ui, GFXKey_Left))
    {
      new_max_depth += -1;
    }
    view->eval_max_depth = (U32)Cap0(new_max_depth);
  }
  
  B32 enter_pressed = key_pressed(ui, GFXKey_Enter);
  
  Scratch view_scratch = scratch_begin(0, 0);
  
  IR_Type_Table *type_table = &app_state->type_table;
  IR_Symbol_Type *top_symbol_type = app_state->top_symbol_type;
  E_Global_Iden *top_global_iden = app_state->top_global_iden;
  
  // NOTE(erb): build block tree
  Eval_Block_Tree_List block_tree_list = block_tree_list_from_watch_evals(view_scratch.arena, view->evals, view->eval_count,
                                                                          type_table, top_symbol_type, top_global_iden, 
                                                                          &view->expansion_state, 
                                                                          app_state->ir_view_types, app_state->ir_view_types_count);
  
  // NOTE(erb): block list
  Eval_Block_Range_List block_list = {0};
  for(EachNode(Eval_Block_Tree, tree, block_tree_list.first))
  {
    push_block_range_list_from_tree_recur(view_scratch.arena, &block_list, tree, 0);
  }
  
  // NOTE(erb): update cursor
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
    view->selection_cursor = Cap(new_cursor, 0, (I32)CapBottom(block_list.total_expansions, 1) - 1);
  }
  
  
  ui_set_next_sizing(ui, ui_grow(), ui_grow());
  ui_set_next_layout_axis(ui, Axis2_Y);
  ui_set_next_background_color(ui, with_alpha(v4ff(0.1f), 1));
  ui_set_next_child_gap(ui, 0);
  ui_parentf(ui, UIFlags_DrawBackGround, S8Z())
  {
    UI_Line_Input_State *input = ui_line_input_state_from_key(ui, input_key);
    
    // NOTE(erb): ON enter new expr
    if(input->writer.str.size != 0 && enter_pressed)
    {
      // NOTE(erb): new eval & clear input
      Assert(view->eval_count+1 < ArraySize(view->evals));
      Watch_Eval *new_eval = (view->evals + view->eval_count++);
      
      watch_eval_init(new_eval, input->writer.str);
      input->writer.str.size = 0;
    }
    
    // NOTE(erb): watch window rows
    ui_watch_window_rows(app_state, ui, &view->expansion_state, type_table, 
                         &block_list, view->selection_cursor, enter_pressed, 
                         view->eval_max_depth, true);
    
    // NOTE(erb): input field box
    ui_line_input_update_state(ui, input);
  }
  scratch_end(&view_scratch);
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
    
    if(AssertOrBool(project.error.value == 0))
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
lister_panel(App_State *app_state, View_ID id)
{
  Assert(view_id_is_valid(id));
  View *view = view_from_id(app_state, id);
  Assert(view != 0);
  
  UI_Ctx *ui = &app_state->ui;
  
  Scratch scratch = scratch_begin(0, 0);
  
  UI_Cell *cell_arr = PushArray(scratch.arena, app_state->event_list.count, UI_Cell);
  U32 cell_count = 0;
  
  for(EachNode(Event, event, app_state->event_list.first))
  {
    UI_Cell *cell = (cell_arr + cell_count++);
    cell->label = str8_from_name_chunk_list(scratch.arena, &event->title_chunk_list);
  }
  
  ui_cell_lister_filtered(ui, S8("search"), ui_keyf("lister_panel"), 
                          cell_arr, cell_count);
  
  scratch_end(&scratch);
}

#if 0
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
            Range_V2f rng = ui_node_rng_v2f(trigger.node);
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
                    Range_V2f rng = ui_node_rng_v2f(comm.node);
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
#endif

#if 0
void
event_focus_panel(App_State *app_state, View_ID id)
{
  Assert(view_id_is_valid(id));
  View *view = view_from_id(app_state, id);
  Assert(view != 0);
  
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
#endif

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
    UI_Debug_Ctx *debug = &app_state->debug_ctx;
    
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
        ui_set_cursor(ui, cursor_kind);
        
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
    node->corner_radius = ui_rad_abs(corner_radius_hover);
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
        for(EachU32(dir, Direction2_COUNT))
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
  B16 select_or_drag;
  B16 remove;
  B16 options;
};

Tab_Button_Result
draw_tab_button_kind(Tab_Button_Kind kind, View_ID view, App_State *app_state, Panel *panel, 
                     UI_Key tab_button_container_key, B32 is_current_selected, 
                     String8 view_label)
{
  UI_Ctx *ui = &app_state->ui;
  Tab_Button_Result result = {0};
  
  ui_seed_key(ui, ui_keyf("tab_button_view_%llu", view))
  {
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
    
    ui_parent(ui, container)
      ui_font_color(ui, font_color)
      ui_border_abs(ui, v4ff(0), 0, 0) 
      ui_background_color(ui, color_blank)
    {
      UI_Comm label = ui_buttonf(ui, view_label, view);
      result.select_or_drag = (label.left_clicked || label.left_dragging);
      result.remove = ui_buttonf(ui, S8("[X]##close_button")).left_clicked;
      result.options = ui_buttonf(ui, S8("<o>##options_button")).left_clicked;
    }
  }
  return(result);
}

void
ui_do_view(App_State *app_state, View *view)
{
  if(view->is_watch) 
  {
    watch_panel(app_state, view);
  }
  else
  {
    ui_do_ui_rule_from_e_value(app_state, &view->eval_interp.memory, &view->eval_interp.value);
  }
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
                                   ui_keyf("fake_tab_move_dest_%p", panel), false, S8("Tab"));
            }
          }
          else
          {
            for(Tab *tab = panel->first_tab; tab;)
            {
              View_ID view = tab->view;
              Assert(view_id_is_valid(view));
              View *v = view_from_id(app_state, view);
              String8 view_label = S8Struct(v->title, v->title_size);
              
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
                    Range_V2f rect = ui_node_rng_v2f(cached);
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
                                       tab_button_container_key, is_selected_tab, view_label);
                }
                
                // NOTE(erb): draw the actuall tab (or original drag ghost)
                {
                  Tab_Button_Kind tab_kind = (view_id_equals(app_state->moving_view, view)
                                              ? TabButtonKind_DragSource : TabButtonKind_Regular);
                  Tab_Button_Result button_result = draw_tab_button_kind(tab_kind, view, app_state, panel, 
                                                                         tab_button_container_key, is_selected_tab, view_label);
                  // NOTE(erb): start global view/tab drag
                  if(button_result.select_or_drag)
                  {
                    panel->current_view = view;
                    app_state->moving_view = view;
                  }
                  remove = button_result.remove;
                  if(button_result.options)
                  {
                    panel->is_editing_view_params = true;
                  }
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
                                       tab_button_container_key, is_selected_tab, view_label);
                }
                
              }
              
              tab = tab->next;
              if(remove) // NOTE(erb): removal
              {
                panel_remove_view_reset_current(panel, view);
                panel->is_editing_view_params = false;
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
              Range_V2f rng = ui_node_rng_v2f(new_tab_comm.node);
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
            // NOTE(erb): actually do the thing
            Assert(panel_tab_from_view(panel, panel->current_view) != 0);
            View *view = view_from_id(app_state, panel->current_view);
            ui_do_view(app_state, view);
            
            panel->is_editing_view_params = (!view->is_watch && panel->is_editing_view_params && !key_pressed(ui, GFXKey_Escape));
            if(panel->is_editing_view_params) 
            {
              // NOTE(erb): options editing
              Scratch options_scratch = arena_get_scratch(scratch.arena);
              
              I32 selection_cursor = 0;
              B32 enter_pressed = false;
              U32 eval_max_depth = 3;
              IR_Type_Table *type_table = &app_state->type_table;
              
#if 0
              IR_Type_ID *custom_exp_types = app_state->ir_view_types;
              U32 custom_exp_count = app_state->ir_view_types_count;
#else
              IR_Type_ID *custom_exp_types = 0;
              U32 custom_exp_count = 0;
#endif
              
              Eval_Watch_ID watch_id = watch_id_make(0, 0);
              Watch_Expansion_State expansion_state = {0};
              watch_expansion_state_init(&expansion_state);
              watch_expansion_toggle(&expansion_state, watch_id);
              
              // NOTE(erb): tiny watch window?
              {
                Eval_Block_Tree *tree = push_block_tree_expanded(options_scratch.arena, S8("options"), S8Z(), watch_id,
                                                                 &view->eval_interp.value, &view->eval_interp.memory, 
                                                                 type_table, custom_exp_types, custom_exp_count);
                
                // NOTE(erb): block list
                Eval_Block_Range_List block_list = {0};
                push_block_range_list_from_tree_recur(options_scratch.arena, &block_list, tree, 0);
                
                ui_center_popup(ui)
                {
                  ui_set_next_sizing(ui, ui_grow(), ui_grow());
                  ui_set_next_background_color(ui, color_black);
                  ui_set_next_layout_axis(ui, Axis2_Y);
                  ui_parentf(ui, UIFlags_DrawBackGround, S8Z())
                  {
                    ui_watch_window_rows(app_state, ui, &expansion_state, 
                                         type_table, &block_list, selection_cursor, 
                                         enter_pressed, eval_max_depth, false);
                  }
                }
              }
              
              scratch_end(&options_scratch);
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
#define T(s, e) (UI_Cell){ .label = S8Lit(s), .filter_needle = S8Lit(s), .flags = UICellFlags_Clickable, .data = (void *)e, }
        UI_Cell tabs_opt_cell_arr[] = 
        {
          T("Calendar", "calendar()"),
          T("Lister", "lister(events)"),
          T("Watch", "watch"),
          T("New Event", "focus(event(1))"),
          T("Tags", "lister(tags)"),
        };
#undef T
        if(!was_picking_new_tab)
        {
          ui_push_nav_interface_key(ui, tab_popup_key);
        }
        
        UI_Lister_Popup_Result result = ui_cell_lister_filtered_floating_popup(ui, new_tab_button_bottom_left, tab_popup_key, 
                                                                               tabs_opt_cell_arr, ArraySize(tabs_opt_cell_arr), 
                                                                               S8("Tab"));
        
        if(result.lister.selected != 0)
        {
          String8 expr_str = S8CStr((char *)result.lister.selected->data);
          // NOTE(erb): CRITICAL CODE (creating tab/view/expr thing)
          
          // NOTE(erb): push view
          View *new_view = push_view(app_state);
          
          B32 is_watch = str8_equals(expr_str, S8("watch"));
          
          new_view->is_watch = is_watch;
          
          // NOTE(erb): title
          new_view->title_size = (U32)Min(ArraySize(new_view->title), result.lister.selected->label.size);
          ArrayCopy(result.lister.selected->label.value, new_view->title, new_view->title_size);
#if 0            
          // TODO(erb): translate to new IR Type system
          Expr_Environment env = {0};
          if(error.value == 0)
          {
            env.data = (void *)app_state;
            expr_env_append_var_func(&env, scratch.arena, S8("events"), app_get_events_expr);
            expr_env_append_var_func(&env, scratch.arena, S8("event"), app_get_event_from_id_expr);
            expr_env_append_var_func(&env, scratch.arena, S8("tags"), app_get_tags_expr);
            expr_env_append_var_func(&env, scratch.arena, S8("raw"), app_ui_rule_raw);
            expr_env_append_var_expr_func(&env, scratch.arena, S8("columns"), app_expand_arg_columns_ui_rule);
            
            expr_env_append_var_call(&env, scratch.arena, S8("calendar"), app_ui_rule_calendar);
            expr_env_append_var_call(&env, scratch.arena, S8("focus"), app_ui_rule_focus);
            
            Expr_Value now_value = {0};
            now_value.dt = os_get_local_date_time();
            now_value.type = ExprType_DateTime;
            expr_env_append_var_value(&env, scratch.arena, S8("now"), now_value);
          }
#endif
          
          if(!is_watch)
          {
            // NOTE(erb): init arena
            arena_init(&new_view->eval_arena, new_view->eval_memory, ArraySize(new_view->eval_memory));
            
            // TODO(erb): change this to global expr storage!!!!
            String8 error = {0};
            E_Value_Interp interp = e_value_from_text(&new_view->eval_arena, expr_str, &app_state->type_table,
                                                      app_state->top_global_iden, app_state->top_symbol_type, &error);
            Assert(error.value == 0);
            new_view->eval_interp = interp;
            
            // NOTE(erb): check if is view type
            B32 is_view_type = rose_ir_type_is_view_type(app_state, interp.value.type);
            Assert(is_view_type);
          }
          
          // NOTE(erb): append view
          panel_append_view_set_current(panel, id_from_view(app_state, new_view));
          panel->is_picking_new_tab = false;
        }
        if(result.canceled)
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
  
  if(AssertOrBool(result == 0))
  {
    if(first->path.size != 0)
    {
      first = first->next = PushStruct(arena, Directory_Entry_List);
    }
    *first = *new_entries;
    result = first;
  }
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
      ui_cell_lister_content_push(ui);
      {
        UI_Lister_Items_Result items_result = ui_cell_lister_items(ui, cell_arr, cell_count, search_term, scroll_offset, false);
        selected = items_result.selected;
        ui_cell_lister_scroll(ui, scroll_key, 
                              items_result.view_height, 
                              items_result.list_total_height, 
                              items_result.list_item_height,
                              items_result.scroll_clicks_y);
      }
      ui_cell_lister_content_pop(ui);
      
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
  
  for(EachU32(idx, 20))
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
    for(EachU32(idx, event->tag_count))
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
      UI_Lister_Result result = ui_cell_lister_filtered(ui, S8("run"), ui_keyf("command_lister"), cell_arr, cell_count);
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

E_FUNC_ADDR_IMPL(sum_func_address) // Arena *arena, E_Value *args, U32 arg_count, Buffer *stack_memory, IR_Type_Table *type_table, void *user_data
{
  E_Value result = {0};
  result.type = ir_builtin_type_id_u32;
  
  Assert(arg_count == 2);
  U32 a = *EValueGetPtr(stack_memory, &args[0], U32);
  U32 b = *EValueGetPtr(stack_memory, &args[1], U32);
  U32 sum = a + b;
  
  U8 *memory = e_value_set_push_memory(&result, arena, sizeof(U32));
  mem_copy((U8 *)&sum, memory, sizeof(U32));
  return(result);
}

IR_Type *
ir_type_record_from_rose_meta(App_State *app_state, String8 name, U32 byte_count,
                              ROSE_Meta_Field *meta_fields, U32 meta_field_count)
{
  IR_Type_Table *type_table = &app_state->type_table;
  IR_Type *record = ir_type_table_next(type_table, IRTypeKind_Record);
  record->record_name = name;
  record->byte_count = byte_count;
  
  Assert(meta_field_count >= 1 && 
         str8_equals(meta_fields[0].name, rose_meta_ui_rule_field_name_str8) &&
         meta_fields[0].byte_offset == 0);
  
  for(EachPtr(ROSE_Meta_Field, meta, meta_fields, meta_field_count))
  {
    IR_Type_ID type_id_table[] =
    {
      [ROSEMetaTypeKind_U32] = ir_builtin_type_id_u32,
      [ROSEMetaTypeKind_B32] = ir_builtin_type_id_b32,
      [ROSEMetaTypeKind_Calendar_Date] = app_state->ir_type_id__calendar_date,
    };
    StaticAssert(ArraySize(type_id_table) == ROSEMetaTypeKind_COUNT, type_id_table_has_all_rose_types);
    Assert(meta->type_kind < ArraySize(type_id_table));
    IR_Type_ID type_id = type_id_table[meta->type_kind];
    Assert(!ir_type_id_is_null(type_id));
    IR_Field_Type *ir_field = ir_type_next_field(record, meta->name, type_id);
    ir_field->byte_offset = meta->byte_offset;
  }
  
  return(record);
}

#define IRTypeFromROSEMetaViewStruct(app_state, Struct_Name) \
ir_type_record_from_rose_meta((app_state), S8(Stringify(Struct_Name)), sizeof(Struct_Name), \
ROSEMetaViewStructFieldsName(Struct_Name), \
ArraySize(ROSEMetaViewStructFieldsName(Struct_Name)))

/////////////////////////////////////////////////////////////////////////
// NOTE(erb): view types & functions

E_FUNC_ADDR_IMPL(calendar_view_func_address) // Arena *arena, E_Value *args, U32 arg_count, Buffer *stack_memory, IR_Type_Table *type_table, void *user_data
{
  Assert(user_data != 0);
  App_State *app_state = (App_State *)user_data;
  
  E_Value result = {0};
  result.type = app_state->ir_type_id__calendar_view;
  U8 *base = e_value_set_push_memory(&result, arena, sizeof(ROSE_Calendar_View));
  ROSE_Calendar_View *cal_view = (ROSE_Calendar_View *)base;
  cal_view->calendar_start = os_get_local_date_time().date;
  cal_view->calendar_grid_x = 1;
  cal_view->calendar_grid_y = 2;
  cal_view->ROSE_META_UI_RULE_FIELD_NAME = ROSEUIRule_Calendar;
  return(result);
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
      View *start_view = push_view(app_state);
      start_view->is_watch = true;
      panel_append_view_set_current(app_state->root_panel, id_from_view(app_state, start_view));
      {
        String8 tab_title = S8("Start Watch");
        start_view->title_size = (U32)Min(ArraySize(start_view->title), tab_title.size);
        ArrayCopy(tab_title.value, start_view->title, start_view->title_size);
      }
      
      ///////////////////////////////////////////////////////////////////////////////////////////////////////
      // NOTE(erb): new eval system work!!!!!!!
      ///////////////////////////////////////////////////////////////////////////////////////////////////////
      {
        IR_Type_Table *type_table = &app_state->type_table;
        ir_type_table_init(type_table);
        
        IR_Type *u32_type = ir_type_from_id(type_table, ir_builtin_type_id_u32);
        //IR_Type *b32_type = ir_type_from_id(&type_table, ir_builtin_type_id_b32);
        
        {
          IR_Symbol_Type *func_symbol = &app_state->func_symbol;
          E_Global_Iden *func_addr = &app_state->func_addr;
          
          // NOTE(erb): type
          IR_Type *type = ir_type_table_next(type_table, IRTypeKind_Function);
          type->sub_type = ir_builtin_type_id_u32;
          ir_type_append_field_packed(type, S8Z(), u32_type);
          ir_type_append_field_packed(type, S8Z(), u32_type);
          
          func_symbol->type = type->id;
          func_symbol->name = S8("sum");
          SLLStackPush(app_state->top_symbol_type, func_symbol);
          
          // NOTE(erb): address
          func_addr->name = func_symbol->name;
          func_addr->type = type->id;
          func_addr->func = sum_func_address;
          func_addr->is_value_not_func = false;
          SLLStackPush(app_state->top_global_iden, func_addr);
        }
        
        {
          U32 iden_value = app_state->iden_value;
          IR_Symbol_Type *iden_symbol = &app_state->iden_symbol;
          E_Global_Iden *iden_addr = &app_state->iden_addr;
          
          iden_symbol->type = ir_builtin_type_id_u32;
          iden_symbol->name = S8("g_num");
          SLLStackPush(app_state->top_symbol_type, iden_symbol);
          
          iden_addr->name = iden_symbol->name;
          iden_addr->type = ir_builtin_type_id_u32;
          iden_addr->value_bytes = S8Struct((U8 *)&iden_value, sizeof(iden_value));
          iden_addr->is_value_not_func = true;
          SLLStackPush(app_state->top_global_iden, iden_addr);
        }
        
        {
          Thing *thing_arr = app_state->thing_arr;
          U32 thing_arr_size = ArraySize(app_state->thing_arr);
#define TH(_num1, _num2, _bool1) (Thing){.num1 = _num1, .num2 = _num2, .bool1 = _bool1,}
          thing_arr[0] = TH(1, 2, true);
          thing_arr[1] = TH(2, 3, false);
          thing_arr[2] = TH(3, 4, false);
          thing_arr[3] = TH(4, 5, true);
          thing_arr[4] = TH(420, 69, false);
#undef TH
          
          IR_Symbol_Type *thing_symbol = &app_state->thing_symbol;
          E_Global_Iden *thing_iden = &app_state->thing_iden;
          
          IR_Type *thing_type = ir_type_table_next(type_table, IRTypeKind_Record);
          thing_type->record_name = S8("Thing");
          thing_type->byte_count = sizeof(Thing); 
          IR_Field_Type *num1_field = ir_type_next_field(thing_type, S8("num1"), ir_builtin_type_id_u32);
          IR_Field_Type *num2_field = ir_type_next_field(thing_type, S8("num2"), ir_builtin_type_id_u32);
          IR_Field_Type *bool1_field = ir_type_next_field(thing_type, S8("bool1"), ir_builtin_type_id_b32);
          num1_field->byte_offset = offsetof(Thing, num1);
          num2_field->byte_offset = offsetof(Thing, num2);
          bool1_field->byte_offset = offsetof(Thing, bool1);
          
          IR_Type *thing_array = ir_type_make_array(type_table, thing_type->id);
          IR_Field_Type *data_field = (thing_array->fields + ir_array_data_field_idx);
          IR_Field_Type *count_field = (thing_array->fields + ir_array_count_field_idx);
          String8 thing_array_memory = push_str8(&app_state->arena, thing_array->byte_count);
          U8 *data_field_memory = (thing_array_memory.value + data_field->byte_offset);
          U8 *count_field_memory = (thing_array_memory.value + count_field->byte_offset);
          
          U64 *data_field_ptr = (U64 *)data_field_memory;
          *data_field_ptr = (U64)(void *)thing_arr;
          U32 *count_field_ptr = (U32 *)count_field_memory;
          *count_field_ptr = thing_arr_size;
          
          thing_symbol->type = thing_array->id;
          thing_symbol->name = S8("g_array");
          SLLStackPush(app_state->top_symbol_type, thing_symbol);
          
          thing_iden->name = thing_symbol->name;
          thing_iden->type = thing_array->id;
          thing_iden->is_value_not_func = true;
          thing_iden->value_bytes = thing_array_memory;
          SLLStackPush(app_state->top_global_iden, thing_iden);
        }
        
        
        // NOTE(erb): VIEW TYPE THINGS
        {
          IR_Type *calendar_date_type = 0;
          {
            calendar_date_type = ir_type_table_next(type_table, IRTypeKind_Record);
            app_state->ir_type_id__calendar_date = calendar_date_type->id;
            
            calendar_date_type->byte_count = sizeof(Calendar_Date);
            IR_Field_Type *year_field = ir_type_next_field(calendar_date_type, S8("year"), ir_builtin_type_id_u32);
            IR_Field_Type *month_field = ir_type_next_field(calendar_date_type, S8("month"), ir_builtin_type_id_u32);
            IR_Field_Type *day_field = ir_type_next_field(calendar_date_type, S8("day"), ir_builtin_type_id_u32);
            year_field->byte_offset = offsetof(Calendar_Date, year);
            month_field->byte_offset = offsetof(Calendar_Date, month);
            day_field->byte_offset = offsetof(Calendar_Date, day);
          }
          
          // NOTE(erb): calendar view type
          IR_Type *calendar_view_type = 0;
          {
            calendar_view_type = IRTypeFromROSEMetaViewStruct(app_state, ROSE_Calendar_View);
            
            app_state->ir_type_id__calendar_view = calendar_view_type->id;
            app_state->ir_view_types[app_state->ir_view_types_count++] = calendar_view_type->id;
          }
          
          IR_Type *calendar_func_type = 0;
          {
            calendar_func_type = ir_type_table_next(type_table, IRTypeKind_Function);
            calendar_func_type->sub_type = calendar_view_type->id;
          }
          
          {
            IR_Symbol_Type *calendar_func_symbol = &app_state->calendar_func_symbol;
            E_Global_Iden *calendar_func_iden = &app_state->calendar_func_iden;
            
            calendar_func_symbol->type = calendar_func_type->id;
            calendar_func_symbol->name = S8("calendar");
            SLLStackPush(app_state->top_symbol_type, calendar_func_symbol);
            
            calendar_func_iden->type = calendar_func_type->id;
            calendar_func_iden->name = calendar_func_symbol->name;
            calendar_func_iden->func = calendar_view_func_address;
            calendar_func_iden->user_data = app_state;
            calendar_func_iden->is_value_not_func = false;
            SLLStackPush(app_state->top_global_iden, calendar_func_iden);
          }
          
          Assert(app_state->ir_view_types_count != 0);
          Assert(!ir_type_id_is_null(app_state->ir_type_id__calendar_date));
        }
      }
    }
    
#if DEBUG
    global_ui_debug_ctx = &app_state->debug_ctx;
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
      for(EachU32(idx, key_map->count))
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
          for(EachU32(key_idx, count))
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