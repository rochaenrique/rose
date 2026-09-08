#include <stdio.h>
#include <math.h>

#undef Assert
#include "../base/base.h"
#include "../gfx/gfx.h"

#include "../base/base.c"
#include "../gfx/gfx.c"

int
main()
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
  
  for (U32Each(tick, 100000000))
  {
    GFX_Window windows[31];
    
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
      B32 equipped = gfx_gl_equip(*wdw);
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
  
  os_release();
  return 0;
}
