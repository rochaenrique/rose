#include <math.h>

// headers
#include "base/base.h"
#include "img/img.h"
#include "gfx/gfx.h"
#include "fnt/fnt.h"
#include "gfx/q2d/q2d.h"
#include "app_bridge.h"

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

typedef enum Timed_File_Kind
{
  TimedFileKind_Shader,
  TimedFileKind_AppLibrary,
} Timed_File_Kind;

typedef struct Timed_File Timed_File;
struct Timed_File
{
  FILETIME time;
  String8 path;
  Timed_File_Kind kind;
  Timed_File *next;
};

Timed_File *
push_timed_file(Arena *arena, String8 path, Timed_File_Kind kind)
{
  Timed_File *file = PushStruct(arena, Timed_File);
  file->kind = kind;
  file->path = path;
  file->time = w32_get_last_write_time(file->path);
  return(file);
}

B32
timed_file_check(Timed_File *file)
{
  FILETIME new_write_time = w32_get_last_write_time(file->path);
  LONG diff = CompareFileTime(&new_write_time, &file->time);
  B32 result = (diff == 1);
  return(result);
}

void
timed_file_update(Timed_File *file)
{
  file->time = w32_get_last_write_time(file->path);
}


typedef struct W32_App_Library W32_App_Library;
struct W32_App_Library
{
  Update_And_Render_Func *update_and_render;
  HMODULE module;
  B32 valid;
};

void 
w32_unload_library(W32_App_Library *library)
{
  if(library->module != 0)
  {
    BOOL free_result = FreeLibrary(library->module);
    if(!free_result)
    {
      Scratch scr = scratch_begin(0, 0);
      String8 err_str = w32_get_last_error_str8(scr.arena, GetLastError());
      os_log(err_str);
      scratch_end(&scr);
      InvalidCodePath;
    }
  }
  ZeroStruct(library);
}

W32_App_Library 
w32_load_app_library(String8 path, String8 temp_path)
{
  Scratch scratch = scratch_begin(0, 0);
  char *cpath = push_cstr(scratch.arena, path);
  char *temp_cpath = push_cstr(scratch.arena, temp_path);
  
  // NOTE(erb): try really hard to copy the file
  BOOL copied = false;
  for(U32 tries = 0; tries < 1000 && !copied; ++tries)
  {
    copied = CopyFileA(cpath, temp_cpath, false);
    if(!copied)
    {
      os_sleep_ms(1);
    }
  }
  if (!copied)
  {
    String8 err_str = w32_get_last_error_str8(scratch.arena, GetLastError());
    os_logf("w32_load_library CopyFileA() failed (%S -> %S): %S", path, temp_path, err_str);
    InvalidCodePath;
  }
  
  Update_And_Render_Func *update_func = 0;
  HMODULE module = LoadLibraryA(temp_cpath);
  if (module != 0) 
  {
    update_func = (Update_And_Render_Func *)GetProcAddress(module, "update_and_render");
    Assert(update_func != 0);
  }
  else
  {
    OutputDebugStringA("w32_load_library LoadLibraryA() failed\n");
    InvalidCodePath;
  }
  scratch_end(&scratch);
  
  W32_App_Library result = {0};
  result.update_and_render = update_func;
  result.module = module;
  result.valid = true;
  return(result);
}

void
gfx_eat_event(GFX_Event_List *event_list, GFX_Event *event)
{
  event_list->count--;
  DLLRemove(event_list->first, event_list->last, event);
}

PUSH_CLIPBOARD_STR8_FUNC(app_push_clipboard) // Arena *arena, void *data
{
  SupressUnused(data);
  String8 result = gfx_get_clipboard_str8(arena);
  return(result);
}

SET_CLIPBOARD_STR8_FUNC(app_set_clipboard) // String8 str, void *data
{
  SupressUnused(data);
  gfx_set_clipboard_str8(str);
}

SET_CURSOR_FUNC(app_set_cursor) // GFX_Cursor_Kind kind, void *data
{
  SupressUnused(data);
  gfx_set_cursor(kind);
}

READ_ENTIRE_FILE_STR8(app_read_entire_file_str8) // Arena *arena, String8 file_path
{
  String8 result = os_read_entire_file_str8_arena(arena, file_path);
  return(result);
}

WRITE_ENTIRE_FILE_STR8(app_write_entire_file_str8) // String8 file_path, String8 contents
{
  os_debug_write_entire_file(file_path, contents);
}

typedef struct Update_Params Update_Params;
struct Update_Params
{
  GFX_Window window;
  App_Memory *app_memory;
  Q2D_State *render_state;
  Q2D_Font *font; 
  GFX_Event_List *events;
  Platform_API platform_api;
  F32 delta_time;
  String8 current_directory;
  
  Update_And_Render_Func *update_func;
};

void
update(void *data)
{
  if(data != 0)
  {
    Update_Params *params = (Update_Params *)data;
    q2d_begin(params->window);
    {
      params->update_func(params->app_memory, 
                          params->render_state, 
                          params->font, 
                          gfx_window_get_client_size(params->window), 
                          params->events, 
                          params->platform_api, 
                          params->delta_time, 
                          params->current_directory);
    }
    q2d_render(params->render_state);
    q2d_end(params->window, params->render_state);
  }
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
  
  Platform_API platform_api = {0};
  platform_api.clipboard.push_str8_func = app_push_clipboard;
  platform_api.clipboard.set_str8_func = app_set_clipboard;
  platform_api.cursor.set_func = app_set_cursor;
  platform_api.read_entire_file_str8 = app_read_entire_file_str8;
  platform_api.write_entire_file_str8 = app_write_entire_file_str8;
  {
    Arena arena_store = {};
    {
      U64 size = MB(128);
      arena_init(&arena_store, os_memory_commit(size), size);
    }
    Arena *arena = &arena_store;
    
    String8 current_dir = os_get_current_directory(arena);
    os_logf("current_dir = %S", current_dir);
    
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
    gfx_window_set_title(window, S8("UI"));
    
    //- equip gl
    B32 equipped = gfx_gl_equip(window);
    if (!equipped)
    {
      os_handle_bad_error(err_get_last_error());
    }
    gfx_window_show(window);
    
    // NOTE(erb): paths
    String8 shader_search_folder = S8("code");
    String8 app_library_path = S8("code/build/win32_ui_app.dll");
    String8 app_library_temp_path = S8("code/build/win32_ui_app.temp.dll");
    Timed_File *first_timed_file = 0;
    
    //- q2d
    B32 q2d_initialized = q2d_init(shader_search_folder);
    if (!q2d_initialized)
    {
      os_handle_bad_error(err_get_last_error());
    }
    
    // NOTE(erb): shaders
    {
      Scratch sh_scratch = scratch_begin(&arena, 1);
      String8_Array shader_names = q2d_get_shader_file_names(sh_scratch.arena);
      for(U32Each(idx, shader_names.count))
      {
        String8 path = os_concat_path(arena, shader_search_folder, shader_names.items[idx]);
        Timed_File *file = push_timed_file(arena, path, TimedFileKind_Shader);
        SLLStackPush(first_timed_file, file);
      }
      scratch_end(&sh_scratch);
    }
    
    // NOTE(erb): app library
    W32_App_Library app_library = {0};
    {
      app_library = w32_load_app_library(app_library_path, app_library_temp_path);
      Assert(app_library.valid);
      Timed_File *file = push_timed_file(arena, app_library_path, TimedFileKind_AppLibrary);
      SLLStackPush(first_timed_file, file);
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
      String8 font_file = os_debug_read_entire_file(S8("data/times.ttf"));
      font = q2d_load_font(arena, font_file, 48.0f);
      os_debug_free_file_memory((void *)font_file.value);
    }
    
    App_Memory app_memory = {0};
    {
      app_memory.size = GB(2);
      app_memory.buffer = os_memory_commit(app_memory.size);
      app_memory.first_attach = true;
    }
    
    gfx_window_set_resize_update_func(window, update);
    
    //- update loop
    F32 target_seconds_per_frame = 1.0f/60.0f;
    F32 tick = 0;
    F32 delta_time = target_seconds_per_frame;
    OS_Clock_Time last_clock = os_get_clock_time();
    
    GFX_Event_List events = {0};
    
    Update_Params update_params = {0};
    update_params.window = window;
    update_params.app_memory = &app_memory;
    update_params.render_state = render_state; 
    update_params.font = &font;
    update_params.platform_api = platform_api;
    update_params.delta_time = target_seconds_per_frame;
    update_params.update_func = app_library.update_and_render;
    update_params.events = &events;
    update_params.current_directory = os_get_current_directory(arena); // NOTE(erb): allocation
    gfx_window_set_resize_update_data(window, &update_params);
    
    for(;;++tick) 
    {
      {
        B32 shader_changed = false;
        for(EachNode(Timed_File, file, first_timed_file))
        {
          if (timed_file_check(file))
          {
            switch(file->kind)
            {
              case TimedFileKind_Shader:
              {
                if(!shader_changed)
                {
                  shader_changed = true;
                  q2d_update_shaders(shader_search_folder);
                }
              } break;
              
              case TimedFileKind_AppLibrary:
              {
                w32_unload_library(&app_library);
                app_library = w32_load_app_library(app_library_path, app_library_temp_path);
                Assert(app_library.valid);
                app_memory.first_attach = true;
              } break;
            }
            timed_file_update(file);
          }
        }
      }
      
      Scratch frame_scratch = arena_get_scratch(arena);
      
      events = gfx_poll_events(frame_scratch.arena);
      
      B32 should_stop = false;
      for(EachNode(GFX_Event, event, events.first))
      {
        if (event->kind == GFXEventKind_WindowClose)
        {
          Assert(event->window == window);
          gfx_eat_event(&events, event);
          should_stop = true;
          break;
        }
      }
      if(should_stop)
      {
        break;
      }
      
      {
        update_params.events = &events;
        update_params.delta_time = delta_time;
        update_params.update_func = app_library.update_and_render;
        update(&update_params);
        gfx_window_set_resize_update_data(window, &update_params);
      }
      
      arena_scratch_reset(&frame_scratch);
      
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
      else
      {
        // NOTE(erb): skipped frame
        os_log(S8("skipped frame"));
      }
      OS_Clock_Time end_clock = os_get_clock_time();
      delta_time = os_seconds_elapsed(last_clock, end_clock);
      last_clock = end_clock;
    }
    
    // TODO(erb): destroy render_data
    //- release
    gfx_window_destroy(window);
    os_release();
    return(0);
  }
}