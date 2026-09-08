/* date = April 30th 2026 4:57 pm */

#include <X11/Xlib.h>
#include <X11/Xutil.h>

// NOTE(erb): X11 C Interface
// https://www.x.org/releases/current/doc/libX11/libX11/libX11.html

#ifndef LINUX_GFX_H
#define LINUX_GFX_H

#ifndef LNX_GFX_WINDOW_SLOT_COUNT
#define LNX_GFX_WINDOW_SLOT_COUNT 32
#endif

typedef struct LNX_GFX_State LNX_GFX_State;
struct LNX_GFX_State
{
  Display *display;
  Screen *screen;
  I32 screen_id;
  XVisualInfo visual_info;
};

typedef struct LNX_Window LNX_Window;
struct LNX_Window
{
  struct LNX_Window *next;
  Window x11_window;
  Atom atom_delete_window;
  GFX_Window_Release_Func *release;
};

#endif //LINUX_GFX_H
