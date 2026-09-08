
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

#define bind(key, modifiers, flags, delta) \
[(key)][(modifiers & GFXModifiers_Control) != 0][((modifiers) & GFXModifiers_Shift) != 0] = {(flags),(delta),0}
Text_Action text_action_from_event_table[][2][2] = 
{
  bind(GFXKey_Left, 0, TextActionFlags_DeltaPickSelectSide, -1),
  bind(GFXKey_Left, GFXModifiers_Shift, TextActionFlags_KeepMark, -1),
  bind(GFXKey_Left, GFXModifiers_Control, TextActionFlags_WordScan, -1),
  bind(GFXKey_Left, GFXModifiers_Shift|GFXModifiers_Control, TextActionFlags_KeepMark|TextActionFlags_WordScan, -1),
  
  bind(GFXKey_Right, 0, TextActionFlags_DeltaPickSelectSide, 1),
  bind(GFXKey_Right, GFXModifiers_Shift, TextActionFlags_KeepMark, 1),
  bind(GFXKey_Right, GFXModifiers_Control, TextActionFlags_WordScan, 1),
  bind(GFXKey_Right, GFXModifiers_Shift|GFXModifiers_Control, TextActionFlags_KeepMark|TextActionFlags_WordScan, 1),
  
  bind(GFXKey_Delete, 0, TextActionFlags_Delete|TextActionFlags_ZeroDeltaWithSelect, 1),
  bind(GFXKey_Delete, GFXModifiers_Control, TextActionFlags_Delete|TextActionFlags_WordScan|TextActionFlags_ZeroDeltaWithSelect, 1),
  
  bind(GFXKey_Backspace, 0, TextActionFlags_Delete|TextActionFlags_ZeroDeltaWithSelect, -1),
  bind(GFXKey_Backspace, GFXModifiers_Control, TextActionFlags_Delete|TextActionFlags_WordScan|TextActionFlags_ZeroDeltaWithSelect, -1),
  
  bind(GFXKey_C, GFXModifiers_Control, TextActionFlags_Copy,  0),
  bind(GFXKey_X, GFXModifiers_Control, TextActionFlags_Cut,   0),
  bind(GFXKey_V, GFXModifiers_Control, TextActionFlags_Paste, 0),
  
  // emacs style
  bind(GFXKey_F, GFXModifiers_Control, TextActionFlags_DeltaPickSelectSide, 1),
  //bind(GFXKey_F, GFXModifiers_Alt, TextActionFlags_WordScan, 1),
  
  bind(GFXKey_B, GFXModifiers_Control, TextActionFlags_DeltaPickSelectSide, -1),
  //bind(GFXKey_B, GFXModifiers_Alt, TextActionFlags_WordScan, -1),
};
#undef bind

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
    if(clipboard_api->push_str8_func != 0)
    {
      cmd.replace_str = clipboard_api->push_str8_func(arena, clipboard_api->data);
      cmd.replace_range = rng_u64(state->cursor, state->mark);
    }
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
    
    if (text_cmd.copy_str.size > 0 && clipboard_api->set_str8_func != 0)
    {
      clipboard_api->set_str8_func(text_cmd.copy_str, clipboard_api->data);
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
      result->writer.str.value = result->buffer;
      result->writer.capacity = ArraySize(result->buffer);
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

UI_Line_Input_State *
ui_line_input(UI_Ctx *ui, UI_Key key)
{
  UI_Line_Input_State *state = ui_line_input_state_from_key(ui, key);
  Assert(state);
  state->scroll_timer = Cap0(state->scroll_timer - (F32)ui->frame_delta_time);
  
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
    ui_set_next_debug_str(ui, S8("line_input_container"));
    ui_set_next_sizing_x(ui, ui_grow());
    ui_set_next_hover_cursor(ui, GFXCursorKind_Beam);
    container = ui_node_make(ui, UIFlags_DrawBorder
                             |UIFlags_DrawRounded
                             |UIFlags_DrawBackGround
                             |UIFlags_Comm
                             |UIFlags_ClipRectangle
                             |UIFlags_DontSeedKey, 
                             key, S8Z());
    container->size.y = (text_size.y + padding_axis_sum(container->padding, Axis2_Y));
    container->sizing[Axis2_Y] = UISizing_Fixed;
    if(is_focused)
    {
      container->border_color = color_blue;
      container->border_thickness = 2.0f;
    }
  }
  
  Range_F32 inner_x = {0};
  V2f input_pos = {0};
  
  UI_Comm out = ui_comm_from_node(ui, container);
  B32 mouse_action_requested = (out.left_down || out.left_pressed);
  if(out.node)
  {
    Range_V2f inner = node_inner_rng_v2f(out.node);
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
    ui_parentf(ui, UIFlags_Floating|UIFlags_DrawText, state->writer.str);
    ui_cursor_and_highlight(ui, input_pos, text_size.y, &text, &state->edit_state, inner_x.max);
  }
  
  if(out.hovered)
  {
    ui->cursor_api.set_func(GFXCursorKind_Beam, ui->cursor_api.data);
  }
  
  return(state);
}