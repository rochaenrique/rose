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

FILETIME 
w32_get_last_write_time(String8 path)
{
  FILETIME result = {0};
  
  Scratch scratch = scratch_begin(0, 0);
  WIN32_FIND_DATA find_data;
  HANDLE find_handle = FindFirstFileA(push_cstr(scratch.arena, path), &find_data);
  scratch_end(&scratch);
  
  if (find_handle != INVALID_HANDLE_VALUE) 
  {
    result = find_data.ftLastWriteTime;
    FindClose(find_handle);
  }
  return result;
}

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
  gfx_window_set_title(window, S8("COORD TEST"));
  gfx_window_set_position(window, v2i(150, 300));
  
  //- equip gl
  B32 equipped = gfx_gl_equip(window);
  if (!equipped)
  {
    os_handle_bad_error(err_get_last_error());
  }
  gfx_window_show(window);
  
  //- q2d
  typedef struct Timed_File Timed_File;
  struct Timed_File
  {
    FILETIME time;
    String8 path;
    Timed_File *next;
  };
  
  String8 shader_search_folder = S8("code");
  Timed_File *first_timed_file = 0;
  {
    B32 q2d_initialized = q2d_init(shader_search_folder);
    if (!q2d_initialized)
    {
      os_handle_bad_error(err_get_last_error());
    }
    
    Scratch sh_scratch = scratch_begin(&arena, 1);
    
    String8_Array shader_names = q2d_get_shader_file_names(sh_scratch.arena);
    for(U32Each(idx, shader_names.count))
    {
      Timed_File *file = PushStruct(arena, Timed_File);
      file->path = os_concat_path(arena, shader_search_folder, shader_names.items[idx]);
      file->time = w32_get_last_write_time(file->path);
      SLLStackPush(first_timed_file, file);
    }
    
    scratch_end(&sh_scratch);
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
  
  String8 texture_paths[] = 
  {
    S8("data/smile.png"),
    S8("data/penguin.png"),
    S8("data/brick.jpg"),
  };
  Q2D_Image_Texture textures[ArraySize(texture_paths)] = {0};
  
  for (EachIdx(idx, texture_paths))
  {
    Scratch image_scratch = arena_get_scratch(arena);
    
    String8 file_mem = os_debug_read_entire_file(texture_paths[idx]);
    IMG_Loaded image = img_load_from_memory(image_scratch.arena, file_mem);
    textures[idx] = q2d_create_texture_from_image(&image);
    
    os_debug_free_file_memory(file_mem.value);
    arena_scratch_reset(&image_scratch);
  }
  
  //- update loop
  F32 tick = 0;
  for (;;++tick) 
  {
    Scratch frame_scratch = arena_get_scratch(arena);
    
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
      B32 shader_changed = false;
      for(EachNode(Timed_File, file, first_timed_file))
      {
        FILETIME new_write_time = w32_get_last_write_time(file->path);
        LONG diff = CompareFileTime(&new_write_time, &file->time);
        if(diff == 1)
        {
          file->time = new_write_time;
          shader_changed = true;
        }
      }
      if(shader_changed)
      {
        q2d_update_shaders(shader_search_folder);
      }
    }
    
    V2i client_size = gfx_window_get_client_size(window);
    
    //- render stuff
    q2d_begin(window);
    {
      U32 x_count = 3;
      U32 y_count = 3;
      U32 total_count = x_count*y_count;
      
      V2f size = v2f_hada_div(v2fi(client_size), v2f((F32)x_count, (F32)y_count));
      
      for(U32Each(y, y_count))
      {
        for(U32Each(x, x_count))
        {
          
          F32 t = (F32)(x*y/total_count);
          V4f color = v4f_lerp(color_red, color_blue, t);
          U32 idx = (y*x_count + x);
          Q2D_Image_Texture *tex = textures + (idx % ArraySize(textures));
          V2f position = v2f_hada(size, v2f((F32)x, (F32)y));
          
          B32 should_sc = (x==1 && y==1);
          
          if(should_sc)
          {
            V2i pos = v2i((I32)position.x, (I32)position.y);
            V2i sz = v2i((I32)size.x, (I32)size.y);
            Range_V2i rect = rng_v2i_center_sub(rng_v2i_point_add(pos, sz), v2ii(50));
            q2d_scissor_begin(render_state, rect);
          }
          
          q2d_render_texture_src_dest(render_state, 
                                      rng_v2f_point_add(v2ff(0), v2fi(tex->size)), 
                                      rng_v2f_point_add(position, size), 
                                      tex->handle, color);
          
          if(should_sc)
          {
            q2d_scissor_end(render_state);
          }
        }
      }
    }
    q2d_render(render_state);
    q2d_end(window, render_state);
    
    arena_scratch_reset(&frame_scratch);
  }
  
  // TODO(erb): destroy render_data
  //- release
  gfx_window_destroy(window);
  os_release();
  return(0);
}
