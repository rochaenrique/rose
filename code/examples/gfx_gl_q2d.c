// headers
#include "base/base.h"
#include "img/img.h"
#include "fnt/fnt.h"
#include "gfx/gfx.h"
#include "gfx/q2d/q2d.h"

// implementation
#include "base/base.c"
#include "img/img.c"
#include "fnt/fnt.c"
#include "gfx/gfx.c"
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
    gfx_window_set_title(window, push_str8f(scratch.arena, "GFX RENDER"));
    gfx_window_set_size(window, v2i(800, 600));
    scratch_end(&scratch);
  }
  
  //- equip gl
  B32 equipped = gfx_gl_equip(window);
  if (!equipped)
  {
    os_handle_bad_error(err_get_last_error());
  }
  gfx_window_show(window);
  
  String8 texture_paths[] = 
  {
    S8("data/smile.png"),
    S8("data/penguin.png"),
    S8("data/brick.jpg"),
  };
  
  Q2D_Image_Texture textures[ArraySize(texture_paths)] = {0};
  
  Arena arena_store = {};
  {
    U64 size = MB(128);
    arena_init(&arena_store, os_memory_commit(size), size);
  }
  Arena *arena = &arena_store;
  
  for (EachIdx(idx, texture_paths))
  {
    Scratch image_scratch = arena_get_scratch(arena);
    
    String8 file_mem = os_debug_read_entire_file(texture_paths[idx]);
    IMG_Loaded image = img_load_from_memory(image_scratch.arena, file_mem);
    textures[idx] = q2d_create_texture_from_image(&image);
    
    os_debug_free_file_memory(file_mem.value);
    arena_scratch_reset(&image_scratch);
  }
  
  B32 q2d_initialized = q2d_init(S8("code"));
  if (!q2d_initialized)
  {
    os_handle_bad_error(err_get_last_error());
  }
  
  Q2D_State *render_state = make_render_data(arena);
  
  //- update loop
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
    if(should_stop)
    {
      break;
    }
    
    {
      V2f point = gfx_window_get_cursor_position(window);
      V2f client_size = v2fi(gfx_window_get_client_size(window));
      
      for (EachIdx(idx, textures))
      {
        F32 x = ((F32)idx/ArraySize(textures))*client_size.x;
        q2d_append_render_rect(render_state,
                               rng_v2f_point_add(v2f(x, 0), v2ff(point.y)), 
                               texture_rect_range(textures[idx]),
                               .1f, .1f, .0f, 
                               quad_colors_single(color_white), 
                               textures[idx].handle);
      }
    }
    
    //- render stuff
    q2d_begin(window);
    {
      q2d_render(render_state);
    }
    q2d_end(window, render_state);
  }
  
  // TODO(erb): destroy render_data
  //- release
  gfx_window_destroy(window);
  os_release();
  return(0);
}
