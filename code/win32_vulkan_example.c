#include <windows.h>
#include <psapi.h>
#include <stdio.h>

#include "base/base.h"
#include "os/win32.h"
#include "gfx/vulkan/vulkan.h"
#include "gfx/vulkan/win32/win32_vulkan.h"

#include "base/base.c"
#include "os/win32.c"
#include "gfx/vulkan/win32/win32_vulkan.c"

int __stdcall WinMain(HINSTANCE instance, 
                      HINSTANCE prev_instance, 
                      LPSTR cmd_line, 
                      int show)
{
  os_init();
  
  Thread_Context tctx = {0};
  thread_context_init(&tctx, w32_memory_commit);
  thread_context_set(&tctx);
  
  //- vulkan init
  // TODO(erb): have error handling with message box
  w32_init_vulkan(instance);
  
  //- window creation
  // TODO(erb): have error handling with message box
  //W32_GL_Window gl_window = w32_make_opengl_window(instance); 
  HWND window = w32_make_vulkan_window(instance);
  
  //- update loop
  ShowWindow(window, show); // TODO(erb): think of a good place to put this function, maybe a pre-first-frame?
  for (;;) 
  {
    B32 should_stop = w32_poll_events();
    if (should_stop) 
    {
      break;
    }
    
    //- pre render
    // TODO(erb)
    
    //- render
    // TODO(erb)
    
    //- wait for target time
    // TODO(erb)
    
    //- render
    // TODO(erb)
    
    //- update counters
    // TODO(erb)
  }
  
  os_release();
  return 0;
}
