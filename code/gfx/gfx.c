GFX_Event *
gfx_push_event(Arena *arena, GFX_Event_List *list, GFX_Event_Kind kind)
{
  GFX_Event *event = PushStruct(arena, GFX_Event);
  DLLQueueAppend(list->first, list->last, event);
  list->count++;
  event->kind = kind;
  return(event);
}

String8
gfx_event_kind_str8(GFX_Event_Kind kind)
{
  switch(kind)
  {
    case GFXEventKind_None           : return S8("None");
    case GFXEventKind_WindowClose    : return S8("WindowClose");
    case GFXEventKind_WindowLoseFocus: return S8("WindowLoseFocus");
    case GFXEventKind_KeyDown        : return S8("KeyDown");
    case GFXEventKind_KeyUp          : return S8("KeyUp");
    case GFXEventKind_MouseDown      : return S8("MouseDown");
    case GFXEventKind_MouseUp        : return S8("MouseUp");
    case GFXEventKind_MouseWheel     : return S8("MouseWheel");
    case GFXEventKind_MouseMove      : return S8("MouseMove");
    case GFXEventKind_Text           : return S8("Text");
  }
  return(S8("undefined"));
}

String8
gfx_key_str8(GFX_Key key)
{
  switch(key)
  {
    case GFXKey_COUNT: InvalidCodePath;
    case GFXKey_None: return S8("None");
    
    case GFXKey_0: return S8("0");
    case GFXKey_1: return S8("1");
    case GFXKey_2: return S8("2");
    case GFXKey_3: return S8("3");
    case GFXKey_4: return S8("4");
    case GFXKey_5: return S8("5");
    case GFXKey_6: return S8("6");
    case GFXKey_7: return S8("7");
    case GFXKey_8: return S8("8");
    case GFXKey_9: return S8("9");
    
    case GFXKey_A: return S8("A");
    case GFXKey_B: return S8("B");
    case GFXKey_C: return S8("C");
    case GFXKey_D: return S8("D");
    case GFXKey_E: return S8("E");
    case GFXKey_F: return S8("F");
    case GFXKey_G: return S8("G");
    case GFXKey_H: return S8("H");
    case GFXKey_I: return S8("I");
    case GFXKey_J: return S8("J");
    case GFXKey_K: return S8("K");
    case GFXKey_L: return S8("L");
    case GFXKey_M: return S8("M");
    case GFXKey_N: return S8("N");
    case GFXKey_O: return S8("O");
    case GFXKey_P: return S8("P");
    case GFXKey_Q: return S8("Q");
    case GFXKey_R: return S8("R");
    case GFXKey_S: return S8("S");
    case GFXKey_T: return S8("T");
    case GFXKey_U: return S8("U");
    case GFXKey_V: return S8("V");
    case GFXKey_W: return S8("W");
    case GFXKey_X: return S8("X");
    case GFXKey_Y: return S8("Y");
    case GFXKey_Z: return S8("Z");
    
    case GFXKey_Space    : return S8("Space");
    case GFXKey_Quote    : return S8("Quote");
    case GFXKey_Comma    : return S8("Comma");
    case GFXKey_Minus    : return S8("Minus");
    case GFXKey_Period   : return S8("Period");
    case GFXKey_Slash    : return S8("Slash");
    case GFXKey_Semicolon: return S8("Semicolon");
    case GFXKey_Equal    : return S8("Equal");
    
    case GFXKey_LeftBracket : return S8("LeftBracket");
    case GFXKey_RightBracket: return S8("RightBracket");
    case GFXKey_Backslash   : return S8("Backslash");
    case GFXKey_Backtick    : return S8("Backtick");
    case GFXKey_Escape      : return S8("Escape");
    case GFXKey_Enter       : return S8("Enter");
    case GFXKey_Tab         : return S8("Tab");
    case GFXKey_Backspace   : return S8("Backspace");
    case GFXKey_Insert      : return S8("Insert");
    case GFXKey_Delete      : return S8("Delete");
    
    case GFXKey_Right: return S8("Right");
    case GFXKey_Left : return S8("Left");
    case GFXKey_Down : return S8("Down");
    case GFXKey_Up   : return S8("Up");
    
    case GFXKey_F1 : return S8("F1");
    case GFXKey_F2 : return S8("F2");
    case GFXKey_F3 : return S8("F3");
    case GFXKey_F4 : return S8("F4");
    case GFXKey_F5 : return S8("F5");
    case GFXKey_F6 : return S8("F6");
    case GFXKey_F7 : return S8("F7");
    case GFXKey_F8 : return S8("F8");
    case GFXKey_F9 : return S8("F9");
    case GFXKey_F10: return S8("F10");
    case GFXKey_F11: return S8("F11");
    case GFXKey_F12: return S8("F12");
    
    case GFXKey_Shift  : return S8("Shift");
    case GFXKey_Control: return S8("Control");
    case GFXKey_Alt    : return S8("Alt");
    case GFXKey_Super  : return S8("Super");
  }
  return(S8("undefined"));
}

String8
gfx_mouse_code_str8(GFX_Mouse_Code code)
{
  switch(code)
  {
    case GFXMouseCode_COUNT : InvalidCodePath;
    case GFXMouseCode_None  : return S8("None");
    case GFXMouseCode_Left  : return S8("Left");
    case GFXMouseCode_Right : return S8("Right");
    case GFXMouseCode_Middle: return S8("Middle");
  }
  return(S8("undefined"));
}

String8
gfx_modifiers_str8(Arena *arena, GFX_Modifiers mods)
{
  Scratch scratch = scratch_begin(&arena, 1);
  String8_Builder sb = str8_builder_make_default();
  
  if(mods&GFXModifiers_Alt)
  {
    str8_builder_str(arena, &sb, S8("Alt"));
  }
  if(mods&GFXModifiers_Control)
  {
    if(sb.total_size>0)
    {
      str8_builder_char(arena, &sb, '|', 1);
    }
    str8_builder_str(arena, &sb, S8("Ctrl"));
  }
  if(mods&GFXModifiers_Shift)
  {
    if(sb.total_size>0)
    {
      str8_builder_char(arena, &sb, '|', 1);
    }
    str8_builder_str(arena, &sb, S8("Shift"));
  }
  
  String8 result = push_str8_from_builder(arena, &sb);
  scratch_end(&scratch);
  return(result);
}

String8 
gfx_event_str8(Arena *arena, GFX_Event *event)
{
  Scratch scratch = scratch_begin(&arena, 1);
  String8 result = {0};
  switch(event->kind)
  {
    case GFXEventKind_None           : result = S8("None");            break;
    case GFXEventKind_WindowClose    : result = S8("WindowClose");     break;
    case GFXEventKind_WindowLoseFocus: result = S8("WindowLoseFocus"); break;
    
    case GFXEventKind_KeyDown:
    case GFXEventKind_KeyUp:
    {
      result = push_str8f(arena, "%s{code=%.*s,repeat_count=%u,mods=%.*s}", 
                          event->kind==GFXEventKind_KeyUp?"KeyUp":"KeyDown", 
                          S8Expand(gfx_key_str8(event->key.code)), event->key.repeat_count, 
                          S8Expand(gfx_modifiers_str8(scratch.arena, event->modifiers)));
    } break;
    
    case GFXEventKind_MouseDown:
    case GFXEventKind_MouseUp:
    {
      V2f pos = event->mouse.position;
      result = push_str8f(arena, "%s{code=%.*s,position=(%.1f,%.1f),mods=%.*s}", 
                          event->kind==GFXEventKind_MouseUp?"MouseUp":"MouseDown", 
                          S8Expand(gfx_mouse_code_str8(event->mouse.code)), 
                          pos.x, pos.y, S8Expand(gfx_modifiers_str8(scratch.arena, event->modifiers)));
    } break;
    
    case GFXEventKind_MouseWheel:
    {
      V2f pos = event->mouse.position;
      V2f wheel = event->mouse.wheel;
      result = push_str8f(arena, "MouseWheel{position=(%.1f,%.1f),wheel=(%.1f,%.1f)}", 
                          pos.x, pos.y, wheel.x, wheel.y);
    } break;
    
    case GFXEventKind_MouseMove:
    {
      V2f pos = event->mouse.position;
      result = push_str8f(arena, "MouseMove{position=(%.1f,%.1f)}", pos.x, pos.y);
    } break;
    
    case GFXEventKind_Text:
    {
      Implement();
    } break;
  }
  scratch_end(&scratch);
  return(result);
}

#if defined(OS_WIN32)
#include "win32/win32_gfx.c"
#elif defined(OS_LINUX)
#include "linux/linux_gfx.c"
#endif

#if GFX_BACKEND == GFX_BACKEND_OPENGL
#include "gl/gl.c"
#elif GFX_BACKEND == GFX_BACKEND_VULKAN
#include "vulkan/vulkan.c"
#endif