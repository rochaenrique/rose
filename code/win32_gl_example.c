#include <windows.h>
#include <psapi.h>
#include <stdio.h>

#include "base/base.h"
#include "gfx/gl/gl.h"
#include "gfx/gl/win32/win32_gl.h"
#include "os/win32.h"

#include "base/base.c"
#include "gfx/gl/win32/win32_gl.c"
#include "os/win32.c"

int __stdcall WinMain(HINSTANCE instance, 
                      HINSTANCE prev_instance, 
                      LPSTR cmd_line, 
                      int show)
{
  os_init();
  
  Arena permanent_arena = arena_alloc(mb(128), w32_memory_commit);
  Arena transient_arena = arena_alloc(mb(128), w32_memory_commit);
  
  Thread_Context *tctx = push_struct(&permanent_arena, Thread_Context);
  thread_context_init(tctx, w32_memory_commit);
  thread_context_set(tctx);
  
  //- clock setup
  W32_Counters counters = w32_init_counters();
  
  //- opengl init
  // TODO(erb): have error handling with message box
  w32_init_opengl(instance);
  
  //- window creation
  // TODO(erb): have error handling with message box
  W32_GL_Window gl_window = w32_make_opengl_window(instance); 
  HWND window = gl_window.window_handle;
  HGLRC gl_context = gl_window.gl_context;
  
  //- project stuff setup
  Codebase_File_Paths paths = w32_get_codebase_file_paths(&permanent_arena, instance);
  
  //- load app layer
  // TODO(erb): handle this error with message box
  
#if 0
  W32_App_Library app_library = w32_load_app_library(&transient_arena, paths.library, paths.temp_library);
  
  Render_Data renderer = {0};
  {
    Read_File_Result vertex_file = w32_debug_read_entire_file(vertex_cpath);
    Read_File_Result fragment_file = w32_debug_read_entire_file(fragment_cpath);
    renderer = gl_make_renderer(&permanent_arena, 
                                (char *)vertex_file.memory, 
                                (char *)fragment_file.memory);
    w32_debug_free_file_memory(vertex_file.memory);
    w32_debug_free_file_memory(fragment_file.memory);
  }
#endif
  
  //- update loop
  ShowWindow(window, show); // TODO(erb): think of a good place to put this function, maybe a pre-first-frame?
  w32_pre_first_frame(&counters);
  for (;;) 
  {
    B32 should_stop = w32_poll_events();
    if (should_stop) 
    {
      break;
    }
    
    //- w32 gl begin
    HDC dc = w32_gl_render_begin(window, gl_context);
    
    //- render
    glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    //- w32 gl end
    w32_wait_for_target_frame_time(&counters);
    w32_gl_render_end(window, dc);
    
    w32_gather_counters(&counters);
  }
  
  os_release();
  return 0;
}
