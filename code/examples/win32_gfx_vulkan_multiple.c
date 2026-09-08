#include <math.h>

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
  
  //- vulkan
  B32 vulkan_initialized = gfx_vulkan_init();
  if (!vulkan_initialized)
  {
    os_handle_bad_error(err_get_last_error());
  }
  
  GFX_Window windows[32];
  U32 window_count = ArraySize(windows);
  
  //- window creation
  for (EachElement(GFX_Window, wdw, windows))
  {
    *wdw = gfx_window_create();
    if (*wdw == 0)
    {
      os_handle_bad_error(err_get_last_error());
    }
    Scratch scratch = scratch_begin(0, 0);
    gfx_window_set_title(*wdw, push_str8f(scratch.arena, "GL (%d)\n", (I32)(wdw - windows)));
    scratch_end(&scratch);
  }
  
  //- equiping
  // TODO(erb): check if the window is allready equiped!
  for (U32Each(idx, window_count))
  {
    B32 equipped = gfx_vulkan_equip(windows[idx]);
    if (!equipped)
    {
      os_handle_bad_error(err_get_last_error());
    }
  }
  
  //- showing
  for (U32Each(idx, window_count))
  {
    gfx_window_show(windows[idx]);
  }
  
  //- update loop
  for (F32 tick = 0;;++tick) 
  {
    Scratch frame_scratch = scratch_begin(0, 0);
    
    GFX_Event_List events = gfx_poll_events(frame_scratch.arena);
    for (EachNode(GFX_Event, event, events.first))
    {
      if(event->kind == GFXEventKind_WindowClose)
      {
        // NOTE(erb): array removal
        for(U32Each(idx, window_count))
        {
          if(windows[idx] == event->window)
          {
            Swap(GFX_Window, windows[window_count-1], windows[idx]);
            windows[window_count--] = 0;
            break;
          }
        }
        gfx_window_destroy(event->window);
      }
    }
    
    if (window_count == 0) 
    {
      break;
    }
    
    for (U32Each(idx, window_count))
    {
      GFX_Window window = windows[idx];
      gfx_vulkan_render_begin(window);
      {
      }
      gfx_vulkan_render_end(window);
    }
    
    scratch_end(&frame_scratch);
  }
  
  //- release
  for (EachElement(GFX_Window, wdw, windows))
  {
    gfx_window_destroy(*wdw);
  }
  os_release();
  return 0;
}
