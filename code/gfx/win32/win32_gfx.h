/* date = April 30th 2026 4:57 pm */

#ifndef WIN32_GFX_H
#define WIN32_GFX_H

#ifndef W32_GFX_WINDOW_SLOT_COUNT
#define W32_GFX_WINDOW_SLOT_COUNT 32
#endif

global Arena *w32_event_arena;
global GFX_Event_List w32_event_list;

typedef struct W32_Window W32_Window;
struct W32_Window
{
  struct W32_Window *next;
  HWND hwnd;
  GFX_Window_Release_Func *release;
  GFX_Window_Resize_Update_Func *resize_update_func;
  void *resize_update_data;
};

#endif //WIN32_GFX_H
