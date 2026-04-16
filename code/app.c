#include "core.c"
#include "ui.c"

typedef struct App_Links
{
  Arena permanent_arena;
  Arena transient_arena;
  UI_Ctx ui_ctx;
  
  Arena input_arena;
  String_Builder input_builder;
  
  Debug_Info debug_info;
  f64 debug_update_timer;
  
  Window_ID main_window;
} App_Links;

void ui_push_mouse_cursor_helper(UI_Ctx *ctx)
{
  V2f cursor = ctx->input->cursor_reverse_y.end;
  Screen_Rect_f32 rect = rect_center(cursor, v2ff(40));
  f32 corner_radius = rect_size(rect).x/2.0f;
  
  UI_Draw_Cmd *cmd = sll_push_allocate(&ctx->arena, &ctx->defered_draw, UI_Draw_Cmd);
  
  cmd->kind = UIDrawKind_RectLines;
  cmd->rect = rect;
  cmd->color = color_white;
  cmd->corner_radius = corner_radius;
  cmd->border_thickness = 3.0f;
}

void debug_update_and_ui(App_Links *app, Input_State *input_state, Debug_Counters frame_debug_counters)
{
  Debug_Info *info = &app->debug_info;
  
  // NOTE(erb): update debug counters
  {
    app->debug_update_timer -= high_freq_time_from_ns(frame_debug_counters.frame_time_ns).s;
    
    if (app->debug_update_timer <= 0.0f)
    {
      info->min.frame_time_ns = 100000000000000;
      info->min.cycles = 100000000000000;
      info->min.page_reclaims = 100000000000000;
      info->min.page_faults = 100000000000000;
      info->min.swaps = 100000000000000;
      info->min.fs_input = 100000000000000;
      info->min.fs_output = 100000000000000;
      
      info->max.frame_time_ns = 0;
      info->max.cycles = 0;
      info->max.page_reclaims = 0;
      info->max.page_faults = 0;
      info->max.swaps = 0;
      info->max.fs_input = 0;
      info->max.fs_output = 0;
      
      app->debug_update_timer = 0.1f;
    }
    
    info->past = frame_debug_counters;
    
    info->min.frame_time_ns = min(info->min.frame_time_ns, frame_debug_counters.frame_time_ns);
    info->min.cycles = min(info->min.cycles, frame_debug_counters.cycles);
    info->min.page_reclaims = min(info->min.page_reclaims, frame_debug_counters.page_reclaims);
    info->min.page_faults = min(info->min.page_faults, frame_debug_counters.page_faults);
    info->min.swaps = min(info->min.swaps, frame_debug_counters.swaps);
    info->min.fs_input = min(info->min.fs_input, frame_debug_counters.fs_input);
    info->min.fs_output = min(info->min.fs_output, frame_debug_counters.fs_output);
    
    info->max.frame_time_ns = max(info->max.frame_time_ns, frame_debug_counters.frame_time_ns);
    info->max.cycles = max(info->max.cycles, frame_debug_counters.cycles);
    info->max.page_reclaims = max(info->max.page_reclaims, frame_debug_counters.page_reclaims);
    info->max.page_faults = max(info->max.page_faults, frame_debug_counters.page_faults);
    info->max.swaps = max(info->max.swaps, frame_debug_counters.swaps);
    info->max.fs_input = max(info->max.fs_input, frame_debug_counters.fs_input);
    info->max.fs_output = max(info->max.fs_output, frame_debug_counters.fs_output);
  }
  
  UI_Ctx *ui = &app->ui_ctx;
  Arena *temp_arena = &app->transient_arena;
  
  
  // NOTE(erb): draw table look thing
  V4f background_color = v4f(0.3f, 0.3f, 0.3f, 0.5);
  
  High_Freq_Time past_time = high_freq_time_from_ns(info->past.frame_time_ns);
  High_Freq_Time min_time = high_freq_time_from_ns(info->min.frame_time_ns);
  High_Freq_Time max_time = high_freq_time_from_ns(info->max.frame_time_ns);
  
  ui_set_next_layout_axis(ui, Axis2_Y);
  ui_sizing_x(ui, ui_fit())
    ui_background_color(ui, background_color) 
    ui_parent_flagged(ui, UIFlags_DrawBackGround)
  {
    ui_text(ui, str_format(temp_arena, "Frame Time: %.6fms (%.6fms - %.6fms)", 
                           past_time.ms,
                           min_time.ms,
                           max_time.ms));
    
    ui_text(ui, str_format(temp_arena, "FPS: %.1f (%.1f - %.1f)", 
                           1.0f/past_time.s,
                           1.0f/max_time.s,
                           1.0f/min_time.s));
    
    ui_text(ui, str_format(temp_arena, "Cycles: %lluk (%lluk - %lluk)", 
                           (u64)(info->past.cycles*0.001),
                           (u64)(info->min.cycles*0.001),
                           (u64)(info->max.cycles*0.001)));
    
    ui_text(ui, str_format(temp_arena, "Page Reclaims: %llu (%llu - %llu)", 
                           info->past.page_reclaims,
                           info->min.page_reclaims,
                           info->max.page_reclaims));
    
    ui_text(ui, str_format(temp_arena, "Page Faults: %llu (%llu - %llu)", 
                           info->past.page_faults,
                           info->min.page_faults,
                           info->max.page_faults));
    
    ui_text(ui, str_format(temp_arena, "Swaps: %llu (%llu - %llu)", 
                           info->past.swaps,
                           info->min.swaps,
                           info->max.swaps));
    
    ui_text(ui, str_format(temp_arena, "FS Input: %llu (%llu - %llu)", 
                           info->past.fs_input,
                           info->min.fs_input,
                           info->max.fs_input));
    
    ui_text(ui, str_format(temp_arena, "FS Output: %llu (%llu - %llu)", 
                           info->past.fs_output,
                           info->min.fs_output,
                           info->max.fs_output));
    
    switch (frame_debug_counters.state)
    {
      case DebugState_None: {} break;
      
      case DebugState_Recording:
      {
        ui_font_color(ui, v4f(0.53f, 0.12f, 0.18f, 1)) 
        {
          ui_text(ui, S("Recording"));
        }
      } break;
      
      case DebugState_Playing: 
      {
        ui_push_mouse_cursor_helper(ui);
        ui_font_color(ui, v4f(0.1f, 0.69f, 0.16f, 1)) 
        {
          ui_text(ui, S("Playing"));
        }
      } break;
    }
    
    b32 has_down_event = false;
    for (u32 key_idx = 0;
         key_idx < array_size(input_state->keys);
         key_idx++)
    {
      Input_Button *key = (input_state->keys + key_idx);
      has_down_event = key->ended_down;
      if (key->ended_down)
      {
        break;
      }
    }
    
    if (has_down_event)
    {
      ui_set_next_layout_axis(ui, Axis2_X);
      ui_parent_zero(ui)
      {
        for (u32 key_idx = 0;
             key_idx < array_size(input_state->keys);
             key_idx++)
        {
          Input_Button *key = (input_state->keys + key_idx);
          if (key->ended_down)
          {
            String str = push_cstr_copy(temp_arena, debug_key_code_cstr((Key_Code)key_idx));
            ui_border_thickness(ui, 1.0f) ui_parent_flagged(ui, UIFlags_DrawBorder)
            {
              ui_text(ui, str_from(str, S("KeyCode_").length));
            }
          }
        }
      }
    }
    
    
#define ui_mod_label(label) \
if (input_state->modifiers & ModifierFlags_##label) \
ui_border_thickness(ui, 1.0f) ui_parent_flagged(ui, UIFlags_DrawBorder) ui_text(ui, S(#label))
    
    if (input_state->modifiers != ModifierFlags_None) 
    {
      ui_set_next_layout_axis(ui, Axis2_X);
      ui_parent_zero(ui)
      {
        ui_mod_label(Shift);
        ui_mod_label(Control);
        ui_mod_label(Alt);
        ui_mod_label(Super);
        ui_mod_label(Caps);
        ui_mod_label(NumLock);
      }
    }
  }
}

void render_ui_cmds(Render_Data *renderer, UI_Draw_Cmd_List draw_cmds)
{
  
  for (UI_Draw_Cmd *cmd = draw_cmds.first;
       cmd;
       cmd = cmd->next)
  {
    switch (cmd->kind)
    {
      case UIDrawKind_Text:
      {
        render_text(renderer, 
                    cmd->font, 
                    cmd->content, 
                    cmd->rect.top_left, 
                    cmd->font_size, 
                    cmd->spacing, 
                    cmd->color);
      } break;
      
      case UIDrawKind_Rect:
      {
        if (cmd->corner_radius == 0.0f)
        {
          render_rect(renderer,
                      cmd->rect,
                      cmd->color);
        }
        else
        {
          render_rect_rounded(renderer,
                              cmd->rect,
                              cmd->color,
                              cmd->corner_radius);
          
        }
      } break;
      
      case UIDrawKind_RectLines:
      {
        if (cmd->corner_radius == 0.0f)
        {
          render_rect_lines(renderer,
                            cmd->rect,
                            cmd->color,
                            cmd->border_thickness);
        }
        else
        {
          render_rect_lines_rounded(renderer,
                                    cmd->rect,
                                    cmd->color,
                                    cmd->corner_radius,
                                    cmd->border_thickness);
        }
      } break;
    }
  }
}

UPDATE_AND_RENDER(update_and_render)
{
  supress_unused(exec_directory);
  g_platform = memory->platform;
  
  App_Links *app = (App_Links *)memory->permanent_memory;
  
  if (!memory->is_initialized)
  {
    memory->is_initialized = true;
    arena_init(&app->permanent_arena, 
               memory->permanent_memory + sizeof(App_Links), 
               memory->permanent_memory_size - sizeof(App_Links));
    arena_init(&app->transient_arena, 
               memory->transient_memory, 
               memory->transient_memory_size);
    
    app->main_window = g_platform.make_window(S("hello world"), v2i(600, 800));
    
    ui_init(&app->ui_ctx, &app->permanent_arena);
    app->input_arena = push_sub_arena(&app->permanent_arena, kb(8));
    str_build_str(&app->input_arena, &app->input_builder, S("test text"));
  }
  
  Font_Face face = {0};
  face.data = g_platform.get_default_font(app->main_window);
  face.height = 20;
  face.spacing = 1;
  
  Input_State *input = g_platform.get_window_events(app->main_window);
  
  App_Update_Result result = {0};
  
  V2f screen_size = v2fi(input->screen_space_size.end);
  result.should_kill = (input->should_kill || key_released(input, KeyCode_Escape));
  {
    UI_Ctx *ui = &app->ui_ctx;
    
    ui_begin_frame(ui, app->main_window, face, input, rect_from_size(screen_size));
    {
      debug_update_and_ui(app, input, frame_debug_counters);
      ui_input(ui, &app->input_arena, &app->input_builder, input);
      
      if (ui_button(ui, S("hello")).clicked)
      {
        Window_ID window = g_platform.make_window(S("hello"), v2i(30, 60));
      }
    }
    
    Render_Data *renderer = g_platform.get_renderer_buffer(app->main_window);
    render_ui_cmds(renderer, ui_calculate_and_draw(ui, &app->transient_arena));
    ui_end_frame(ui);
  }
  
  arena_release(&app->transient_arena);
  return result;
}
