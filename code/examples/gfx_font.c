//#define FNT_BACKEND FNT_BACKEND_FREETYPE

#include <uchar.h>

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

OS_ENTRY_POINT()
{
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
    U64 size = MB(128);
    arena_init(&arena_store, os_memory_commit(size), size);
  }
  Arena *arena = &arena_store;
  
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
  
  //- window init
  GFX_Window window = gfx_window_create();
  if (window == 0) 
  {
    os_handle_bad_error(err_get_last_error());
  }
  
  {
    Scratch scratch = scratch_begin(0, 0);
    gfx_window_set_title(window, S8(FNT_BACKEND_STRING));
    scratch_end(&scratch);
  }
  
  //- equip gl
  B32 equipped = gfx_gl_equip(window);
  if (!equipped)
  {
    os_handle_bad_error(err_get_last_error());
  }
  gfx_window_show(window);
  
  //- q2d
  B32 q2d_initialized = q2d_init(S8("code"));
  if (!q2d_initialized)
  {
    os_handle_bad_error(err_get_last_error());
  }
  Q2D_State *render_state = make_render_data(arena);
  
  B32 fnt_initialized = fnt_init();
  if(!fnt_initialized)
  {
    os_handle_bad_error(err_get_last_error());
  }
  
  //- font baking
  Q2D_Font font = {0};
  {
    String8 font_file = os_debug_read_entire_file(S8("data/arial.ttf"));
    font = q2d_load_font(arena, font_file, 64.0f);
    os_debug_free_file_memory((void *)font_file.value);
  }
  
  //- update loop
  
  F32 font_ascent = font.data.ascent_em*font.data.scale_em_to_pixels;
  F32 font_descent = font.data.descent_em*font.data.scale_em_to_pixels;
  F32 font_height = fnt_get_height_px(&font.data);
  
  for (;;) 
  {
    B32 should_stop = false;
    {
      Scratch scratch = scratch_begin(0, 0);
      GFX_Event_List events = gfx_poll_events(scratch.arena);
      for(EachNode(GFX_Event, event, events.first))
      {
        if (event->kind == GFXEventKind_WindowClose)
        {
          Assert(event->window == window);
          should_stop = true;
          break;
        }
      }
      scratch_end(&scratch);
    }
    if (should_stop) 
    {
      break;
    }
    
    V2f client_size = v2fi(gfx_window_get_client_size(window));
    
    //- render stuff
    q2d_begin(window);
    {
      Scratch scratch = scratch_begin(&arena, 1);
      
      String8 str = S8("1.ãAHApqellog!!.j.\n2.ãsecond!!\n3.last!\n0123456789%$");
      String8_List lines = str8_split_char(scratch.arena, str, '\n');
      
      // NOTE(erb): set the first lines baseline at the center of the screen
      F32 lines_height = lines.count*font_height;
      V2f cursor = v2f(0, (client_size.y-lines_height)*0.5f+font_ascent);
      for(EachNode(String8_Node, line_node, lines.first))
      {
        // NOTE(erb): don't actually render, just calculate (notice last parameter - should_draw=false)
        F32 width = q2d_render_string_baseline(render_state, line_node->str, cursor, &font, 0, 0, color_black, false);
        
        // NOTE(erb): render baseline, ascender and descender
        q2d_render_rect_color(render_state, rng_v2f_point_add(cursor, v2f(width, 2)), color_red);
        q2d_render_rect_color(render_state,
                              rng_v2f_point_add(v2f(cursor.x, cursor.y-font_ascent), v2f(width, font_ascent)), 
                              with_alpha(color_green, 0.3f));
        q2d_render_rect_color(render_state,
                              rng_v2f_point_add(v2f(cursor.x, cursor.y), v2f(width, font_descent)),
                              with_alpha(color_blue, 0.3f));
        
        // NOTE(erb): render the text
        q2d_render_string_baseline(render_state, line_node->str, cursor, &font, 0, 0, color_black, true);
        
        // NOTE(erb): advance the cursor by the baseline to baseline distance
        cursor.y += font_height;
      }
      
      scratch_end(&scratch);
    }
    q2d_render(render_state);
    q2d_end(window, render_state);
  }
  
  // TODO(erb): destroy render_data
  //- release
  gfx_window_destroy(window);
  os_release();
  return(0);
}
