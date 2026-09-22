///////////////////////////////////////////////////////////
// NOTE(erb): fancy str

Fancy_String8 *
fancy_str8_push(Arena *arena, Fancy_String8_List *list, String8 str, V4f color)
{
  Fancy_String8 *result = PushStruct(arena, Fancy_String8);
  result->str = str;
  result->color = color;
  SLLQueueAppend(list->first, list->last, result);
  return(result);
}

void
ui_fancy_str(UI_Ctx *ui, Fancy_String8_List fancy_strs)
{
  ui_set_next_layout_axis(ui, Axis2_X);
  ui_parentf(ui, 0, S8Z())
  {
    for(EachNode(Fancy_String8, str, fancy_strs.first))
    {
      ui_set_next_font_color(ui, str->color);
      ui_labelf(ui, str->str);
    }
  }
}

///////////////////////////////////////////////////////////
// NOTE(erb): color picker

void
ui_color_picker(UI_Ctx *ui, V4f *color)
{
  ui_set_next_sizing(ui, ui_fixed(400), ui_fixed(200));
  ui_set_next_layout_axis(ui, Axis2_X);
  ui_set_next_background_color(ui, with_alpha(v4ff(0.2f), 1.0f));
  ui_set_next_border_abs(ui, color_white, 1.0f, 0);
  ui_set_next_padding(ui, pad2_equal(1));
  ui_parentf(ui, UIFlags_DrawBackGround|UIFlags_DrawBorder, S8Z())
  {
    ui_set_next_sizing_x(ui, ui_grow());
    ui_set_next_padding(ui, pad2_equal(4));
    ui_set_next_layout_axis(ui, Axis2_Y);
    ui_parentf(ui, 0, S8Z())
    {
      
      // NOTE(erb): rgb
      {
        V3f rgb = rgb_from_rgba(*color);
        V3i rgb255 = rgb_255_from_rgb_f32(rgb);
        Range_V3i rng = rng_v3i(v3ii(0), v3ii(255));
        String8 fmt = S8("%u");
        
        ui_v3i_input(ui, S8("rgb"), &rgb255, rng, fmt, fmt, fmt);
        
        color->rgb = rgb_f32_from_rgb_255(rgb255);
        color->a = 1.0f;
      }
      
#if 0      
      // NOTE(erb): hsv
      {
        V3f rgb = rgb_from_rgba(*color);
        V3f hsv = hsv_from_rgb(rgb);
        V3i hsv_i32 = v3f_floor_to_v3i(v3f(hsv.h, 
                                           hsv.s*100.0f,
                                           hsv.v*100.0f));
        Range_V3i rng = rng_v3i(v3ii(0), v3i(360, 100, 100));
        {
          ui_v3i_input(ui, S8("hsv"), &hsv_i32, rng, 
                       S8("%u*"), S8("%u%%"), S8("%u%%"));
        }
        hsv.h = (F32)hsv_i32.h;
        hsv.s = (F32)hsv_i32.s / 100.0f;
        hsv.h = (F32)hsv_i32.v / 100.0f;
        color->rgb = rgb_from_hsv(hsv);
        color->a = 1.0f;
      }
#endif
      
      // NOTE(erb): hex
      {
        V3f rgb = rgb_from_rgba(*color);
        ui_labelf(ui, S8("#%x"), rgb_to_packed_u32(rgb));
      }
    }
    
    ui_set_next_background_color(ui, *color);
    ui_set_next_sizing(ui, ui_fixed(200), ui_grow());
    ui_parentf(ui, UIFlags_DrawBackGround, S8Z());
  }
}

UI_Comm
ui_color_picker_trigger(UI_Ctx *ui, B32 is_open, V4f color)
{
  UI_Node *trigger = 0;
  ui_set_next_background_color(ui, color);
  ui_border_abs(ui, (is_open ? color_black : color_white), 1.0f, 1.0f)
  {
    trigger = ui_nodef(ui, 
                       UIFlags_DrawBackGround|
                       UIFlags_DrawBorder|
                       UIFlags_DrawRounded|
                       UIFlags_Comm, 
                       S8("color_picker_container"));
  }
  ui_parent(ui, trigger);
  UI_Comm comm = ui_comm_from_node(ui, trigger);
  return(comm);
}

void
ui_color_picker_popup(UI_Ctx *ui, V2f start_position, B32 *is_open, V4f *color)
{
  // TODO(erb): pass in which side to open
  ui_set_next_floating_pos(ui, start_position);
  ui_set_next_sizing(ui, ui_fit(), ui_fit());
  UI_Node *window = ui_nodef(ui, UIFlags_Comm|UIFlags_Floating, S8("picker_wdw_%p"), color);
  UI_Comm window_comm = ui_comm_from_node(ui, window);
  
  // NOTE(erb): floating position calculation
  if(window_comm.node != 0)
  {
    window->pos = window_comm.node->pos;
    
    Range_V2f root = node_rng_v2f(ui->root);
    Range_V2f range = node_rng_v2f(window_comm.node);
    
    for(U32Each(axis, 1))
    {
      Range_F32 root_axis = rng_v2f_axis(root, axis);
      Range_F32 range_axis = rng_v2f_axis(range, axis);
      
      if(range_axis.min < root_axis.min)
      {
        window->pos.x = root_axis.min + 5;
      }
      if(range_axis.max > root_axis.max)
      {
        window->pos.c[axis] = root_axis.max - window_comm.node->size.c[axis] - 5;
      }
    }
  }
  
  if(window_comm.pressed_out && *is_open)
  {
    *is_open = false;
  }
  
  ui_parent(ui, window)
  {
    ui_seed_key(ui, ui_keyf("color_picker_floating_%p", color));
    ui_color_picker(ui, color);
  }
}

///////////////////////////////////////////////////////////
// NOTE(erb): date pickers

I32
ui_date_picker_with_arrows(UI_Ctx *ui, UI_Key key, Calendar_Date date)
{
  I32 delta = 0;
  
  ui_set_next_layout_axis(ui, Axis2_X);
  ui_set_next_align_x(ui, UIAlign_Center);
  ui_set_next_align_y(ui, UIAlign_Center);
  ui_set_next_child_gap(ui, 15.0f);
  ui_set_next_sizing_x(ui, ui_grow());
  ui_set_next_sizing_y(ui, ui_grow());
  ui_parentf(ui, 0, S8Z())
    ui_seed_key(ui, key)
  {
    ui_set_next_border_abs(ui, v4ff(0), 0, 0);
    ui_set_next_background_color(ui, with_alpha(v4ff(0.1f), 1.0f));
    ui_set_next_padding(ui, pad2_xy(4.0f, 0));
    if(ui_buttonf(ui, S8("<##less_button_date")).left_clicked)
    {
      delta -= 1;
    }
    
    ui_labelf(ui, S8("%u"), date.day);
    ui_labelf(ui, S8("%u"), date.month);
    ui_labelf(ui, S8("%u"), date.year);
    
    ui_set_next_border_abs(ui, v4ff(0), 0, 0);
    ui_set_next_background_color(ui, with_alpha(v4ff(0.1f), 1.0f));
    ui_set_next_padding(ui, pad2_xy(4.0f, 0));
    if(ui_buttonf(ui, S8(">##more_button_date")).left_clicked)
    {
      delta += 1;
    }
  }
  return(delta);
}

I32
ui_time_picker_with_arrows(UI_Ctx *ui, UI_Key key, Clock_Time time)
{
  I32 delta = 0;
  
  ui_set_next_layout_axis(ui, Axis2_X);
  ui_set_next_align_x(ui, UIAlign_Center);
  ui_set_next_align_y(ui, UIAlign_Center);
  ui_set_next_child_gap(ui, 15.0f);
  ui_set_next_sizing_x(ui, ui_grow());
  ui_set_next_sizing_y(ui, ui_grow());
  ui_parentf(ui, 0, S8Z())
    ui_seed_key(ui, key)
  {
    ui_set_next_border_abs(ui, v4ff(0), 0, 0);
    ui_set_next_background_color(ui, with_alpha(v4ff(0.1f), 1.0f));
    ui_set_next_padding(ui, pad2_xy(4.0f, 0));
    if(ui_buttonf(ui, S8("<##less_button_time")).left_clicked)
    {
      delta -= 1;
    }
    
    ui_labelf(ui, S8("%02u"), time.hour);
    ui_labelf(ui, S8("%02u"), time.minute);
    
    ui_set_next_border_abs(ui, v4ff(0), 0, 0);
    ui_set_next_background_color(ui, with_alpha(v4ff(0.1f), 1.0f));
    ui_set_next_padding(ui, pad2_xy(4.0f, 0));
    if(ui_buttonf(ui, S8(">##more_button_time")).left_clicked)
    {
      delta += 1;
    }
  }
  return(delta);
}

void
ui_lock_button(UI_Ctx *ui, UI_Key key, V4f accent_color, B32 *is_locked)
{
  ui_set_next_layout_axis(ui, Axis2_X);
  ui_set_next_align_x(ui, UIAlign_Center);
  ui_set_next_sizing_x(ui, ui_grow());
  ui_parentf(ui, 0, S8Z())
  {
    V4f font_color = (*is_locked ? accent_color : color_white);
    ui_set_next_font_color(ui, font_color);
    ui_set_next_seed_key(ui, key);
    if(ui_buttonf(ui, S8("locked")).left_clicked)
    {
      *is_locked = !(*is_locked);
    }
  }
}

void
ui_date_time_range_picker(UI_Ctx *ui, Date_Input_State *state, Date_Time *start, Date_Time *end)
{
  V4f accent_color = color_green;
  
  ui_set_next_layout_axis(ui, Axis2_X);
  UI_Node *picker = ui_nodef(ui, UIFlags_DrawBackGround, S8Z());
  ui_parent(ui, picker)
    ui_seed_key(ui, ui_keyf("date_picker_%p_%p", start, end))
  {
    UI_Node *date_col = 0;
    UI_Node *time_col = 0;
    ui_sizing(ui, ui_grow(), ui_grow())
      ui_layout_axis(ui, Axis2_Y)
      ui_border_abs(ui, color_white, 1.0f, 0)
      ui_padding(ui, pad2_xy(5.0f, 5.0f))
      ui_align_y(ui, UIAlign_Center)
    {
      UI_Flags flags = UIFlags_DrawBorder;
      date_col = ui_nodef(ui, flags, S8Z());
      time_col = ui_nodef(ui, flags, S8Z());
    }
    
    // NOTE(erb): both dates
    ui_parent(ui, date_col)
    {
      I32 start_delta = ui_date_picker_with_arrows(ui, ui_keyf("start_date"), start->date);
      ui_lock_button(ui, ui_keyf("date_lock"), accent_color, &state->date_end_follows_start);
      I32 end_delta = ui_date_picker_with_arrows(ui, ui_keyf("end_date"), end->date);
      
      if(start_delta != 0 || end_delta != 0)
      {
        I32 start_days = cal_date_to_days(start->date) + start_delta;
        if(state->date_end_follows_start)
        {
          end->date = start->date = cal_date_from_days(start_days);
        }
        else
        {
          I32 end_days = cal_date_to_days(end->date) + end_delta;
          start->date = cal_date_from_days(Min(start_days, end_days));
          end->date   = cal_date_from_days(Max(start_days, end_days));
        }
        
      }
    }
    
    // NOTE(erb): calendar
    ui_set_next_sizing_x(ui, ui_grow());
    ui_set_next_border_abs(ui, color_white, 1.0f, 0);
    ui_set_next_layout_axis(ui, Axis2_Y);
    ui_parentf(ui, UIFlags_DrawBorder, S8Z())
    {
      ui_set_next_layout_axis(ui, Axis2_X);
      ui_sizing_x(ui, ui_grow())
        ui_parentf(ui, 0, S8Z())
        ui_align_x(ui, UIAlign_Center) ui_font_size(ui, 20.0f)
      {
        char day_letters[] = { 'm', 't', 'w', 't', 'f', 's', 's' };
        for(EachElement(char, d, day_letters))
        {
          String8 s = S8Struct(d, 1);
          ui_parentf(ui, 0, S8Z()) // NOTE(erb): container for alignment
          {
            ui_labelf(ui, s);
          }
        }
      }
      
      ui_set_next_sizing(ui, ui_grow(), ui_fixed(200));
      ui_set_next_padding(ui, pad2_equal(5.0f));
      UI_Node *calendar_container = ui_nodef(ui, 0, S8("calendar_container"));
      ui_parent(ui, calendar_container)
        ui_font_size(ui, 15.0f)
      {
        Range_V2f rect = {0};
        {
          UI_Node *cached = ui_node_from_key(ui, calendar_container->key);
          if(cached != 0)
          {
            rect = node_inner_rng_v2f(cached);
          }
        }
        V2f rect_size = rng_v2f_size(rect);
        
        V2i grid_size = v2i(7, 5);
        V2f cell_size = v2f_hada_div(rect_size, v2fi(grid_size));
        V2f cursor = rect.min;
        
        V4f color = v4f_lerp(picker->background.color1, accent_color, 0.2f);
        Gradient2 gradient = grad2(color, picker->background.color1, Orientation2_NWSE);
        
        V4f hover_color = v4f_lerp(picker->background.color1, accent_color, 0.5f);
        Gradient2 hover_gradient = grad2(hover_color, picker->background.color1, Orientation2_NWSE);
        
        V4f select_color = v4f_lerp(picker->background.color1, accent_color, 0.7f);
        Gradient2 select_gradient = grad2(select_color, picker->background.color1, Orientation2_NWSE);
        
        Gradient2 edge_gradient = grad2(accent_color, picker->background.color1, Orientation2_NWSE);
        
        U32 monday_weekday = 1;
        I32 start_days = cal_date_to_days(start->date);
        I32 end_days   = cal_date_to_days(end->date);
        I32 calendar_starting_day = (start_days - weekday_difference(weekday_from_days(start_days), monday_weekday));
        for(U32Each(y, (U32)grid_size.y))
        {
          for(U32Each(x, (U32)grid_size.x))
          {
            I32 cell_day = (calendar_starting_day + y*grid_size.x + x);
            Calendar_Date cell_date = cal_date_from_days(cell_day);
            
            ui_set_next_rect(ui, rng_v2f_point_add(cursor, cell_size));
            ui_set_next_background(ui, gradient);
            ui_set_next_border_abs(ui, v4ff(0), 0, 0);
            UI_Node *day_node = ui_nodef(ui, 
                                         UIFlags_DrawBackGround|
                                         UIFlags_Floating|
                                         UIFlags_DrawBorder|
                                         UIFlags_Comm, 
                                         S8("day_cell_%u_%u"), x, y);
            UI_Comm comm = ui_comm_from_node(ui, day_node);
            if(cell_day == start_days || cell_day == end_days)
            {
              day_node->background = edge_gradient;
              day_node->border_thickness = 1.0f;
              day_node->border_color = color_white;
            }
            else if(cell_day >= start_days && cell_day <= end_days)
            {
              day_node->background = select_gradient;
            }
            if(comm.hovered)
            {
              day_node->background = hover_gradient;
            }
            
            if(comm.left_clicked)
            {
              if(state->date_end_follows_start)
              {
                start->date = end->date = cell_date;
              }
              else
              {
                if(cell_day <= start_days)
                {
                  start->date = cell_date;
                }
                if((cell_day >= start_days && cell_day <= end_days) || 
                   cell_day >= end_days)
                {
                  end->date = cell_date;
                }
              }
            }
            
            ui_parent(ui, day_node)
            {
              ui_labelf(ui, S8("%u"), cell_date.day);
            }
            
            cursor.x += cell_size.x;
          }
          cursor.x = rect.min.x;
          cursor.y += cell_size.y;
        }
      }
    }
    
    // NOTE(erb): times editor
    ui_parent(ui, time_col)
    {
      I32 start_delta = ui_time_picker_with_arrows(ui, ui_keyf("start_time"), start->time);
      ui_lock_button(ui, ui_keyf("time_lock"), accent_color, &state->time_end_follows_start);
      I32 end_delta = ui_time_picker_with_arrows(ui, ui_keyf("end_time"), end->time);
      
      if(start_delta != 0 || end_delta != 0)
      {
        Time_Duration start_op = {0};
        start_op.value = start_delta;
        start_op.kind = TimeDurationKind_Minute;
        *start = date_time_add_duration(*start, start_op);
        if(state->time_end_follows_start)
        {
          *end = *start;
        }
        else
        {
          Time_Duration end_op = {0};
          end_op.value = end_delta;
          end_op.kind = TimeDurationKind_Minute;
          *end = date_time_add_duration(*end, end_op);
          
          if(date_time_greater_equal(start, end))
          {
            Swap(Date_Time, *start, *end);
          }
        }
      }
    }
    
    // NOTE(erb): time series
    ui_set_next_debug_str(ui, S8("time series"));
    ui_set_next_sizing(ui, ui_grow(), ui_grow());
    ui_set_next_layout_axis(ui, Axis2_Y);
    ui_set_next_border_abs(ui, color_white, 1.0f, 0);
    ui_set_next_padding(ui, pad2_xy(5.0f, 5.0f));
    ui_set_next_align_y(ui, UIAlign_Center);
    UI_Node *time_line_container = ui_nodef(ui, UIFlags_DrawBorder, S8("time_line"));
    ui_parent(ui, time_line_container)
    {
      Range_V2f rect = {0};
      {
        UI_Node *cached = ui_node_from_key(ui, time_line_container->key);
        if(cached != 0)
        {
          rect = node_inner_rng_v2f(cached);
          V2f rect_size = rng_v2f_size(rect);
          
          Gradient2 base_gradient = grad2_single(picker->background.color0);
          V4f highlight_color = v4f_lerp(picker->background.color0, accent_color, 0.1f);
          Gradient2 highlight_gradient = grad2(highlight_color, picker->background.color0, Orientation2_NWSE);
          
          Clock_Time time_start = {0};
          time_start.hour = 7;
          
          U32 tick_count = 8;
          V2f cursor = rect.min;
          V2f tick_size = v2f(rect_size.x, rect_size.y/tick_count);
          ui_font_size(ui, 15.0f)
            for(U32Each(y, tick_count))
          {
            U32 tick_hours = (time_start.hour + y);
            
            Range_V2f rng = rng_v2f_point_add(cursor, tick_size);
            ui_set_next_debug_str(ui, S8("tick"));
            ui_set_next_rect(ui, rng);
            ui_set_next_background(ui, ((y%2)==0 ? base_gradient : highlight_gradient));
            UI_Node *tick_node = ui_nodef(ui, UIFlags_Floating|UIFlags_DrawBackGround, S8Z());
            ui_parent(ui, tick_node)
            {
              ui_labelf(ui, S8("%02uh"), tick_hours);
            }
            if(start->time.hour == tick_hours)
            {
              Range_V2f line = rng;
              line.min.y += (start->time.minute/60.0f) * rng_v2f_size(rng).y;
              line.max.y = line.min.y + 1.0f;
              ui_rect_color(ui, line, color_white);
            }
            cursor.y += tick_size.y;
          }
        }
      }
    }
  }
}

void
ui_date_time_line_input(UI_Ctx *ui, Date_Input_State *date_input_state, Date_Time *start, Date_Time *end)
{
  Date_Time dt_now = os_get_local_date_time();
  
  ui_set_next_layout_axis(ui, Axis2_Y);
  ui_parentf(ui, 0, S8Z())
  {
    UI_Key input_key = ui_keyf("date_time_input_key_%p_%p", start, end);
    UI_Line_Input_State *input = ui_line_input(ui, input_key);
    
    if(input != 0 && ui_line_input_changed(ui, input))
    {
      Scratch scratch = scratch_begin(0, 0);
      ZeroStruct(&date_input_state->last_error);
      Token_List tokens = token_list_from_str(scratch.arena, input->writer.str, &date_input_state->last_error);
      
      if(date_input_state->last_error.value == 0)
      {
        Token_Iterator token_it = {0};
        token_it.current = tokens.first;
        update_parse_date_time_range_from_tokens(&token_it, dt_now.date, start, end, &date_input_state->last_error);
      }
      
      scratch_end(&scratch);
    }
  }
}

UI_Node *
ui_cell_lister_begin(UI_Ctx *ui, UI_Key key)
{
  ui_set_next_sizing(ui, ui_grow(), ui_grow());
  ui_set_next_layout_axis(ui, Axis2_Y);
  ui_set_next_padding(ui, pad2_equal(2));
  ui_set_next_child_gap(ui, 0);
  ui_set_next_background_color(ui, v4f(0, 0, 0.5f, 1));
  ui_set_next_border_abs(ui, v4f(0.7f, 0.7f, 0.9f, 1), 2, 0);
  ui_set_next_debug_str(ui, S8("lister_container"));
  UI_Node *lister = ui_nodef(ui, 
                             UIFlags_DrawBackGround|
                             UIFlags_DrawBorder, 
                             S8Z());
  ui_push_parent(ui, lister);
  ui_push_seed_key(ui, key);
  return(lister);
}

void
ui_cell_lister_end(UI_Ctx *ui)
{
  ui_pop_seed_key(ui);
  ui_pop_parent(ui);
}

UI_Line_Input_State *
ui_cell_lister_input(UI_Ctx *ui, String8 label)
{
  UI_Line_Input_State *input = 0;
  
  ui_set_next_child_gap(ui, 0);
  ui_set_next_sizing_x(ui, ui_grow());
  ui_parentf(ui, 0, S8Z())
  {
    if(label.size > 0)
    {
      ui_set_next_padding(ui, pad2_xy(4.0f, 0));
      ui_set_next_background_color(ui, v4f(0, 0, 0.2f, 1));
      ui_set_next_sizing_y(ui, ui_grow());
      ui_set_next_align_y(ui, UIAlign_Center);
      ui_parentf(ui, UIFlags_DrawBackGround, S8Z())
      {
        ui_set_next_font_color(ui, with_alpha(v4ff(0.8f), 1.0f));
        ui_set_next_font_size(ui, 24.0f);
        ui_labelf(ui, label);
      }
    }
    
    ui_set_next_border_color(ui, color_blank);
    ui_set_next_font_color(ui, color_white);
    ui_set_next_background_color(ui, color_blank);
    UI_Key input_key = ui_key_seed_with(ui_keyf("input_key"), UIGetStackValue(ui, seed_key));
    input = ui_line_input(ui, input_key);
  }
  return(input);
}

void
ui_cell_lister_content_begin(UI_Ctx *ui)
{
  ui_set_next_layout_axis(ui, Axis2_X);
  ui_set_next_sizing(ui, ui_grow(), ui_grow());
  ui_set_next_child_gap(ui, 0);
  ui_push_parent(ui, ui_nodef(ui, 0, S8Z()));
}

void
ui_spacer(UI_Ctx *ui, F32 pixels)
{
  Assert(ui->current);
  Assert(pixels >= 0);
  Axis2 axis = ui->current->layout_axis;
  
  UI_Sizing_Value x = (axis == Axis2_X) ? ui_fixed(pixels) : ui_grow();
  UI_Sizing_Value y = (axis == Axis2_Y) ? ui_fixed(pixels) : ui_grow();
  
  ui_set_next_sizing(ui, x, y);
  ui_parentf(ui, 0, S8Z());
}

void
ui_cell_lister_scroll(UI_Ctx *ui, UI_Key scroll_key, F32 view_size, F32 list_total_size, 
                      F32 list_item_height, I32 content_scroll_clicks_y)
{
  // NOTE(erb): scroll bar
  ui_set_next_layout_axis(ui, Axis2_Y);
  ui_set_next_sizing(ui, ui_fit(), ui_grow());
  ui_set_next_border_abs(ui, v4f(0.7f, 0.7f, 0.9f, 1), 1, 0);
  ui_set_next_padding(ui, pad2_equal(1.0f));
  ui_set_next_child_gap(ui, 0);
  UI_Node *container = ui_nodef(ui, UIFlags_DrawBorder, S8("scroll_bar_outer"));
  ui_parent(ui, container)
  {
    UI_Node *cached_container = ui_node_from_key(ui, container->key);
    if(cached_container != 0)
    {
      Range_V2f container_rng = node_inner_rng_v2f(cached_container);
      F32 scroll_container_height = (container_rng.max.y - container_rng.min.y);
      
      F32 t = (list_total_size > 0) ? (view_size/list_total_size) : 0.0f;
      F32 scroll_height = lerp(0, scroll_container_height, Cap01(t));
      
      F32 travel = (scroll_container_height - scroll_height);
      F32 max_scroll = Cap0(list_total_size - view_size);
      
      ui_set_next_background_color(ui, v4f(0, 0, 0.2f, 1));
      ui_set_next_sizing(ui, ui_fixed(20), ui_fixed(scroll_height));
      UI_Node *scroll = ui_node_make(ui, 
                                     UIFlags_DrawBackGround|
                                     UIFlags_Comm|
                                     UIFlags_DontSeedKey, 
                                     scroll_key, S8Z());
      
      UI_Comm out = ui_comm_from_node(ui, scroll);
      if(out.node != 0)
      {
        scroll->scroll_offset = out.node->scroll_offset;
      }
      
      // NOTE(erb): mouse 
      if(out.left_dragging && travel > 0)
      {
        F32 content_px_per_scroll_px = (max_scroll / travel);
        scroll->scroll_offset += ui->input.cursor_delta.y * content_px_per_scroll_px;
      }
      
      // NOTE(erb): pos delta
      {
        I32 pos_delta = 0;
        if(key_pressed(ui, GFXKey_Down))
        {
          pos_delta += 1;
        }
        if(key_pressed(ui, GFXKey_Up))
        {
          pos_delta -= 1;
        }
        if(out.hovered)
        {
          pos_delta += ui->input.scroll_clicks.y;
        }
        pos_delta += content_scroll_clicks_y;
        scroll->scroll_offset += (pos_delta * list_item_height);
      }
      
      // NOTE(erb): positioning
      scroll->scroll_offset = Cap(scroll->scroll_offset, 0, max_scroll);
      
      F32 thumb_t = (max_scroll > 0) ? scroll->scroll_offset/max_scroll : 0;
      F32 spacer_size = Cap(thumb_t * travel, 0, scroll_container_height);
      
      ui_spacer(ui, spacer_size);
      ui_parent(ui, scroll);
    }
  }
}

void
ui_cell_lister_content_end(UI_Ctx *ui)
{
  // NOTE(erb): end lister
  ui_pop_parent(ui);
}

typedef struct UI_Lister_Items_Result UI_Lister_Items_Result;
struct UI_Lister_Items_Result
{
  F32 view_height;
  F32 list_item_height;
  F32 list_total_height;
  
  I32 scroll_clicks_y;
  UI_Cell *selected;
};

B32
ui_cell(UI_Ctx *ui, UI_Cell *cell, UI_Key key)
{
  AssertImplies(cell->flags & UICellFlags_DrawExpandArrowClose,
                !(cell->flags & UICellFlags_DrawExpandArrowOpen));
  AssertImplies(cell->flags & UICellFlags_DrawExpandArrowOpen,
                !(cell->flags & UICellFlags_DrawExpandArrowClose));
  
  Scratch scratch = scratch_begin(0, 0);
  
  B32 more_space = (cell->flags & UICellFlags_MoreSpace);
  F32 padding             = (more_space ?  7.0f :  4.0f);
  F32 label_font_size     = (more_space ? 28.0f : 24.0f);
  F32 sub_label_font_size = (more_space ? 21.0f : 15.0f);
  
  UI_Node *node = 0;
  ui_background_color(ui, color_blank)
  {
    ui_set_next_border_abs(ui, v4f(0.7f, 0.7f, 0.9f, 1), 1.5f, 0);
    ui_set_next_padding(ui, pad2_equal(padding));
    ui_set_next_sizing_x(ui, ui_grow());
    ui_set_next_layout_axis(ui, Axis2_X);
    ui_set_next_align_y(ui, UIAlign_Center);
    ui_set_next_child_gap(ui, 5.0f);
    UI_Flags flags = (UIFlags_Comm|
                      UIFlags_DrawBackGround|
                      UIFlags_DrawBorder|
                      UIFlags_DontSeedKey);
    if(cell->flags & UICellFlags_Clickable)
    {
      flags |= UIFlags_HoverEffect;
    }
    node = ui_node_make(ui, flags, key, S8Z());
  }
  
  ui_parent(ui, node)
  {
    // NOTE(erb): arrow
    if((cell->flags & UICellFlags_DrawExpandArrowOpen) || 
       (cell->flags & UICellFlags_DrawExpandArrowClose))
    {
      ui_set_next_font_color(ui, color_white);
      ui_set_next_font_size(ui, label_font_size);
      ui_labelf(ui, (cell->flags & UICellFlags_DrawExpandArrowOpen) ? S8("\\/") : S8(">"));
    }
    
    B32 has_label_content = (cell->label.size > 0
                             || ((cell->flags & UICellFlags_FancyStrLabel) && cell->label_fancy_str.first != 0) 
                             || ((cell->flags & UICellFlags_DrawSubLabel) && cell->sub_label.size > 0));
    if(has_label_content)
    {
      ui_set_next_sizing_x(ui, ui_grow());
      ui_set_next_layout_axis(ui, Axis2_Y);
      ui_parentf(ui, 0, S8Z())
      {
        // NOTE(erb): fancy str
        if(cell->flags & UICellFlags_FancyStrLabel)
        {
          ui_fancy_str(ui, cell->label_fancy_str);
        }
        else // NOTE(erb): label
        {
          ui_set_next_font_color(ui, color_white);
          ui_set_next_font_size(ui, label_font_size);
          ui_labelf(ui, cell->label);
        }
        
        // NOTE(erb): sub label
        if(cell->flags & UICellFlags_DrawSubLabel)
        {
          ui_set_next_font_color(ui, with_alpha(v4ff(0.7f), 1.0f));
          ui_set_next_font_size(ui, sub_label_font_size);
          ui_labelf(ui, cell->sub_label);
        }
      }
    }
    
    // NOTE(erb): key maps
    if(cell->flags & UICellFlags_DrawKeyMaps)
    {
      Assert(cell->key_map != 0);
      
      ui_background_color(ui, with_alpha(v4ff(0.7f), 1))
        ui_border_abs(ui, v4f(0.5f, 0.5f, 0.7f, 1), 1.0f, 2)
        for(U32Each(combo_idx, cell->key_map->count))
      {
        Scratch combo_scratch = arena_get_scratch(scratch.arena);
        Key_Combo *combo = (cell->key_map->combos + combo_idx);
        
        ui_parentf(ui, 
                   UIFlags_DrawBackGround
                   |UIFlags_DrawBorder
                   |UIFlags_DrawRounded, S8Z())
        {
          if(combo->mods != 0)
          {
            ui_labelf(ui, S8("%S + %S"), 
                      gfx_key_str8(combo->key), 
                      gfx_modifiers_str8(combo_scratch.arena, combo->mods));
          }
          else
          {
            ui_labelf(ui, gfx_key_str8(combo->key));
          }
        }
        
        scratch_end(&combo_scratch);
      }
    }
    
    // NOTE(erb): line input
    if(cell->flags & UICellFlags_LineInput)
    {
      ui_set_next_border_abs(ui, color_blank, 0, 0);
      ui_line_input_update_state(ui, cell->line_input);
    }
    
    // NOTE(erb): range input
    if(cell->flags & UICellFlags_RangeInput)
    {
      ui_labelf(ui, S8("%.1f"), *cell->f32_out);
      
      ui_set_next_sizing_x(ui, ui_grow());
      ui_rng_f32_slider(ui, cell->f32_out, cell->f32_rng);
    }
    
    // NOTE(erb): boolean input
    if(cell->flags & UICellFlags_BoolInput)
    {
      ui_set_next_sizing_y(ui, ui_grow());
      ui_toggle_switch(ui, cell->b32_out);
    }
  }
  
  B32 selected = ((cell->flags & UICellFlags_Clickable) && 
                  ui_comm_from_node(ui, node).left_clicked);
  scratch_end(&scratch);
  return(selected);
}

UI_Lister_Items_Result
ui_cell_lister_items(UI_Ctx *ui, UI_Cell *cell_arr, U32 cell_count, String8 filter_str, F32 scroll_offset, B32 should_shrink_y)
{
  Scratch scratch = scratch_begin(0, 0);
  UI_Lister_Items_Result result = {0};
  
  ui_set_next_child_gap(ui, 0);
  ui_set_next_layout_axis(ui, Axis2_Y);
  ui_set_next_sizing_x(ui, ui_grow());
  if(!should_shrink_y)
  {
    ui_set_next_sizing_y(ui, ui_grow());
  }
  ui_set_next_background_color(ui, color_blank);
  UI_Node *container = ui_nodef(ui, UIFlags_Comm|
                                UIFlags_DrawBackGround|
                                UIFlags_ClipRectangle,
                                S8("cell_items_container"));
  ui_parent(ui, container)
  {
    UI_Flags items_wrapper_flags = 0;
    Range_V2f container_rng = {0};
    
    UI_Comm container_comm = ui_comm_from_node(ui, container);
    if(container_comm.node != 0)
    {
      container_rng = node_rng_v2f(container_comm.node);
      if(!should_shrink_y)
      {
        items_wrapper_flags |= UIFlags_Floating;
        ui_set_next_floating_pos(ui, container_rng.min);
      }
    }
    if(container_comm.hovered)
    {
      // NOTE(erb): negative
      result.scroll_clicks_y = -ui->input.scroll_clicks.y;
    }
    
    ui_set_next_child_gap(ui, 0);
    ui_set_next_layout_axis(ui, Axis2_Y);
    ui_set_next_sizing(ui, ui_grow(), ui_fit());
    ui_set_next_padding(ui, pad2_equal(0));
    ui_parentf(ui, items_wrapper_flags, S8Z())
    {
      UI_Key first_item_key = ui_keyf("first_display_item");
      UI_Node *first_cached_item = ui_node_from_key(ui, first_item_key);
      F32 item_height = (first_cached_item != 0) ? rng_v2f_size(node_rng_v2f(first_cached_item)).y : 0;
      F32 container_height = (container_rng.max.y - container_rng.min.y);
      result.view_height = container_height;
      result.list_item_height = item_height;
      result.list_total_height = item_height * cell_count;
      
      Range_F32 pixel_range = rng_f32_point_add(scroll_offset, container_height);
      
      Range_U64 index_range = rng_u64(0, cell_count-1);
      if(item_height > 0)
      {
        index_range.min = (U64)(pixel_range.min / item_height);
        index_range.max = CapTop((U64)(pixel_range.max / item_height + 1), cell_count-1);
      }
      
      F32 space_after = (F32)((cell_count-1) - index_range.max) * item_height;
      
#if 0
      F32 space_before = index_range.min * item_height;
      ui_spacer(ui, space_before);
#endif
      
      B32 is_first = true;
      for(U64EachInRange(idx, index_range))
      {
        // NOTE(erb): actual items
        UI_Cell *cell = (cell_arr + idx);
        B32 show = (filter_str.size == 0 || str8_find(cell->filter_needle, filter_str, 0).exists);
        if(show)
        {
          UI_Key key = (is_first ? first_item_key : ui_keyf("cell_item_lister_%p", cell));
          B32 cell_selected = ui_cell(ui, cell, key);
          if(cell_selected)
          {
            result.selected = cell;
          }
          is_first = false;
        }
      }
      ui_spacer(ui, space_after);
    }
    
  }
  scratch_end(&scratch);
  return(result);
}

UI_Lister_Result
ui_cell_lister(UI_Ctx *ui, String8 search_label, UI_Key key, UI_Cell *cell_arr, U32 cell_count)
{
  UI_Lister_Result result = {0};
  
  UI_Node *lister = ui_cell_lister_begin(ui, key);
  {
    UI_Line_Input_State *input = ui_cell_lister_input(ui, search_label);
    result.input_str = input->writer.str;
    
    UI_Key scroll_key = ui_keyf("scroll_movable");
    UI_Node *scroll = ui_node_from_key(ui, scroll_key);
    F32 scroll_offset = (scroll != 0) ? scroll->scroll_offset : 0.0f;
    
    ui_cell_lister_content_begin(ui);
    UI_Lister_Items_Result items_result = ui_cell_lister_items(ui, cell_arr, cell_count, 
                                                               result.input_str, scroll_offset, 
                                                               false);
    result.selected = items_result.selected;
    ui_cell_lister_scroll(ui, scroll_key, 
                          items_result.view_height, 
                          items_result.list_total_height, 
                          items_result.list_item_height, 
                          items_result.scroll_clicks_y);
    ui_cell_lister_content_end(ui);
  }
  ui_cell_lister_end(ui);
  
  result.lister = lister;
  return(result);
}

typedef struct UI_Lister_Popup_Result UI_Lister_Popup_Result;
struct UI_Lister_Popup_Result
{
  B32 pressed_out;
  UI_Lister_Result lister;
};

UI_Lister_Popup_Result
ui_cell_lister_floating_popup(UI_Ctx *ui, V2f top_left, UI_Key key, UI_Cell *cell_arr, U32 cell_count, String8 search_label)
{
  ui_set_next_sizing_x(ui, ui_fixed(200));
  ui_set_next_floating_pos(ui, top_left);
  UI_Node *popup = ui_node_make(ui, UIFlags_Comm|UIFlags_Floating, key, S8Z());
  
  UI_Lister_Popup_Result result = {0};
  ui_popup(ui, popup) 
    ui_seed_key(ui, popup->key)
    ui_border_abs(ui, v4ff(0), 0, 0)
  {
    UI_Key lister_key = ui_keyf("popup_lister_%llu", ui_hash_from_key(key));
    
    // NOTE(erb): Hack to achieve the desired event bubling. Because the lister collects the events (selections)
    // on each cell, we collect the events for the popup before, so that at the end of the first interaction frame, 
    // the correct 'active' node is a cell item and NOT the popup.
    // 
    // How it would work if we collected after: 
    //  ``
    //  result.lister = ui_cell_lister(ui, S8Z(), ui_keyf("tab_panel_popup"), cell_arr, ViewKind_COUNT);
    //  result.pressed_out = (result.lister.selected == 0) && ui_comm_from_node(ui, popup).pressed_out;
    //  ``
    //  1. click on cell in the UI
    //  2. ui_comm(cell) -> cell becomes hot -> cell becomes active
    //  3. ui_comm(popup) -> popup becomes hot -> popup becomes active
    //  4. release on cell in the UI
    //  5. ui_comm(cell) -> cell becomes hot -> not active so nothing happens -> result.lister.selected will always be 0
    //  6. ui_comm(popup) -> popup becomes hot -> popup is active from last frame -> produces pressed_out
    //
    // If we call ui_comm(popup) before then it works out as intented. 
    
    B32 pressed_out = ui_comm_from_node(ui, popup).pressed_out;
    result.lister = ui_cell_lister(ui, S8Z(), lister_key, cell_arr, ViewKind_COUNT);
    result.pressed_out = (pressed_out && result.lister.selected == 0);
  }
  return(result);
}
