global WNDCLASSA w32_window_class = {0};
global W32_Window w32_window_slots[W32_GFX_WINDOW_SLOT_COUNT];
global W32_Window *w32_window_first;

#define w32_gfx_handle_from_slot(slot) (GFX_Window)(((slot) - w32_window_slots) + 1)
#define w32_slot_from_gfx_handle(handle) (W32_Window *)((w32_window_slots + (handle)) - 1)

file_local B32
w32_window_is_valid(W32_Window *window)
{
  B32 result = (w32_window_slots <= window && 
                window < w32_window_slots + ArraySize(w32_window_slots));
  return(result);
}

B32
gfx_init()
{
  // NOTE(erb): one based indexing so the 0 handle is null
  for (EachIdx(window_idx, w32_window_slots))
  {
    W32_Window *slot = w32_window_slots + window_idx;
    slot->next = slot + 1;
  }
  w32_window_first = w32_window_slots;
  return(true);
}

V2f         
gfx_screen_get_cursor_position(void)
{
  V2f result = {0};
  POINT point;
  if (GetCursorPos(&point))
  {
    result = v2f((F32)point.x, (F32)point.y);
  }
  return(result);
}

B32
gfx_window_is_valid(GFX_Window handle)
{
  W32_Window *slot = w32_slot_from_gfx_handle(handle);
  B32 result = w32_window_is_valid(slot);
  return(result);
}

// NOTE(erb): this is debug code
GFX_Key
gfx_key_from_w32_vkey(WPARAM w_param)
{
  function_persist B32 first = true;
  function_persist GFX_Key key_table[256];
  if(first)
  {
    first = false;
    
    for(U32Each(idx, 'Z'-'A'))
    {
      key_table['A'+idx] = idx+GFXKey_A;
    }
    for(U32Each(idx, '9'-'0'))
    {
      key_table['0'+idx] = idx+GFXKey_0;
    }
    for(U32Each(idx, VK_F12-VK_F1))
    {
      key_table[VK_F1+idx] = idx+GFXKey_F1;
    }
    
    key_table[VK_SPACE     ] = GFXKey_Space;
    key_table[VK_OEM_7     ] = GFXKey_Quote;
    key_table[VK_OEM_2     ] = GFXKey_Slash;
    key_table[VK_OEM_1     ] = GFXKey_Semicolon;
    key_table[VK_OEM_COMMA ] = GFXKey_Comma;
    key_table[VK_OEM_MINUS ] = GFXKey_Minus;
    key_table[VK_OEM_PLUS  ] = GFXKey_Equal;
    key_table[VK_OEM_PERIOD] = GFXKey_Period;
    key_table[VK_OEM_4 ] = GFXKey_LeftBracket;
    key_table[VK_OEM_6 ] = GFXKey_RightBracket;
    key_table[VK_OEM_5 ] = GFXKey_Backslash;
    key_table[VK_OEM_3 ] = GFXKey_Backtick;
    key_table[VK_ESCAPE] = GFXKey_Escape;
    key_table[VK_INSERT] = GFXKey_Insert;
    key_table[VK_DELETE] = GFXKey_Delete;
    key_table[VK_RETURN] = GFXKey_Enter;
    key_table[VK_TAB   ] = GFXKey_Tab;
    key_table[VK_BACK  ] = GFXKey_Backspace;
    
    key_table[VK_RIGHT] = GFXKey_Right;
    key_table[VK_LEFT ] = GFXKey_Left;
    key_table[VK_DOWN ] = GFXKey_Down;
    key_table[VK_UP   ] = GFXKey_Up;
    
    key_table[VK_CONTROL ] = GFXKey_Control;
    key_table[VK_LCONTROL] = GFXKey_Control;
    key_table[VK_RCONTROL] = GFXKey_Control;
    key_table[VK_SHIFT   ] = GFXKey_Shift;
    key_table[VK_LSHIFT  ] = GFXKey_Shift;
    key_table[VK_RSHIFT  ] = GFXKey_Shift;
    key_table[VK_MENU    ] = GFXKey_Alt;
    key_table[VK_LMENU   ] = GFXKey_Alt;
    key_table[VK_RMENU   ] = GFXKey_Alt;
  }
  
  Assert(w_param > 0 && w_param < ArraySize(key_table));
  GFX_Key result = key_table[w_param];
  return(result);
}

GFX_Window
gfx_handle_from_w32_hwnd(HWND hwnd)
{
  Assert(hwnd != INVALID_HANDLE_VALUE);
  GFX_Window result = 0;
  for(EachElement(W32_Window, slot, w32_window_slots))
  {
    if(slot->hwnd == hwnd)
    {
      result = w32_gfx_handle_from_slot(slot);
      break;
    }
  }
  Assert(result != 0);
  return(result);
}

GFX_Modifiers
gfx_get_modifiers(void)
{
  GFX_Modifiers result = 0;
  if(GetKeyState(VK_MENU) & bit_16)
  {
    result |= GFXModifiers_Alt;
  }
  if(GetKeyState(VK_SHIFT) & bit_16)
  {
    result |= GFXModifiers_Shift;
  }
  if(GetKeyState(VK_CONTROL) & bit_16)
  {
    result |= GFXModifiers_Control;
  }
  return(result);
}

GFX_Event *
w32_gfx_push_event(GFX_Event_Kind kind, HWND hwnd)
{
  GFX_Event *event = gfx_push_event(w32_event_arena, &w32_event_list, kind);
  event->modifiers = gfx_get_modifiers();
  event->window = gfx_handle_from_w32_hwnd(hwnd);
  return(event);
}


GFX_Event_List
gfx_poll_events(Arena *arena)
{
  w32_event_arena = arena;
  ZeroStruct(&w32_event_list);
  
  MSG message = {0};
  while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
  {
    TranslateMessage(&message);
    DispatchMessage(&message);
    if (message.message == WM_QUIT) 
    {
      w32_gfx_push_event(GFXEventKind_WindowClose, message.hwnd);
    }
  }
  return(w32_event_list);
}

void
gfx_window_set_resize_update_func(GFX_Window handle, GFX_Window_Resize_Update_Func *func)
{
  Assert(func);
  if(gfx_window_is_valid(handle))
  {
    W32_Window *slot = w32_slot_from_gfx_handle(handle);
    Assert(slot);
    slot->resize_update_func = func;
  }
  else
  {
    InvalidCodePath;
  }
}

void
gfx_window_set_resize_update_data(GFX_Window handle, void *data)
{
  Assert(data);
  if(gfx_window_is_valid(handle))
  {
    W32_Window *slot = w32_slot_from_gfx_handle(handle);
    Assert(slot);
    slot->resize_update_data = data;
  }
  else
  {
    InvalidCodePath;
  }
}

LRESULT CALLBACK
w32_window_proc(HWND hwnd, UINT msg_kind, WPARAM w_param, LPARAM l_param)
{
  LRESULT result = 0;
  if(w32_event_arena == 0)
  {
    result = DefWindowProc(hwnd, msg_kind, w_param, l_param);
  }
  else
  {
    switch(msg_kind)
    {
      default: 
      {
        result = DefWindowProc(hwnd, msg_kind, w_param, l_param);
      } break;
      
      case WM_SIZE:
      case WM_PAINT:
      {
        PAINTSTRUCT paint;
        BeginPaint(hwnd, &paint);
        GFX_Window handle = gfx_handle_from_w32_hwnd(hwnd);
        if(gfx_window_is_valid(handle))
        {
          W32_Window *slot = w32_slot_from_gfx_handle(handle);
          if(slot->resize_update_func)
          {
            slot->resize_update_func(slot->resize_update_data);
          }
        }
        EndPaint(hwnd, &paint);
      } break;
      
      case WM_CLOSE:
      {
        w32_gfx_push_event(GFXEventKind_WindowClose, hwnd);
      } break;
      
      case WM_KILLFOCUS:
      {
        w32_gfx_push_event(GFXEventKind_WindowLoseFocus, hwnd);
      } break;
      
      case WM_LBUTTONDOWN: case WM_LBUTTONUP:
      case WM_MBUTTONDOWN: case WM_MBUTTONUP:
      case WM_RBUTTONDOWN: case WM_RBUTTONUP:
      {
        B32 release = (msg_kind == WM_LBUTTONUP ||
                       msg_kind == WM_MBUTTONUP ||
                       msg_kind == WM_RBUTTONUP);
        
        GFX_Mouse_Code mouse_code = GFXMouseCode_None;
        switch(msg_kind)
        {
          case WM_LBUTTONDOWN: 
          case WM_LBUTTONUP:
          {
            mouse_code = GFXMouseCode_Left;
          } break;
          
          case WM_MBUTTONDOWN: case WM_MBUTTONUP:
          {
            mouse_code = GFXMouseCode_Middle;
          } break;
          
          case WM_RBUTTONDOWN: case WM_RBUTTONUP:
          {
            mouse_code = GFXMouseCode_Right;
          } break;
        }
        
        if(release)
        {
          ReleaseCapture();
        }
        else
        {
          SetCapture(hwnd);
        }
        
        GFX_Event *event = w32_gfx_push_event(release ? GFXEventKind_MouseUp : GFXEventKind_MouseDown, hwnd);
        event->mouse.code = mouse_code;
        event->mouse.position.x = (F32)(I16)LOWORD(l_param);
        event->mouse.position.y = (F32)(I16)HIWORD(l_param);
      } break;
      
      case WM_MOUSEMOVE:
      {
        GFX_Event *event = w32_gfx_push_event(GFXEventKind_MouseMove, hwnd);
        event->mouse.position.x = (F32)(I16)LOWORD(l_param);
        event->mouse.position.y = (F32)(I16)HIWORD(l_param);
      } break;
      
      case WM_MOUSEWHEEL:
      case WM_MOUSEHWHEEL:
      {
        POINT p = {0};
        p.x = (I32)(I16)LOWORD(l_param);
        p.y = (I32)(I16)HIWORD(l_param);
        ScreenToClient(hwnd, &p);
        
        I32 wheel_delta = GET_WHEEL_DELTA_WPARAM(w_param);
        I32 click = wheel_delta/120;
        
        GFX_Event *event = w32_gfx_push_event(GFXEventKind_MouseWheel, hwnd);
        event->mouse.position = v2f((F32)p.x, (F32)p.y);
        
        Axis2 axis = (msg_kind == WM_MOUSEWHEEL) ? Axis2_Y : Axis2_X;
        event->mouse.wheel.c[axis] = (F32)wheel_delta;
        event->mouse.wheel_clicks.c[axis] = click;
      } break;
      
      case WM_SYSKEYDOWN: case WM_SYSKEYUP:
      case WM_KEYDOWN: case WM_KEYUP:
      {
        B32 is_down = !(l_param & bit_32);
        U32 repeat_count = (l_param & bit_mask_16);
        
        GFX_Event *event = w32_gfx_push_event(is_down?GFXEventKind_KeyDown:GFXEventKind_KeyUp, hwnd);
        event->key.code = gfx_key_from_w32_vkey(w_param);
        event->key.repeat_count = repeat_count;
        
        // NOTE(erb): enforce modifiers
        if(event->key.code == GFXKey_Alt)
        {
          event->modifiers &= ~GFXModifiers_Alt;
        }
        else if(event->key.code == GFXKey_Control)
        {
          event->modifiers &= ~GFXModifiers_Control;
        }
        else if(event->key.code == GFXKey_Shift)
        {
          event->modifiers &= ~GFXModifiers_Shift;
        }
      } break;
      
      case WM_SYSCHAR: case WM_CHAR:
      { 
        B32 is_down = !(l_param & bit_32);
        if(is_down)
        {
          U32 repeat_count = (l_param & bit_mask_16);
          
          GFX_Event *event = w32_gfx_push_event(GFXEventKind_Text, hwnd);
          event->key.codepoint = (I32)w_param;
          event->key.repeat_count = repeat_count;
        }
      } break;
    }
  }
  return(result);
}

char *
w32_get_registered_window_class_name()
{
  char *result = (char *)w32_window_class.lpszClassName;
  if (!result)
  {
    HINSTANCE instance = w32_state.instance;
    
    ZeroStruct(&w32_window_class);
    w32_window_class.hInstance = instance;
    w32_window_class.lpfnWndProc = w32_window_proc;
    w32_window_class.lpszClassName = "window_class";
    ATOM atom = RegisterClass(&w32_window_class);
    if (atom != 0) 
    {
      result = (char *)w32_window_class.lpszClassName;
    }
  }
  return result;
}

GFX_Window 
gfx_window_create(void)
{
  HINSTANCE instance = w32_state.instance;
  String8 error = {0};
  
  W32_Window *slot = w32_window_first;
  if (!w32_window_is_valid(slot))
  {
    error = S8("ran out of win32 window slots");
  }
  
  //- create win32 window class
  char *w32_window_class_name = w32_get_registered_window_class_name();
  if (!w32_window_class_name)
  {
    error = S8("failed to get registered window");
  }
  
  //- create win32 window
  HWND window = 0;
  if (error.value == 0)
  {
    window = CreateWindowA(w32_window_class_name,
                           "TITLE ME",
                           WS_TILEDWINDOW, 
                           CW_USEDEFAULT, CW_USEDEFAULT, // x, y
                           CW_USEDEFAULT, CW_USEDEFAULT, // w, h
                           0, 0, instance, 0); // parent, menu, instance, param
    if (window == INVALID_HANDLE_VALUE) 
    {
      error = S8("failed to create window");
    }
  }
  
  GFX_Window result = 0;
  if (error.value != 0)
  {
    if(window != 0)
    {
      BOOL destoyed_window = DestroyWindow(window);
      Assert(destoyed_window);
    }
    err_set_error(error);
  }
  else
  {
    SLLStackPop(w32_window_first);
    slot->hwnd = window;
    result = w32_gfx_handle_from_slot(slot);
  }
  return(result);
}

B32
gfx_window_destroy(GFX_Window handle)
{
  String8 error = {0};
  if (!gfx_window_is_valid(handle))
  {
    error = S8("invalid win32 window handle");
  }
  
  HWND hwnd = 0;
  if (error.value == 0)
  {
    W32_Window *slot = w32_slot_from_gfx_handle(handle);
    hwnd = slot->hwnd;
    if (slot->release != 0)
    {
      B32 released = slot->release(handle);
      if (!released)
      {
        error = err_get_last_error();
      }
    }
    SLLStackPush(w32_window_first, slot);
  }
  
  BOOL destoyed_window = DestroyWindow(hwnd);
  if (!destoyed_window)
  {
    error = S8("failed to destory window");
  }
  
  if (error.value != 0)
  {
    err_set_error(error);
  }
  B32 result = (error.value == 0);
  return(result);
}

V2i         
gfx_window_get_client_size(GFX_Window handle)
{
  V2i result = {0};
  if (gfx_window_is_valid(handle))
  {
    W32_Window *slot = w32_slot_from_gfx_handle(handle);
    RECT rect;
    if (GetClientRect(slot->hwnd, &rect))
    {
      result = v2i((I32)(rect.right - rect.left),
                   (I32)(rect.bottom - rect.top));
    }
  }
  return(result);
}


void
gfx_window_set_size(GFX_Window handle, V2i size)
{
  Assert(size.x > 0 && size.y > 0 && "size must be positive");
  if (gfx_window_is_valid(handle))
  {
    W32_Window *slot = w32_slot_from_gfx_handle(handle);
    SetWindowPos(slot->hwnd, 
                 0, 0, 0, //hWndInsertAfter, X position, Y  position
                 size.x, size.y,
                 SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOZORDER);
  }
}

void
gfx_window_set_position(GFX_Window handle, V2i position)
{
  if (gfx_window_is_valid(handle))
  {
    W32_Window *slot = w32_slot_from_gfx_handle(handle);
    SetWindowPos(slot->hwnd, 
                 0, //hWndInsertAfter
                 position.x, position.y,
                 0, 0, //width, height
                 SWP_NOSIZE|SWP_NOOWNERZORDER|SWP_NOZORDER
                 );
  }
}

void
gfx_window_show(GFX_Window handle)
{
  if (gfx_window_is_valid(handle))
  {
    W32_Window *slot = w32_slot_from_gfx_handle(handle);
    ShowWindow(slot->hwnd, SW_SHOWDEFAULT);
    EnableWindow(slot->hwnd, TRUE);
  }
}

void 
gfx_window_set_title(GFX_Window handle, String8 text)
{
  if (gfx_window_is_valid(handle))
  {
    W32_Window *slot = w32_slot_from_gfx_handle(handle);
    Scratch scratch = scratch_begin(0, 0);
    char *cstr = push_cstr(scratch.arena, text);
    SetWindowTextA(slot->hwnd, cstr);
    scratch_end(&scratch);
  }
}

V2f         
gfx_window_get_cursor_position(GFX_Window handle)
{
  V2f result = {0};
  if (gfx_window_is_valid(handle))
  {
    W32_Window *slot = w32_slot_from_gfx_handle(handle);
    
    POINT point;
    if (GetCursorPos(&point))
    {
      if (ScreenToClient(slot->hwnd, &point))
      {
        result = v2f((F32)point.x, (F32)point.y);
      }
    }
  }
  return(result);
}

char *w32_cursor_from_gfx_table[] =
{
  [GFXCursorKind_None                   ] = IDC_ARROW,
  [GFXCursorKind_Arrow                  ] = IDC_ARROW,
  [GFXCursorKind_Beam                   ] = IDC_IBEAM,
  [GFXCursorKind_Hand                   ] = IDC_HAND,
  [GFXCursorKind_Cross                  ] = IDC_CROSS,
  [GFXCursorKind_Wait                   ] = IDC_WAIT,
  [GFXCursorKind_ArrowNS         ] = IDC_SIZENS,
  [GFXCursorKind_ArrowWE         ] = IDC_SIZEWE,
  [GFXCursorKind_ArrowNWSE] = IDC_SIZENWSE,
  [GFXCursorKind_ArrowSWNE] = IDC_SIZENESW,
  [GFXCursorKind_ArrowAll               ] = IDC_SIZEALL,
};

char *
w32_cursor_from_gfx(GFX_Cursor_Kind kind)
{
  Assert(ArraySize(w32_cursor_from_gfx_table) == GFXCursorKind_COUNT);
  Assert(kind >= 0 && kind <= ArraySize(w32_cursor_from_gfx_table));
  char *result = w32_cursor_from_gfx_table[(U32)kind];
  return(result);
}

void
gfx_set_cursor(GFX_Cursor_Kind kind)
{
  if(kind != GFXCursorKind_None)
  {
    HCURSOR cursor = LoadCursorA(0, w32_cursor_from_gfx(kind));
    Assert(cursor != 0);
    SetCursor(cursor);
  }
}

String8
gfx_get_clipboard_str8(Arena *arena)
{
  Assert(IsClipboardFormatAvailable(CF_TEXT));
  
  String8 result = {0};
  BOOL opened = OpenClipboard(0);
  if(opened)
  {
    HANDLE data = GetClipboardData(CF_TEXT);
    Assert(data != 0);
    
    char *memory = GlobalLock(data);
    Assert(memory != 0);
    
    result = push_str8_from_cstr(arena, memory);
    
    GlobalUnlock(data);
    BOOL closed = CloseClipboard();
    Assert(closed);
  }
  else
  {
    InvalidCodePath;
  }
  return(result);
}

void
gfx_set_clipboard_str8(String8 str)
{
  Assert(IsClipboardFormatAvailable(CF_TEXT));
  BOOL opened = OpenClipboard(0);
  if(opened)
  {
    // TODO(erb): validate format with '\r\n' endings
    // NOTE(erb): copy the string into a global memory buffer
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, (str.size+1));
    char *copy = GlobalLock(memory);
    if(copy == 0)
    {
      CloseClipboard();
      InvalidCodePath;
    }
    ArrayCopy(str.value, copy, str.size);
    copy[str.size] = 0;
    BOOL still_locked = GlobalUnlock(memory);
    Assert(still_locked == 0);
    
    // NOTE(erb): set the data
    EmptyClipboard();
    SetClipboardData(CF_TEXT, memory);
    BOOL closed = CloseClipboard();
    Assert(closed);
  }
  else
  {
    InvalidCodePath;
  }
}
