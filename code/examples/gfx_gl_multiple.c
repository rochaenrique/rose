#include <math.h>

// headers
#include "base/base.h"
#include "gfx/gfx.h"

// implementation
#include "base/base.c"
#include "gfx/gfx.c"

OS_ENTRY_POINT()
{
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
  B32 gl_initialized = gfx_gl_init();
  if (!gl_initialized)
  {
    os_handle_bad_error(err_get_last_error());
  }
  
  //- create windows
  GFX_Window windows[32];
  U32 window_count = ArraySize(windows);
  for (EachElement(GFX_Window, wdw, windows))
  {
    *wdw = gfx_window_create();
    if (*wdw == 0)
    {
      os_handle_bad_error(err_get_last_error());
    }
    Scratch scratch = scratch_begin(0, 0);
    gfx_window_set_title(*wdw, push_str8f(scratch.arena, "GL (%d)", (I32)(wdw - windows)));
    scratch_end(&scratch);
  }
  
  //- equiping
  // TODO(erb): check if the window is allready equiped!
  for (EachU32(idx, window_count))
  {
    B32 equipped = gfx_gl_equip(windows[idx]);
    if (!equipped)
    {
      os_handle_bad_error(err_get_last_error());
    }
  }
  
  //- showing
  for (EachU32(idx, window_count))
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
        int x = 0;
        SupressUnused(x);
        for(EachU32(idx, window_count))
        {
          if(windows[idx] == event->window)
          {
            window_count--;
            Swap(GFX_Window, windows[window_count], windows[idx]);
            windows[window_count] = 0;
            break;
          }
        }
        gfx_window_destroy(event->window);
      }
    }
    
    if(window_count == 0)
    {
      break;
    }
    
    for (EachU32(idx, window_count))
    {
      GFX_Window window = windows[idx];
      F32 t = sinf(tick/(100 * (idx + 1)));
      V4f clear_color = v4f_lerp(color_red, color_green, t);
      gfx_gl_render_begin(window);
      {
        glClearColor(V4Expand(clear_color));
        glClear(GL_COLOR_BUFFER_BIT);
      }
      gfx_gl_render_end(window);
    }
    
    scratch_end(&frame_scratch);
  }
  
  //- release
  for (EachU32(idx, window_count))
  {
    gfx_window_destroy(windows[idx]);
  }
  os_release();
  return 0;
}
