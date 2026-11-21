global LNX_GFX_State lnx_gfx_state;
global LNX_Window lnx_window_slots[LNX_GFX_WINDOW_SLOT_COUNT];
global LNX_Window *lnx_window_first;

#define lnx_gfx_handle_from_slot(slot) (GFX_Window)(((slot) - lnx_window_slots) + 1)
#define lnx_slot_from_gfx_handle(handle) (LNX_Window *)((lnx_window_slots + (handle)) - 1)

file_local B32
lnx_window_is_valid(LNX_Window *window)
{
  B32 result = (lnx_window_slots <= window && 
                window < lnx_window_slots + ArraySize(lnx_window_slots));
  return(result);
}

file_local int
x11_error_handler(Display *display, XErrorEvent *error)
{
  // TODO(erb): make this a real implementation
  Scratch scratch = scratch_begin(0, 0);
  I32 size = 512;
  char *buffer = PushArray(scratch.arena, size, char);
  XGetErrorText(display, error->error_code, buffer, size);
  os_logf("[X11 ERROR]: %s\n", buffer);
  Assert(false);
  scratch_end(&scratch);
  return(0);
}

B32
gfx_init()
{
  //- setup window slots
  {
    for (U32 window_idx = 0; 
         window_idx < ArraySize(lnx_window_slots);
         ++window_idx)
    {
      LNX_Window *slot = lnx_window_slots + window_idx;
      slot->next = slot + 1;
    }
    lnx_window_first = lnx_window_slots;
  }
  
  String8 error = {0};
  
  //- set the error handler
  XSetErrorHandler(x11_error_handler);
  
  //- open display
  Display *display = XOpenDisplay(0);
	if (display == 0) 
  {
    error = S8("failed to open X11 display\n");
	}
  
#if DEBUG
  if (error.value == 0) 
  {
    XSynchronize(display, True);
  }
#endif
  
  //- get screen
  Screen *screen = 0; 
  I32 screen_id = 0;
  if (error.value == 0)
  {
    screen = DefaultScreenOfDisplay(display);
    if (screen == 0) 
    {
      error = S8("failed to get X11 screen\n");
    }
    else
    {
      screen_id = DefaultScreen(display);
    }
  }
  
  B32 result = false;
  if (error.value == 0) 
  {
    ZeroStruct(&lnx_gfx_state);
    lnx_gfx_state.display = display;
    lnx_gfx_state.screen = screen;
    lnx_gfx_state.screen_id = screen_id;
    
    // TODO(erb): have a way to initialize this structure to some default values.
    // this will later be populated by the gfx_*_equip functions
    XVisualInfo visual_info = {0};
    lnx_gfx_state.visual_info = visual_info;
    result = true;
  }
  else
  {
    if (screen) 
    {
      XFree(screen);
    }
    if (display) 
    {
      XCloseDisplay(display);
    }
    err_set_error(error);
  }
  return(result);
}

GFX_Window
gfx_handle_from_x11_window(Window x11_window)
{
  Assert(x11_window != 0);
  GFX_Window result = 0;
  for(EachElement(LNX_Window, slot, lnx_window_slots))
  {
    if(slot->x11_window == x11_window)
    {
      result = lnx_gfx_handle_from_slot(slot);
      break;
    }
  }
  Assert(result != 0);
  return(result);
}

B32
lnx_is_key_down_keymap(char* keys, KeySym sym)
{
  KeyCode code = XKeysymToKeycode(lnx_gfx_state.display, sym);
  B32 result = keys[code/8] & (1 << (code%8));
  return(result);
}

GFX_Modifiers
gfx_get_modifiers()
{
  char keys_return[32];
  XQueryKeymap(lnx_gfx_state.display, keys_return);
  
  GFX_Modifiers result = 0;
  if(lnx_is_key_down_keymap(keys_return, XK_Shift_L) || 
     lnx_is_key_down_keymap(keys_return, XK_Shift_R))
  {
    result |= GFXModifiers_Shift;
  }
  if(lnx_is_key_down_keymap(keys_return, XK_Alt_L) || 
     lnx_is_key_down_keymap(keys_return, XK_Alt_R))
  {
    result |= GFXModifiers_Alt;
  }
  if(lnx_is_key_down_keymap(keys_return, XK_Control_L) || 
     lnx_is_key_down_keymap(keys_return, XK_Control_R))
  {
    result |= GFXModifiers_Shift;
  }
  return(result);
}

GFX_Event *
lnx_gfx_push_event(Arena *arena, GFX_Event_List *list, GFX_Event_Kind kind, Window x11_window)
{
  GFX_Event *event = gfx_push_event(arena, list, kind);
  event->modifiers = gfx_get_modifiers();
  event->window = gfx_handle_from_x11_window(x11_window);
  return(event);
}

GFX_Key
gfx_key_from_x11_keysym(KeySym key_sym)
{
  function_persist B32 first = false;
  function_persist GFX_Key key_table[512];
  
  if(!first)
  {
    key_table[XK_bracketleft] = GFXKey_LeftBracket;
    key_table[XK_backslash] = GFXKey_Backslash;
    key_table[XK_grave] = GFXKey_Backtick;
    key_table[XK_comma] = GFXKey_Comma;
    key_table[XK_semicolon] = GFXKey_Semicolon;
    key_table[XK_minus] = GFXKey_Minus;
    key_table[XK_space] = GFXKey_Space;
    key_table[XK_slash] = GFXKey_Slash;
    key_table[XK_plus] = GFXKey_Equal;
    
    // NOTE(erb): these values are greater than 256
#define KM(idx, value) key_table[(idx) % ArraySize(key_table)] = (value);
    
    for(EachU32(idx, 12))
    {
      KM(XK_F1+idx, idx+GFXKey_F1);
    }
    for(EachU32(idx, 9))
    {
      KM(XK_0+idx, idx+GFXKey_0);
    }
    for(EachU32(idx, 'z'-'a'))
    {
      KM(XK_a+idx, idx+GFXKey_A);
      KM(XK_A+idx, idx+GFXKey_A);
    }
    
    KM(XK_BackSpace, GFXKey_Backspace);
    KM(XK_Tab, GFXKey_Tab);
    KM(XK_Return, GFXKey_Enter);
    KM(XK_Escape, GFXKey_Escape);
    KM(XK_Delete, GFXKey_Delete);
    KM(XK_Insert, GFXKey_Insert);
    
    KM(XK_Shift_L, GFXKey_Shift);
    KM(XK_Shift_R, GFXKey_Shift);
    KM(XK_Control_L, GFXKey_Control);
    KM(XK_Control_R, GFXKey_Control);
    KM(XK_Alt_L, GFXKey_Alt);
    KM(XK_Alt_R, GFXKey_Alt);
    
    KM(XK_Up, GFXKey_Up);
    KM(XK_Down, GFXKey_Down);
    KM(XK_Left, GFXKey_Left);
    KM(XK_Right, GFXKey_Right);
#undef KM
  }
  
  U32 idx = key_sym % ArraySize(key_table);
  GFX_Key result = key_table[idx];
  return(result);
}

GFX_Event_List
gfx_poll_events(Arena *arena)
{
  GFX_Event_List event_list = {0};
  
  while (XPending(lnx_gfx_state.display) > 0) 
  {
    XEvent any_event;
    XNextEvent(lnx_gfx_state.display, &any_event);
    switch (any_event.type) 
    {
      case KeyPress: 
      case KeyRelease: 
      {
        XKeyEvent *xkey = &any_event.xkey;
        
        // NOTE(erb): modifiers
        GFX_Modifiers mods = 0;
        if(xkey->state & ShiftMask)
        {
          mods = GFXKey_Alt;
        }
        if(xkey->state & ControlMask)
        {
          mods = GFXKey_Control;
        }
        if((xkey->state & Mod1Mask) || 
           (xkey->state & Mod2Mask) || 
           (xkey->state & Mod3Mask) || 
           (xkey->state & Mod4Mask) || 
           (xkey->state & Mod5Mask))
        {
          mods = GFXKey_Alt;
        }
        
        GFX_Event_Kind k = xkey->type==KeyPress ? GFXEventKind_KeyDown : GFXEventKind_KeyUp;
        GFX_Event *event = lnx_gfx_push_event(arena, &event_list, k, xkey->window);
        event->key.code = gfx_key_from_x11_keysym(XLookupKeysym(xkey, 0));
        event->modifiers = mods;
        // TODO(erb): figure out repeat count
        event->key.repeat_count = 0;
        
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
      
      case ButtonPress:
      case ButtonRelease:
      {
        XButtonEvent *xbutton = &any_event.xbutton;
        
        GFX_Event_Kind kind = GFXEventKind_None;
        V2f wheel = {0};
        GFX_Mouse_Code mouse_code = GFXMouseCode_None;
        
        // NOTE(erb): Button4 and Button5 are scroll events, TERRIBLE API DESIGN
        if(xbutton->button == Button1 ||
           xbutton->button == Button2 ||
           xbutton->button == Button3)
        {
          switch(xbutton->button)
          {
            case Button1:
            {
              mouse_code = GFXMouseCode_Left;
            } break;
            
            case Button2:
            {
              mouse_code = GFXMouseCode_Middle;
            } break;
            
            case Button3:
            {
              mouse_code = GFXMouseCode_Right;
            } break;
          }
          
          kind = xbutton->type==ButtonPress ? GFXEventKind_MouseDown : GFXEventKind_MouseUp;
        }
        else if (xbutton->button == Button4 ||
                 xbutton->button == Button5)
        {
          kind = GFXEventKind_MouseWheel;
          wheel.y = xbutton->button==Button4 ? 1 : -1;
        }
        else
        {
          InvalidCodePath;
        }
        
        GFX_Event *event = lnx_gfx_push_event(arena, &event_list, kind, xbutton->window);
        event->mouse.position = v2f((F32)xbutton->x, (F32)xbutton->y);
        event->mouse.code = mouse_code;
        event->mouse.wheel = wheel;
      } break;
      
      case FocusOut: // NOTE(erb): there is also a FocusIn
      {
        lnx_gfx_push_event(arena, &event_list, GFXEventKind_WindowLoseFocus, any_event.xany.window);
      } break;
      
      case ClientMessage: 
      {
        GFX_Window window = 0;
        Atom atom = (Atom)any_event.xclient.data.l[0];
        for(EachElement(LNX_Window, slot, lnx_window_slots))
        {
          if(slot->atom_delete_window == atom)
          {
            window = lnx_gfx_handle_from_slot(slot);
            break;
          }
        }
        if(window != 0)
        {
          GFX_Event *event = gfx_push_event(arena, &event_list, GFXEventKind_WindowClose);
          event->modifiers = gfx_get_modifiers();
          event->window = window;
        }
      } break;
      
      case MotionNotify:
      {
        XMotionEvent *xmotion = &any_event.xmotion;
        GFX_Event *event = lnx_gfx_push_event(arena, &event_list, GFXEventKind_MouseMove, xmotion->window);
        event->mouse.position = v2f((F32)xmotion->x, (F32)xmotion->y);
      } break;
      
    }
  }
  return(event_list);
}

V2f
gfx_screen_get_cursor_position(void)
{
  B32 got_result = false;
  V2f result = {0};
  
  U32 screen_count = XScreenCount(lnx_gfx_state.display);
  for (EachU32(idx, screen_count))
  {
    Window window = XRootWindow(lnx_gfx_state.display, idx);
    
    Window root_return, child_return;
    I32 root_x_return, root_y_return;
    I32 win_x_return, win_y_return;
    U32 mask_return;
    Bool query_result = XQueryPointer(lnx_gfx_state.display, window, &root_return, &child_return,
                                      &root_x_return, &root_y_return, &win_x_return, &win_y_return, 
                                      &mask_return);
    if (query_result)
    {
      got_result = true;
      result = v2f((F32)root_x_return, (F32)root_y_return);
      break;
    }
  }
  Assert(got_result);
  return(result);
}

B32
gfx_window_is_valid(GFX_Window handle)
{
  LNX_Window *slot = lnx_slot_from_gfx_handle(handle);
  B32 result = lnx_window_is_valid(slot);
  return(result);
}

GFX_Window
gfx_window_create(void)
{
  String8 error = {0};
  
  LNX_Window *slot = lnx_window_first;
  if (!lnx_window_is_valid(slot)) 
  {
    error = S8("ran out of linux window slots");
  }
  
	//- open window
  XSetWindowAttributes window_attributes = {0};
  Window window = {0};
  if (error.value == 0)
  {
    //- attributes
    window_attributes.border_pixel = BlackPixel(lnx_gfx_state.display, lnx_gfx_state.screen_id);
    window_attributes.background_pixel = WhitePixel(lnx_gfx_state.display, lnx_gfx_state.screen_id);
    window_attributes.override_redirect = True;
    // TODO(erb): this might need to have ExposureMask
    window_attributes.event_mask = (FocusChangeMask|
                                    KeyPressMask|KeyReleaseMask|
                                    ButtonPressMask|ButtonReleaseMask|
                                    EnterWindowMask|LeaveWindowMask|
                                    PointerMotionMask|PointerMotionHintMask|
                                    Button1MotionMask|Button2MotionMask|Button3MotionMask|Button4MotionMask|Button5MotionMask|
                                    ButtonMotionMask|
                                    ExposureMask);
    window_attributes.colormap = XCreateColormap(lnx_gfx_state.display, 
                                                 RootWindow(lnx_gfx_state.display, lnx_gfx_state.screen_id), 
                                                 lnx_gfx_state.visual_info.visual, AllocNone);
    
    U64 value_mask = CWBackPixel|CWColormap|CWBorderPixel|CWEventMask;
    window = XCreateWindow(lnx_gfx_state.display, 
                           RootWindow(lnx_gfx_state.display, lnx_gfx_state.screen_id), 
                           0, 0, 960, 540, 1,  //x, y, width, height, border_width
                           lnx_gfx_state.visual_info.depth, InputOutput, lnx_gfx_state.visual_info.visual,
                           value_mask, &window_attributes);
    if (window == 0) 
    {
      error = S8("failed to create window");
    }
  }
  
  //- redirect window close event
  Atom atom_delete_window = {0};
  if (error.value == 0) 
  {
    atom_delete_window = XInternAtom(lnx_gfx_state.display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(lnx_gfx_state.display, window, &atom_delete_window, 1);
  }
  
  GFX_Window result = 0;
  if (error.value == 0) 
  {
    SLLStackPop(lnx_window_first);
    slot->x11_window = window;
    slot->atom_delete_window = atom_delete_window;
    result = lnx_gfx_handle_from_slot(slot);
  }
  else
  {
    if (window) 
    {
      XDestroyWindow(lnx_gfx_state.display, window);
    }
    if (window_attributes.colormap) 
    {
      XFreeColormap(lnx_gfx_state.display, window_attributes.colormap);
    }
    err_set_error(error);
  }
  return(result);
}

B32
gfx_window_destroy(GFX_Window handle)
{
  String8 error = {0};
  
  if (!gfx_window_is_valid(handle)) 
  {
    error = S8("invalid linux window handle");
  }
  
  Window x11_window = 0;
  if (error.value == 0) 
  {
    LNX_Window *slot = lnx_slot_from_gfx_handle(handle);
    x11_window = slot->x11_window;
    if (slot->release != 0) 
    {
      B32 released = slot->release(handle);
      if (!released) 
      {
        error = err_get_last_error();
      }
    }
    SLLStackPush(lnx_window_first, slot);
  }
  
  XWindowAttributes window_attributes = {0};
  if (error.value == 0) 
  {
    if (XGetWindowAttributes(lnx_gfx_state.display, x11_window, &window_attributes))
    {
      XFreeColormap(lnx_gfx_state.display, window_attributes.colormap);
    }
    else
    {
      error = S8("failed to get x11 window attributes");
    }
  }
  
  if (error.value == 0) 
  {
    XDestroyWindow(lnx_gfx_state.display, x11_window);
  }
  else
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
    LNX_Window *slot = lnx_slot_from_gfx_handle(handle);
    
    Window root_return;
    I32 x_return, y_return;
    U32 width_return, height_return, border_width_return, depth_return;
    
    Status geometry_result = XGetGeometry(lnx_gfx_state.display, slot->x11_window, &root_return, 
                                          &x_return, &y_return, &width_return, &height_return, 
                                          &border_width_return, &depth_return);
    Assert(geometry_result != 0);
    if(geometry_result != 0)
    {
      result = v2i(width_return, height_return);
    }
  }
  return(result);
}

void
gfx_window_set_size(GFX_Window handle, V2i size)
{
  // TODO(erb): create a V2u (unsigned integer vector) type
  Assert(size.x > 0 && size.y > 0 && "Size must be positive");
  if (gfx_window_is_valid(handle)) 
  {
    LNX_Window *slot = lnx_slot_from_gfx_handle(handle);
    XResizeWindow(lnx_gfx_state.display, slot->x11_window, size.x, size.y);
  }
}

void
gfx_window_set_position(GFX_Window handle, V2i position)
{
  if (gfx_window_is_valid(handle)) 
  {
    LNX_Window *slot = lnx_slot_from_gfx_handle(handle);
    XMoveWindow(lnx_gfx_state.display, slot->x11_window, position.x, position.y);
  }
}

void
gfx_window_show(GFX_Window handle)
{
  if (gfx_window_is_valid(handle)) 
  {
    LNX_Window *slot = lnx_slot_from_gfx_handle(handle);
    XClearWindow(lnx_gfx_state.display, slot->x11_window);
    XMapRaised(lnx_gfx_state.display, slot->x11_window);
  }
}

void
gfx_window_set_title(GFX_Window handle, String8 text)
{
  if (gfx_window_is_valid(handle)) 
  {
    LNX_Window *slot = lnx_slot_from_gfx_handle(handle);
    Scratch scratch = scratch_begin(0, 0);
    char *cstr = push_cstr(scratch.arena, text);
    XStoreName(lnx_gfx_state.display, slot->x11_window, cstr);
    scratch_end(&scratch);
  }
}

V2f
gfx_window_get_cursor_position(GFX_Window handle)
{
  V2f result = {0};
  if (gfx_window_is_valid(handle)) 
  {
    LNX_Window *slot = lnx_slot_from_gfx_handle(handle);
    V2i win_relative_pos;
    
    Window root_return, child_return;
    I32 root_x_return, root_y_return;
    I32 win_x_return, win_y_return;
    U32 mask_return;
    Bool query_result = XQueryPointer(lnx_gfx_state.display, slot->x11_window, &root_return, &child_return,
                                      &root_x_return, &root_y_return, &win_x_return, &win_y_return, 
                                      &mask_return);
    Assert(query_result);
    if (query_result)
    {
      result = v2f((F32)win_x_return, (F32)win_y_return);
    }
  }
  return(result);
}