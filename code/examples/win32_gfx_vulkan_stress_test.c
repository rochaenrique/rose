#define GFX_BACKEND GFX_BACKEND_VULKAN

// headers
#include "base/base.h"
#include "gfx/gfx.h"

// implementation
#include "base/base.c"
#include "gfx/gfx.c"

int __stdcall WinMain(HINSTANCE instance, 
                      HINSTANCE prev_instance, 
                      LPSTR cmd_line, int show)
{
  SupressUnused(instance);
  SupressUnused(prev_instance);
  SupressUnused(cmd_line);
  SupressUnused(show);
  
  B32 os_initialized = os_init();
  if (!os_initialized)
  {
    os_handle_bad_error(err_get_last_error());
  }
  Thread_Context tctx = {0};
  thread_context_init(&tctx);
  os_thread_context_set(&tctx);
  
  gfx_init();
  
  //- gl init
  B32 vulkan_initialized = gfx_vulkan_init();
  if (!vulkan_initialized)
  {
    os_handle_bad_error(err_get_last_error());
  } 
  
  GFX_Window windows[32];
  
  for (U32Each(tick, 1000000000000))
  {
    //- window creation
    for (EachElement(GFX_Window, wdw, windows))
    {
      *wdw = gfx_window_create();
      if (*wdw == 0)
      {
        os_handle_bad_error(err_get_last_error());
      }
    }
    
    //- equiping
    for (EachElement(GFX_Window, wdw, windows))
    {
      B32 equipped = gfx_vulkan_equip(*wdw);
      if (!equipped)
      {
        os_handle_bad_error(err_get_last_error());
      }
    }
    
    //- release
    for (EachElement(GFX_Window, wdw, windows))
    {
      gfx_window_destroy(*wdw);
    }
  }
  
  gfx_vulkan_release();
  os_release();
  return 0;
}
