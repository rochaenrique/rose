#include <math.h>

// headers
#include "base/base.h"
#include "img/img.h"
#include "gfx/gfx.h"
#include "fnt/fnt.h"
#include "gfx/q2d/q2d.h"

// implementation
#include "base/base.c"
#include "img/img.c"
#include "gfx/gfx.c"
#include "fnt/fnt.c"
#include "gfx/q2d/q2d.c"

V2f
measure_text(Q2D_State *render_state, String8 str, Q2D_Font *font, 
             F32 blank_spacing, F32 char_spacing)
{
  F32 width = q2d_render_string_baseline(render_state, str, v2ff(0), font, 
                                         blank_spacing, char_spacing, v4ff(0), false);
  V2f result = v2f(width, fnt_get_vadvance_px(&font->data));
  return(result);
}

GFX_Event_List 
filter_window_events(GFX_Event_List *events, GFX_Window window)
{
  GFX_Event_List result = {0};
  for(EachNode(GFX_Event, event, events->first))
  {
    if(event->window == window)
    {
      DLLQueueAppend(result.first, result.last, event);
      result.count++;
      DLLRemove(events->first, events->last, event);
    }
  }
  return(result);
}

typedef struct Event_Timed Event_Timed;
struct Event_Timed
{
  GFX_Event event;
  F32 timer;
  Event_Timed *next;
};

#define EVENT_CACHE_COUNT 1024

typedef struct Wdw Wdw;
struct Wdw
{
  GFX_Window handle;
  Arena event_arena;
  
  Event_Timed *first_event;
  Event_Timed *free_event;
};

void
destroy_window(Wdw *windows, U32 *window_count, U32 idx)
{
  Assert(idx < *window_count);
  Wdw wdw = windows[idx];
  
  (*window_count)--;
  Swap(Wdw, windows[*window_count], windows[idx]);
  ZeroStruct((windows + (*window_count)));
  
  gfx_window_destroy(wdw.handle);
}

Wdw
create_window(Arena *arena)
{
  GFX_Window wdw = gfx_window_create();
  if (wdw == 0)
  {
    os_handle_bad_error(err_get_last_error());
  }
  Scratch scratch = scratch_begin(0, 0);
  gfx_window_set_title(wdw, push_str8f(scratch.arena, "(%u)", (U32)wdw));
  scratch_end(&scratch);
  
  B32 equipped = gfx_gl_equip(wdw);
  if (!equipped)
  {
    os_handle_bad_error(err_get_last_error());
  }
  B32 q2d_initialized = q2d_init(S8("code"));
  if(!q2d_initialized)
  {
    os_handle_bad_error(err_get_last_error());
  }
  gfx_window_show(wdw);
  
  Wdw result = {0};
  result.handle = wdw;
  result.event_arena = push_sub_arena(arena, sizeof(Event_Timed)*EVENT_CACHE_COUNT);
  return(result);
}

typedef struct Input_Table Input_Table;
struct Input_Table
{
  B32 keys[GFXKey_COUNT];
  B32 mouse_buttons[GFXMouseCode_COUNT];
  V2f scroll;
};

Input_Table *
input_table_from_events(Arena *arena, GFX_Event_List *events)
{
  Input_Table *result = PushStruct(arena, Input_Table);
  for(EachNode(GFX_Event, node, events->first))
  {
    switch(node->kind)
    {
      default: {}; break;
      
      case GFXEventKind_KeyUp: 
      case GFXEventKind_KeyDown:
      {
        result->keys[node->key.code] = (node->kind == GFXEventKind_KeyDown);
      } break;
      
      case GFXEventKind_MouseUp: 
      case GFXEventKind_MouseDown:
      {
        result->mouse_buttons[node->key.code] = (node->kind == GFXEventKind_MouseDown);
      } break;
      
      case GFXEventKind_MouseWheel: 
      {
        result->scroll = v2f_add(result->scroll, node->mouse.wheel);
      } break;
    }
  }
  return(result);
}

#define K(name) GFXKey_##name

GFX_Key keyboard_layout[][15] = 
{
  {K(Escape), K(F1), K(F2), K(F3), K(F4), K(F5), K(F6), K(F7), K(F8), K(F9), K(F10), K(F11), K(F12), K(Insert), K(Delete)},
  {K(Backtick), K(1), K(2), K(3), K(4), K(5), K(6), K(7), K(8), K(9), K(0), K(Minus), K(Equal), K(Backspace)},
  {K(Tab), K(Q), K(W), K(E), K(R), K(T), K(Y), K(U), K(I), K(O), K(P), K(LeftBracket), K(RightBracket), K(Enter)},
  {K(Tab), K(A), K(S), K(D), K(F), K(G), K(H), K(J), K(K), K(L), K(Semicolon), K(Quote), K(Backslash)},
  {K(Shift), K(Backslash), K(Z), K(X), K(C), K(V), K(B), K(N), K(M), K(Comma), K(Period), K(Slash), K(Shift)},
  {K(Control), K(Super), K(Alt), K(Space), K(Alt), K(Super) ,K(Control), K(Left), K(Up), K(Right)}
};
#undef K

void
draw_keyboard_key(Q2D_State *render_state, Q2D_Font *font, Range_V2f key_range, V4f color, GFX_Key key)
{
  q2d_render_rect_color_ex(render_state, key_range, quad_colors_single(color), 10.0f, 0.5f, 0.0f);
  
  // NOTE(erb): center the string
  String8 str = {0};
  switch(key)
  {
    default: str = gfx_key_str8(key) ; break;
    case GFXKey_Enter: str = S8("<-"); break;
    case GFXKey_Tab  : str = S8("->"); break;
    case GFXKey_Shift: str = S8("S^"); break;
    case GFXKey_Up   : str = S8("^") ; break;
    case GFXKey_Down : str = S8("v") ; break;
    case GFXKey_Left : str = S8("<") ; break;
    case GFXKey_Right: str = S8(">") ; break;
  }
  
  V2f center = rng_v2f_center(key_range);
  V2f text_size = measure_text(render_state, str, font, 0, 0);
  V2f text_pos = v2f(center.x-text_size.x*0.5f,
                     center.y-text_size.y*0.5f+font->data.ascent_em*font->data.scale_em_to_pixels);
  // TODO(erb): split the text to what fits
  q2d_render_string_baseline(render_state, str, text_pos, font, 0, 0, color_white, true);
}

void
draw_input_table(Q2D_State *render_state, Q2D_Font *font, B32 *table, Range_V2f rect)
{
  V2f size = rng_v2f_size(rect);
  F32 row_height = size.y/ArraySize(keyboard_layout);
  
  V2f padding = v2f_scale(size, 0.01f);
  
  for(EachU32(row, ArraySize(keyboard_layout)))
  {
    GFX_Key *row_keys = (GFX_Key *)(keyboard_layout + row);
    
    U32 count = 0;
    for(EachU32(col, ArraySize(keyboard_layout[row])))
    {
      if(row_keys[col] != GFXKey_None)
      {
        count++;
      }
    }
    V2f cell_size = v2f(size.x/count, row_height);
    
    for(EachU32(col, count))
    {
      GFX_Key key = row_keys[col];
      if(key != GFXKey_None)
      {
        V2f key_pos = v2f_add(rect.min, v2f_hada(cell_size, v2f((F32)col, (F32)row)));
        
        GFX_Key key_split[] = {key, key==GFXKey_Up?GFXKey_Down:GFXKey_None};
        U32 split_count = key==GFXKey_Up?2:1;
        V2f split_size = v2f(cell_size.x, cell_size.y/split_count);
        
        F32 y = key_pos.y+padding.y;
        for(EachU32(split_idx, split_count))
        {
          GFX_Key split_key = key_split[split_idx];
          V4f color = table[split_key]?color_red:color_black;
          
          V2f split_pos = v2f(key_pos.x+padding.x, y);
          V2f key_size = v2f_sub(split_size, padding);
          Range_V2f key_range = rng_v2f_point_add(split_pos, key_size);
          
          draw_keyboard_key(render_state, font, key_range, color, split_key);
          
          y += key_size.y+padding.y;
        }
      }
    }
  }
}

void
animate_scroll(Q2D_State *render_state, F32 dt, Range_V2f rect, B32 up, V4f off_color, V4f on_color)
{
  q2d_scissor_begin(render_state, rng_v2f_floor_inclusive(rect));
  
  V2f rect_size = rng_v2f_size(rect);
  V2f size = v2f_scale(v2ff(rect_size.x), 1.5f);
  
  static F32 timed_disp = 0;
  timed_disp += size.y*dt*10.0f;
  if(timed_disp > size.y)
  {
    timed_disp = 0;
  }
  
  F32 disp_y = 0.0f;
  if(up)
  {
    disp_y = timed_disp;
  }
  else
  {
    disp_y = size.y - timed_disp;
  }
  
  U32 count = 5;
  F32 height = size.y/(count-1);
  V2f point = v2f(rect.min.x + (rect_size.x - size.x)*0.5f, 
                  rect.min.y - count*height + disp_y);
  for(EachU32(idx, count*2))
  {
    V4f color = (idx%2)==0?on_color:off_color;
    Range_V2f edge_range = rng_v2f_point_add(point, v2ff(size.x));
    q2d_render_rect_color_ex(render_state, edge_range, 
                             quad_colors_single(color), size.x*0.5f, 1.0f, 0.0f);
    point.y += height;
  }
  
  q2d_scissor_end(render_state);
}

void
draw_input_mouse(Q2D_State *render_state, B32 *table, V2f scroll, Range_V2f mouse_rect)
{
  V4f out_color = v4f(0.5f, 0.5f, 0.5f, 1.0f);
  V4f on_color = color_blue;
  V4f off_color = color_black;
  
  // NOTE(erb): background
  q2d_render_rect_color(render_state, mouse_rect, out_color);
  
  F32 divider_pct = 0.3f;
  V2f mouse_size = rng_v2f_size(mouse_rect);
  V2f top_size = v2f(mouse_size.x, mouse_size.y*divider_pct);
  V2f bottom_size = v2f(mouse_size.x, mouse_size.y*(1.0f-divider_pct));
  
  F32 padding = Max(mouse_size.x, mouse_size.y)*0.01f;
  F32 half_padding = padding*0.5f;
  
  // NOTE(erb): backdrop
  {
    V2f pos = mouse_rect.min;
    pos.y += top_size.y+half_padding;
    V2f size = bottom_size;
    size.y -= half_padding;
    q2d_render_rect_color(render_state, rng_v2f_point_add(pos, size), off_color);
  }
  
  // NOTE(erb): buttons
  GFX_Mouse_Code buttons[] = {GFXMouseCode_Left, GFXMouseCode_Right};
  V2f button_size = v2f_sub(v2f(top_size.x/ArraySize(buttons), top_size.y), v2ff(half_padding));
  for(EachU32(idx, ArraySize(buttons)))
  {
    V4f color = table[buttons[idx]]?on_color:off_color;
    V2f pos = mouse_rect.min;
    pos.x += (button_size.x+padding)*idx;
    
    Range_V2f rect = rng_v2f_point_add(pos, button_size);
    q2d_render_rect_color(render_state, rect, color);
  }
  
  // NOTE(erb): scroll
  {
    V2f size = v2f_scale(mouse_size, 0.175f);
    V2f center = v2f_add(mouse_rect.min, v2f(mouse_size.x*0.5f, top_size.y));
    V2f top_left = v2f_sub(center, v2f_scale(size, 0.5f));
    Range_V2f rect = rng_v2f_point_add(top_left, size);
    Range_V2f inner_rect = rng_v2f_center_sub(rect, v2ff(padding));
    
    {
      // NOTE(erb): background
      q2d_render_rect_color(render_state, rect, out_color);
    }
    
    {
      // NOTE(erb): actuall scroll
      V4f color = table[GFXMouseCode_Middle]?on_color:off_color;
      q2d_render_rect_color(render_state, inner_rect, color);
    }
    
    // NOTE(erb): animate scroll
    if(scroll.y != 0.0f)
    {
      B32 up = scroll.y > 0.0f;
      animate_scroll(render_state, 0.001f, inner_rect, up, off_color, on_color);
    }
  }
}

OS_ENTRY_POINT()
{
  F32 target_seconds_per_frame = 1.0f/60.0f;
  
  //- os init
  B32 os_initialized = os_init();
  if (!os_initialized)
  {
    os_handle_bad_error(err_get_last_error());
  }
  Thread_Context tctx = {0};
  thread_context_init(&tctx);
  os_thread_context_set(&tctx);
  
  Arena arena_store = {};
  {
    U64 size = MB(256);
    arena_init(&arena_store, os_memory_commit(size), size);
  }
  Arena *arena = &arena_store;
  
  String8 current_dir = os_get_current_directory(arena);
  os_logf("current_dir = %.*s", S8Expand(current_dir));
  
  //- gfx init
  B32 gfx_initialized = gfx_init();
  if (!gfx_initialized)
  {
    os_handle_bad_error(err_get_last_error());
  }
  B32 gl_initialized = gfx_gl_init();
  if (!gl_initialized)
  {
    os_handle_bad_error(err_get_last_error());
  }
  
  //- q2d
  Q2D_State *render_state = make_render_data(arena);
  
  // NOTE(erb): create windows
  Wdw windows[2];
  U32 window_count = ArraySize(windows);
  for (EachU32(idx, window_count))
  {
    windows[idx] = create_window(arena);
  }
  
  B32 fnt_initialized = fnt_init();
  if(!fnt_initialized)
  {
    os_handle_bad_error(err_get_last_error());
  }
  
  // NOTE(erb): font baking
  Q2D_Font font = {0};
  {
    String8 font_file = os_debug_read_entire_file(S8("data/arial.ttf"));
    font = q2d_load_font(arena, font_file, 32.0f);
    os_debug_free_file_memory((void *)font_file.value);
  }
  
  F32 font_height = fnt_get_height_px(&font.data);
  F32 initial_y = font.data.ascent_em*font.data.scale_em_to_pixels;
  
  // NOTE(erb): loop
  F32 delta_time = target_seconds_per_frame;
  OS_Clock_Time last_clock = os_get_clock_time();
  for (;;) 
  {
    Scratch frame_scratch = arena_get_scratch(arena);
    
    // NOTE(erb): check if a window has been closed
    GFX_Event_List all_events = gfx_poll_events(frame_scratch.arena);
    
    //- update window
    for (EachU32(idx, window_count))
    {
      Scratch wdw_scratch = arena_get_scratch(frame_scratch.arena);
      
      Wdw *window = (windows + idx);
      GFX_Event_List events = filter_window_events(&all_events, window->handle);
      
      V2f client_size = v2fi(gfx_window_get_client_size(window->handle));
      
      for(EachNode(GFX_Event, event, events.first))
      {
        Event_Timed *timed = 0;
        if(window->free_event)
        {
          timed = SLLStackPop(window->free_event);
        }
        else
        {
          timed = PushStruct(&window->event_arena, Event_Timed);
        }
        SLLStackPush(window->first_event, timed);
        timed->timer = 2.0f;
        timed->event = *event;
        timed->event.next = 0;
        timed->event.prev = 0;
      }
      
      q2d_begin(window->handle);
      {
        V2f cursor = v2f(0, initial_y);
        F32 max_width = 0;
        F32 max_x = 0;
        U32 event_idx = 0;
        for(Event_Timed *node = window->first_event, *prev = window->first_event; node; node = node->next, ++event_idx)
        {
          node->timer -= delta_time;
          if(node->timer >= 0 && cursor.x <= client_size.x)
          {
            V4f color = with_alpha(color_blue, Cap01(node->timer));
            
            Scratch event_scratch = arena_get_scratch(wdw_scratch.arena);
            String8 s = push_str8f(event_scratch.arena, "%u-%.*s", event_idx, S8Expand(gfx_event_str8(arena, &node->event)));
            F32 width = q2d_render_string_baseline(render_state, s, cursor, &font, 0, 0, color, true);
            arena_scratch_reset(&event_scratch);
            
            max_width = Max(max_width, width);
            cursor.y += font_height;
            if(cursor.y >= client_size.y)
            {
              max_x += 10.0f+max_width;
              cursor = v2f(max_x, initial_y);
            }
            prev = node;
          }
          else
          {
            // NOTE(erb): remove the node
            prev->next = node->next;
          }
        }
        
        Input_Table *input_table = input_table_from_events(wdw_scratch.arena, &events);
        {
          F32 l = 0.9f;
          Range_V2f keys_rect = rng_v2f_point_add(v2f_scale(client_size, (1.0f-l)*0.5f),
                                                  v2f_scale(client_size, l));
          draw_input_table(render_state, &font, input_table->keys, keys_rect);
        }
        
        {
          F32 m = 0.3f;
          V2f mouse_size = v2f(client_size.x*m*0.75f, client_size.x*m);
          V2f pos = gfx_window_get_cursor_position(window->handle);
          pos.x -= mouse_size.x*0.5f;
          draw_input_mouse(render_state, input_table->mouse_buttons, 
                           input_table->scroll, rng_v2f_point_add(pos, mouse_size));
        }
        
        {
          F32 fps = 1.0f/delta_time;
          String8 fps_str = push_str8f(wdw_scratch.arena, "%.3f", fps);
          
          V2f fps_size = measure_text(render_state, fps_str, &font, 0, 0);
          V2f fps_box_pos = v2f(client_size.x-fps_size.x, 0);
          q2d_render_rect_color(render_state, rng_v2f_point_add(fps_box_pos, fps_size), color_debug);
          V2f fps_str_pos = v2f(fps_box_pos.x, initial_y);
          q2d_render_string_baseline(render_state, fps_str, fps_str_pos, &font, 0, 0, color_green, true);
        }
      }
      q2d_render(render_state);
      q2d_end(window->handle, render_state);
      
      scratch_end(&wdw_scratch);
    }
    
    arena_scratch_reset(&frame_scratch);
    
    if(window_count == 0)
    {
      break;
    }
    
    F32 seconds_elapsed = os_seconds_elapsed(last_clock, os_get_clock_time());
    if(seconds_elapsed <= target_seconds_per_frame)
    {
      os_sleep_ms(floor_f32_to_i32(seconds_elapsed*1000.0f));
      do
      {
        seconds_elapsed = os_seconds_elapsed(last_clock, os_get_clock_time());
      }
      while(seconds_elapsed <= target_seconds_per_frame);
    }
    OS_Clock_Time end_clock = os_get_clock_time();
    delta_time = os_seconds_elapsed(last_clock, end_clock);
    last_clock = end_clock;
  }
  
  // TODO(erb): destroy render_data
  //- release
  os_release();
  return(0);
}
