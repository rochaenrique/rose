// ///////////////////////////////////////////////
// NOTE(erb): regular button
// ///////////////////////////////////////////////

UI_Comm
ui_buttonf(UI_Ctx *ui, String8 label, ...)
{
  Scratch scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, label);
  UI_String8_Key str_key = ui_parse_label_str_key_varg(scratch.arena, label, args);
  va_end(args);
  
  ui_set_next_hover_cursor(ui, GFXCursorKind_Hand);
  UI_Node *container = ui_node_make(ui, 
                                    UIFlags_Comm|
                                    UIFlags_DrawBackGround|
                                    UIFlags_DrawRounded|
                                    UIFlags_DrawBorder|
                                    UIFlags_HoverEffect, 
                                    str_key.key, str_key.str);
  ui_parent(ui, container)
	{
    ui_labelf(ui, str_key.str);
	}
  scratch_end(&scratch);
	UI_Comm result = ui_comm_from_node(ui, container);
  if(ui_is_top_nav_interface(ui, container->key))
  {
    container->border_color = color_red;
    container->border_thickness = 2.0f;
  }
	return(result);
}

// ///////////////////////////////////////////////
// NOTE(erb): line input
// ///////////////////////////////////////////////

F32
ui_get_cursor_offset_x(UI_Text *text, I64 cursor)
{
  String8 slice = str8_prefix(text->content, cursor);
  V2f text_size = ui_measure_str8_face(slice, &text->face);
  
  if(cursor > 0 && 
     cursor < (I64)text->content.size-1)
  {
    text_size.x += text->face.char_spacing;
  }
  F32 result = text_size.x;
  return(result);
}

UI_Node *
ui_rect_color(UI_Ctx *ctx, Range_V2f range, V4f color)
{
  ui_set_next_rect(ctx, range);
  ui_set_next_background_color(ctx, color);
  UI_Node *node = ui_nodef(ctx, UIFlags_DrawBackGround|UIFlags_Floating, S8Z());
  ui_parent(ctx, node);
  return(node);
}

void 
ui_cursor_and_highlight(UI_Ctx *ctx, V2f pos, F32 height, UI_Text *text, Text_Edit_State *text_state, F32 container_max_x)
{
  F32 cursor_pos_x = pos.x + ui_get_cursor_offset_x(text, text_state->cursor);
  
  // NOTE(erb): mark
  if (text_state->mark != text_state->cursor)
  {
    F32 mark_pos_x = pos.x + ui_get_cursor_offset_x(text, text_state->mark);
    Range_F32 highlight_range_x = rng_f32_min_max(mark_pos_x, cursor_pos_x);
    Range_V2f highlight_range = rng_v2f(v2f(highlight_range_x.min, pos.y),
                                        v2f(highlight_range_x.max, pos.y + height));
    
    ui_set_next_debug_str(ctx, S8("mark"));
    ui_rect_color(ctx, highlight_range, with_alpha(color_yellow, 0.5f));
  }
  
  // NOTE(erb): cursor
  {
    V2f size = v2f(2.0f, height);
    Range_V2f rect = rng_v2f_point_add(v2f(cursor_pos_x, pos.y), size);
    if(rect.max.x >= container_max_x)
    {
      rect.min.x -= 2.0f;
      rect.max.x -= 2.0f;
    }
    
    ui_set_next_debug_str(ctx, S8("cursor"));
    ui_rect_color(ctx, rect, color_blue);
  }
}

#define BIND_ACT(key, modifiers, flags, delta) \
[(key)][(modifiers & GFXModifiers_Control) != 0][((modifiers) & GFXModifiers_Shift) != 0] = {(flags),(delta),0}
Text_Action text_action_from_event_table[][2][2] = 
{
  BIND_ACT(GFXKey_Left, 0, TextActionFlags_DeltaPickSelectSide, -1),
  BIND_ACT(GFXKey_Left, GFXModifiers_Shift, TextActionFlags_KeepMark, -1),
  BIND_ACT(GFXKey_Left, GFXModifiers_Control, TextActionFlags_WordScan, -1),
  BIND_ACT(GFXKey_Left, GFXModifiers_Shift|GFXModifiers_Control, TextActionFlags_KeepMark|TextActionFlags_WordScan, -1),
  
  BIND_ACT(GFXKey_Right, 0, TextActionFlags_DeltaPickSelectSide, 1),
  BIND_ACT(GFXKey_Right, GFXModifiers_Shift, TextActionFlags_KeepMark, 1),
  BIND_ACT(GFXKey_Right, GFXModifiers_Control, TextActionFlags_WordScan, 1),
  BIND_ACT(GFXKey_Right, GFXModifiers_Shift|GFXModifiers_Control, TextActionFlags_KeepMark|TextActionFlags_WordScan, 1),
  
  BIND_ACT(GFXKey_Delete, 0, TextActionFlags_Delete|TextActionFlags_ZeroDeltaWithSelect, 1),
  BIND_ACT(GFXKey_Delete, GFXModifiers_Control, TextActionFlags_Delete|TextActionFlags_WordScan|TextActionFlags_ZeroDeltaWithSelect, 1),
  
  BIND_ACT(GFXKey_Backspace, 0, TextActionFlags_Delete|TextActionFlags_ZeroDeltaWithSelect, -1),
  BIND_ACT(GFXKey_Backspace, GFXModifiers_Control, TextActionFlags_Delete|TextActionFlags_WordScan|TextActionFlags_ZeroDeltaWithSelect, -1),
  
  BIND_ACT(GFXKey_C, GFXModifiers_Control, TextActionFlags_Copy,  0),
  BIND_ACT(GFXKey_X, GFXModifiers_Control, TextActionFlags_Cut,   0),
  BIND_ACT(GFXKey_V, GFXModifiers_Control, TextActionFlags_Paste, 0),
  
  // emacs style
  BIND_ACT(GFXKey_F, GFXModifiers_Control, TextActionFlags_DeltaPickSelectSide, 1),
  //BIND_ACT(GFXKey_F, GFXModifiers_Alt, TextActionFlags_WordScan, 1),
  
  BIND_ACT(GFXKey_B, GFXModifiers_Control, TextActionFlags_DeltaPickSelectSide, -1),
  //BIND_ACT(GFXKey_B, GFXModifiers_Alt, TextActionFlags_WordScan, -1),
};
#undef BIND_ACT

Text_Action 
text_action_from_event(GFX_Event *event) 
{
  Text_Action action = {0};
  
  if (event->kind == GFXEventKind_KeyDown)
  {
    if (event->key.code < ArraySize(text_action_from_event_table))
    {
      B32 ctrl = (event->modifiers & GFXModifiers_Control) != 0;
      B32 shift = (event->modifiers & GFXModifiers_Shift) != 0;
      action = text_action_from_event_table[event->key.code][ctrl][shift];
    }
  }
  else if (event->kind == GFXEventKind_Text)
  {
    
    if(!is_ascii(event->key.codepoint) || char_is_printable(event->key.codepoint))
    {
      action.codepoint = event->key.codepoint;
      action.delta = 1;
    }
  }
  return(action);
}

Text_Cmd 
text_cmd_from_state_and_action(Arena *arena, Clipboard_API *clipboard_api, String8 str, Text_Edit_State *state, Text_Action *action)
{
  Text_Cmd cmd = {0};
  cmd.new_cursor = state->cursor;
  cmd.new_mark = state->mark;
  
  // NOTE(erb): navigation 
  I64 char_delta = 0;
  {
    if (action->flags & TextActionFlags_WordScan) 
    {
      char_delta = str8_scan_word_delta(str, state->cursor, action->delta);
    }
    else 
    {
      if(action->codepoint == 0)
      {
        char_delta = utf8_scan_delta(str, state->cursor, action->delta);
      }
      else
      {
        char_delta = utf8_encode(action->codepoint).size;
      }
    }
  }
  
  if (state->cursor != state->mark &&
      action->flags & TextActionFlags_ZeroDeltaWithSelect)
  {
    char_delta = 0;
  }
  
  if (state->cursor != state->mark &&
      action->flags & TextActionFlags_DeltaPickSelectSide)
  {
    char_delta = 0;
    if (action->delta > 0) 
    {
      cmd.new_cursor = Max(state->cursor, state->mark);
    }
    else if (action->delta < 0)
    {
      cmd.new_cursor = Min(state->cursor, state->mark);
    }
  }
  
  // NOTE(erb): apply delta
  cmd.new_cursor = state->cursor + char_delta; 
  
  // NOTE(erb): post navigation
  
  // NOTE(erb): copy
  if (action->flags & TextActionFlags_Copy || 
      action->flags & TextActionFlags_Cut) 
  {
    String8 slice = str8_slice(str, rng_u64_min_max(state->cursor, state->mark));
    cmd.copy_str = slice;
  }
  
  // NOTE(erb): delete
  if (action->flags & TextActionFlags_Delete || 
      action->flags & TextActionFlags_Cut)
  {
    cmd.replace_range = rng_u64_min_max(cmd.new_cursor, cmd.new_mark);
    cmd.new_cursor = cmd.new_mark = cmd.replace_range.min;
  }
  
  // NOTE(erb): insert
  if (action->codepoint != 0)
  {
    cmd.replace_str = push_str8_utf8_encoded(arena, &action->codepoint, 1);
    cmd.replace_range = rng_u64(state->cursor, state->mark);
  }
  // NOTE(erb): paste
  else if (action->flags & TextActionFlags_Paste)
  {
    cmd.replace_str = clipboard_api_push_str8(clipboard_api, arena);
    cmd.replace_range = rng_u64(state->cursor, state->mark);
  }
  
  // NOTE(erb): keep mark
  if (!(action->flags & TextActionFlags_KeepMark))
  {
    cmd.new_mark = cmd.new_cursor;
  }
  return(cmd);
}

B32 
apply_text_action(Clipboard_API *clipboard_api, Text_Action action, Text_Edit_State *state, String8_Writer *input_writer)
{
  B32 did_apply = false;
  
  if (action.delta != 0 || action.codepoint != 0 ||
      action.flags != 0)
  {
    Scratch scratch = scratch_begin(0, 0);
    
    Text_Cmd text_cmd = text_cmd_from_state_and_action(scratch.arena, clipboard_api,
                                                       input_writer->str, state, &action);
    str8_writer_replace(input_writer, text_cmd.replace_str, text_cmd.replace_range);
    
    did_apply = (state->cursor != text_cmd.new_cursor || 
                 state->mark != text_cmd.new_mark);
    
    state->cursor = Cap(text_cmd.new_cursor, 0, (I64)input_writer->str.size); 
    state->mark = Cap(text_cmd.new_mark, 0, (I64)input_writer->str.size); 
    
    if(text_cmd.copy_str.size > 0 && clipboard_api->set_str8_func != 0)
    {
      clipboard_api_set_str8(clipboard_api, text_cmd.copy_str);
    }
    
    scratch_end(&scratch);
  }
  
  return(did_apply);
}

UI_Line_Input_State *
ui_line_input_state_from_key(UI_Ctx *ui, UI_Key key)
{
  UI_Line_Input_State *result = 0;
  
  U64 index = ui_hash_from_key(key) & (ArraySize(ui->input_state_cache) - 1);
  UI_Line_Input_State **slot = (ui->input_state_cache + index);
  for(EachNode(UI_Line_Input_State, test, *slot))
  {
    if(ui_key_equals(test->key, key))
    {
      result = test;
      break;
    }
  }
  if(!result)
  {
    if(ui->first_free_input_state)
    {
      result = SLLStackPop(ui->first_free_input_state);
    }
    else
    {
      result = PushStruct(&ui->input_state_arena, UI_Line_Input_State);
      result->key = key;
      result->writer = push_str8_writer(&ui->input_state_arena, 512); // TODO(erb): arbitrary size here
      result->created_frame = ui->frame_tick;
      ui->input_state_count++;
    }
    SLLStackPush(*slot, result);
  }
  return(result);
}

F32 
rng_f32_range_overflow_prefer_start(Range_F32 container, Range_F32 test)
{
  F32 start_overflow = (container.start - test.start);
  F32 result = 0;
  if(start_overflow < 0)
  {
    result = start_overflow;
  }
  else 
  {
    F32 end_overlfow = (container.end - test.end);
    if(end_overlfow > 0)
    {
      result = end_overlfow;
    }
  }
  return(result);
}

B32
ui_line_input_changed(UI_Ctx *ui, UI_Line_Input_State *state)
{
  Assert(ui != 0);
  Assert(state != 0);
  B32 result = (ui->frame_tick <= state->last_changed_frame);
  return(result);
}

B32
ui_line_input_created(UI_Ctx *ui, UI_Line_Input_State *state)
{
  Assert(ui != 0);
  Assert(state != 0);
  B32 result = (ui->frame_tick <= state->created_frame);
  return(result);
}

void
ui_line_input_update_state(UI_Ctx *ui, UI_Line_Input_State *state)
{
  Assert(state != 0);
  state->scroll_timer = Cap0(state->scroll_timer - (F32)ui->frame_delta_time);
  
  UI_Key key = state->key;
  B32 is_focused = ui_key_equals(key, UIGetStackValue(ui, nav_interface_key));
  
  B32 key_action_happened = false;
  if(is_focused)
  {
    for(EachNode(GFX_Event, event, ui->event_list.first)) 
    {
      Text_Action action = text_action_from_event(event);
      if(apply_text_action(&ui->clipboard_api, action, &state->edit_state, 
                           &state->writer))
      {
        key_action_happened = true;
      }
    }
  }
  if(key_action_happened)
  {
    state->last_changed_frame = ui->frame_tick;
  }
  
  UI_Text text = {0};
  text.face = ui_font_face_from_ctx(ui);
  text.content = state->writer.str;
  V2f text_size = ui_measure_text(&text);
  
  // NOTE(erb): container
  UI_Node *container = 0;
  {
    if(is_focused)
    {
      ui_push_border_color(ui, color_blue);
      ui_push_border_thickness(ui, 2.0f);
    }
    
    ui_push_sizing_x(ui, ui_grow());
    ui_set_next_debug_str(ui, S8("line_input_container"));
    ui_set_next_hover_cursor(ui, GFXCursorKind_Beam);
    container = ui_node_make(ui, UIFlags_DrawBorder
                             |UIFlags_DrawRounded
                             |UIFlags_DrawBackGround
                             |UIFlags_Comm
                             |UIFlags_ClipRectangle
                             |UIFlags_DontSeedKey, 
                             key, S8Z());
    ui_pop_sizing_x(ui);
    container->size.y = (text_size.y + padding_axis_sum(container->padding, Axis2_Y));
    container->sizing[Axis2_Y] = UISizing_Fixed;
    
    if(is_focused)
    {
      ui_pop_border_color(ui);
      ui_pop_border_thickness(ui);
    }
  }
  
  Range_F32 inner_x = {0};
  V2f input_pos = {0};
  
  UI_Comm out = ui_comm_from_node(ui, container);
  B32 mouse_action_requested = (out.left_down || out.left_pressed);
  if(out.node)
  {
    Range_V2f inner = ui_node_rng_v2f_inner(out.node);
    inner_x = rng_v2f_axis(inner, Axis2_X);
    input_pos = inner.min;
    
    B32 text_overflowing = (text_size.x > rng_f32_size(inner_x));
    
    // NOTE(erb): scroll timer
    if(key_action_happened)
    {
      state->scroll_timer = 0.0f;
    }
    else if(text_overflowing && out.scroll.x != 0)
    {
      state->scroll_timer = UI_INPUT_STATE_SCROLL_TIME;
    }
    
    // NOTE(erb): mouse interaction
    if(mouse_action_requested)
    {
      U64 idx = index_from_cursor_delta((ui->input.cursor.x - (input_pos.x + state->collected_scroll_x)), &text);
      state->edit_state.cursor = idx;
      if(out.left_pressed)
      {
        state->edit_state.mark = idx;
      }
      ui_push_nav_interface_key(ui, key);
    }
    
    // NOTE(erb): scroll
    if(text_overflowing)
    {
      if(!key_action_happened)
      {
        state->collected_scroll_x += out.scroll.x;
      }
      // NOTE(erb): align the text so its always at the edges
      if(state->scroll_timer <= 0.0f)
      {
        Range_F32 input_range = rng_f32_point_add(input_pos.x + state->collected_scroll_x, text_size.x);
        state->collected_scroll_x += rng_f32_range_overflow_prefer_start(inner_x, input_range);
      }
    }
    else
    {
      state->collected_scroll_x = 0;
    }
    
    // NOTE(erb): align the text so the cursor is inside while typing
    if((state->scroll_timer <= 0.0f && key_action_happened) || mouse_action_requested)
    {
      F32 cursor_pos = (input_pos.x + state->collected_scroll_x + 
                        ui_get_cursor_offset_x(&text, state->edit_state.cursor));
      state->collected_scroll_x += rng_f32_overflow(inner_x, cursor_pos);
    }
  }
  
  // NOTE(erb): apply total scroll
  input_pos.x += state->collected_scroll_x;
  
  // NOTE(erb): actually build ui
  ui_parent(ui, container) ui_seed_key(ui, key)
  {
    if(out.node)
    {
      ui_set_next_floating_pos(ui, input_pos);
    }
    ui_set_next_debug_str(ui, S8("input_field"));
    ui_set_next_sizing_x(ui, ui_grow());
    ui_parentf(ui, UIFlags_Floating|UIFlags_DrawText, text.content);
    ui_cursor_and_highlight(ui, input_pos, text_size.y, &text, &state->edit_state, inner_x.max);
  }
  
  if(out.hovered)
  {
    ui_set_cursor(ui, GFXCursorKind_Beam);
  }
}

UI_Line_Input_State *
ui_line_input(UI_Ctx *ui, UI_Key key)
{
  UI_Line_Input_State *state = ui_line_input_state_from_key(ui, key);
  Assert(state != 0);
  ui_line_input_update_state(ui, state);
  return(state);
}


// ///////////////////////////////////////////////
// NOTE(erb): toggle switch input
// ///////////////////////////////////////////////

void
ui_toggle_switch(UI_Ctx *ui, B32 *out_b32)
{
  V4f border_color = with_alpha(v4ff(.4f), 1);
  V4f off_color = with_alpha(v4ff(.2f), 1);
  V4f on_color = color_green;
  
  // NOTE(erb): this is done like this so the caller can overide with "ui_set_next_..."
  UI_Node *node = 0;
  
  ui_background_color(ui, (*out_b32 ? on_color : off_color))
    ui_border(ui, border_color, 2.f, ui_rad(.3f, UIRadiusKind_PercentOfY))
    ui_sizing_x(ui, ui_fixed(60))
    ui_sizing_y(ui, ui_fixed(30))
  {
    ui_set_next_padding(ui, pad2_equal(5));
    node = ui_nodef(ui, 
                    UIFlags_Comm|
                    UIFlags_DrawBackGround|
                    UIFlags_DrawRounded|
                    UIFlags_DrawBorder, 
                    S8("b32_toggle_switch_%p"), out_b32);
  }
  UI_Comm comm = ui_comm_from_node(ui, node);
  if(comm.left_clicked)
  {
    *out_b32 = !(*out_b32);
  }
  
  ui_parent(ui, node)
  {
    if(comm.node != 0)
    {
      // NOTE(erb): inner indicator
      Range_V2f container = ui_node_rng_v2f_inner(comm.node);
      V2f container_size = rng_v2f_size(container);
      
      Range_V2f indicator_rng = container; 
      if(*out_b32)
      {
        indicator_rng.min.x = indicator_rng.max.x - container_size.x*0.5f;
      }
      else
      {
        indicator_rng.max.x = indicator_rng.min.x + container_size.x*0.5f;
      }
      
      ui_set_next_rect(ui, indicator_rng);
      ui_set_next_background_color(ui, color_white);
      ui_set_next_corner_radius(ui, node->corner_radius);
      ui_parentf(ui, UIFlags_DrawBackGround|UIFlags_Floating|UIFlags_DrawRounded, S8Z());
    }
  }
}


// ///////////////////////////////////////////////
// NOTE(erb): i32 input
// ///////////////////////////////////////////////

void
ui_rng_i32_input_compact(UI_Ctx *ui, I32 *value, Range_I32 range, String8 format)
{
  ui_set_next_align_x(ui, UIAlign_End);
  
  UI_Node *node = ui_nodef(ui, 
                           UIFlags_Comm|
                           UIFlags_DrawBackGround|
                           UIFlags_DrawRounded|
                           UIFlags_DrawBorder, 
                           S8("rng_u32_input_%p"), value);
  
  UI_Comm comm = ui_comm_from_node(ui, node);
  ui_parent(ui, node)
  {
    if(comm.node != 0)
    {
      Range_V2f internal = ui_node_rng_v2f(comm.node);
      F32 total_width = (internal.max.x - internal.min.x);
      
      F32 progress_width = (F32)(*value)/rng_i32_size(range) * total_width;
      internal.max.x = internal.min.x + progress_width;
      if(progress_width > 0)
      {
        internal = rng_v2f_center_sub(internal, v2ff(node->border_thickness));
      }
      
      ui_set_next_rect(ui, internal);
      ui_set_next_background_color(ui, with_alpha(color_red, 0.3f));
      ui_parentf(ui, 
                 UIFlags_Floating|
                 UIFlags_DrawBackGround|
                 UIFlags_DrawRounded, S8Z());
    }
    ui_labelf(ui, format, *value);
  }
  
  if(comm.left_dragging)
  {
    node->border_color = color_red;
    F32 value_post = ((F32)(*value) + ui->input.cursor_delta.x);
    value_post = Cap(value_post, (F32)range.min, (F32)range.max);
    *value = (U32)floor_f32_to_i32(value_post);
  }
}

void
ui_rng_f32_slider(UI_Ctx *ui, F32 *value, Range_F32 range)
{
  V4f border_color = with_alpha(v4ff(.4f), 1);
  V4f bg_color = v4f(1, .7f, .2f, 1);
  
  // NOTE(erb): this is done like this so the caller can overide with "ui_set_next_..."
  UI_Node *node = 0;
  
  ui_border(ui, border_color, 2.f, ui_rad(.3f, UIRadiusKind_PercentOfY))
    ui_sizing_x(ui, ui_fixed(100))
    ui_sizing_y(ui, ui_fixed(30))
  {
    ui_set_next_padding(ui, pad2_equal(5));
    node = ui_nodef(ui, 
                    UIFlags_Comm|
                    UIFlags_DrawBackGround|
                    UIFlags_DrawRounded|
                    UIFlags_DrawBorder, 
                    S8("rng_i32_slider_%p"), value);
  }
  UI_Comm comm = ui_comm_from_node(ui, node);
  if(comm.node != 0)
  {
    Range_V2f node_rng = ui_node_rng_v2f(comm.node);
    F32 node_width = (node_rng.max.x - node_rng.min.x);
    
    if(comm.left_dragging)
    {
      F32 delta = (ui->input.cursor_delta.x / node_width) * rng_f32_size(range);
      F32 value_f32 = (*value) + delta;
      *value = Cap(value_f32, range.min, range.max);
    }
    
    ui_parent(ui, node)
    {
      // NOTE(erb): inner indicator
      Range_V2f indicator_rng = node_rng;
      F32 progress_width = (F32)(*value)/rng_f32_size(range) * node_width;
      indicator_rng.max.x = indicator_rng.min.x + progress_width;
      
      ui_set_next_rect(ui, indicator_rng);
      ui_set_next_background_color(ui, bg_color);
      ui_set_next_border(ui, node->border_color, node->border_thickness, node->corner_radius);
      ui_parentf(ui, 
                 UIFlags_DrawBackGround|
                 UIFlags_Floating|
                 UIFlags_DrawRounded|
                 UIFlags_DrawBorder, S8Z());
    }
  }
}

void
ui_rng_i32_slider(UI_Ctx *ui, I32 *value, Range_U32 range)
{
  F32 value_f32 = (F32)(*value);
  Range_F32 range_f32 = rng_f32((F32)range.min, (F32)range.max);
  ui_rng_f32_slider(ui, &value_f32, range_f32);
  *value = round_f32_to_i32(value_f32);
}

void
ui_v3i_input(UI_Ctx *ui, String8 label, V3i *value, Range_V3i range, String8 fmt1, String8 fmt2, String8 fmt3)
{
  ui_seed_key(ui, ui_keyf("v2i_input_%S_%b", label, value));
  ui_set_next_sizing_x(ui, ui_grow());
  ui_set_next_layout_axis(ui, Axis2_X);
  ui_set_next_align_y(ui, UIAlign_Center);
  ui_parentf(ui, 0, S8Z())
  {
    ui_set_next_font_size(ui, 19.0f);
    ui_labelf(ui, label);
    
    ui_sizing_x(ui, ui_grow())
    {
      String8 fmt_arr[] = { fmt1, fmt2, fmt3 };
      for(EachU32(idx, 3))
      {
        I32 *v = (value->c + idx);
        Range_I32 rng = rng_i32(range.min.c[idx], range.max.c[idx]);
        String8 fmt = fmt_arr[idx];
        ui_rng_i32_input_compact(ui, v, rng, fmt);
      }
    }
  }
  
}
