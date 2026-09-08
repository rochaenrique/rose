global W32_GL_State w32_gl_state;

#define w32_device_context_from_gfx_handle(handle) (HDC *)((w32_window_device_contexts + (handle)) - 1)

B32
win32_gl_choose_wgl_pixel_format(HDC dc)
{
  String8 error = {0};
  
  int format_idx = 0;
  if (error.value == 0)
  {
    int attrib_list_int[] = 
    {
      WGL_DRAW_TO_WINDOW_ARB, TRUE,
      WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB, 
      WGL_SWAP_METHOD_ARB, WGL_SWAP_EXCHANGE_ARB,
      WGL_SUPPORT_OPENGL_ARB, TRUE,
      WGL_DOUBLE_BUFFER_ARB, TRUE,
      WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
      
      WGL_COLOR_BITS_ARB,  24,
      WGL_RED_SHIFT_ARB,    0,
      WGL_GREEN_SHIFT_ARB,  8,
      WGL_BLUE_SHIFT_ARB,  16,
      WGL_ALPHA_SHIFT_ARB, 24,
      
      WGL_RED_BITS_ARB,   8,
      WGL_GREEN_BITS_ARB, 8,
      WGL_BLUE_BITS_ARB,  8,
      WGL_ALPHA_BITS_ARB, 8,
      0
    };
    
    UINT num_formats = 0; 
    BOOL did_pf = w32_wglChoosePixelFormatARB(dc,
                                              attrib_list_int,
                                              0,
                                              1,
                                              &format_idx,
                                              &num_formats);
    if (!did_pf || num_formats == 0) 
    {
      error = S8("failed to choose wgl pixel format");
    }
  }
  
  PIXELFORMATDESCRIPTOR format_desc = {0};
  if (error.value == 0)
  {
    BOOL spf = SetPixelFormat(dc, format_idx, &format_desc);
    if (!spf) 
    {
      error = S8("failed to set pixel format");
    }
  }
  
  B32 result = true;
  if (error.value != 0)
  {
    err_set_error(error);
    result = false;
  }
  return(result);
}

B32
gfx_gl_init()
{
  String8 error = {0};
  HINSTANCE instance = w32_state.instance;
  
  //- opengl lib
  HMODULE w32_gl_module = 0;
  {
    w32_gl_module = LoadLibraryA("opengl32.dll");
    if (w32_gl_module == 0)
    {
      error = S8("failed to initialize opengl32.dll");
    }
  }
  
  
  //- load wgl functions
  if (error.value == 0)
  {
#define X(NAME, RETURN, PARAMS) GET_PROC_ADDR(w32_gl_module, w32_##NAME, #NAME);
#include "gfx/gl/win32/win32_wgl_functions_x_list.h"
#undef X
    
#define X(NAME, RETURN, PARAMS) if (w32_##NAME == 0){ error = S8("missing wgl function "## #NAME); goto end_check_wgl_functions; }
#include "gfx/gl/win32/win32_wgl_functions_x_list.h"
#undef X
    end_check_wgl_functions:; 
  }
  
  //- boostrap window
  char *bootstrap_class_name = "marker_bootstrapclass";
  HWND bootstrap_window = 0;
  if (error.value == 0) 
  {
    WNDCLASSA window_class = {0};
    window_class.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    window_class.hInstance = instance;
    window_class.lpfnWndProc = DefWindowProc;
    window_class.lpszClassName = bootstrap_class_name;
    ATOM atom = RegisterClass(&window_class);
    if (atom == 0) 
    {
      error = S8("failed to create bootstrap window class");
    }
    
    if (error.value == 0) 
    {
      bootstrap_window = CreateWindowA(bootstrap_class_name,
                                       "marker_bootstrap_window",
                                       0, 0, 0, 0, 0,
                                       0, 0, instance, 0);
      if (bootstrap_window == INVALID_HANDLE_VALUE) 
      {
        error = S8("failed to create bostrap window");
      }
    }
  }
  
  // NOTE(erb): boostrap opengl
  HGLRC bootstrap_context = 0;
  if (error.value == 0)
  {
    HDC dc = GetDC(bootstrap_window);
    
    PIXELFORMATDESCRIPTOR desired_pf = {0};
    desired_pf.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    desired_pf.nVersion = 1;
    desired_pf.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
    desired_pf.iPixelType = PFD_TYPE_RGBA;
    desired_pf.cColorBits = 32;
    
    desired_pf.cRedShift   =  0;
    desired_pf.cGreenShift =  8;
    desired_pf.cBlueShift  = 16;
    desired_pf.cAlphaShift = 24;
    
    desired_pf.cRedBits   = 8;
    desired_pf.cGreenBits = 8;
    desired_pf.cBlueBits  = 8;
    desired_pf.cAlphaBits = 8;
    
    desired_pf.cDepthBits = 0;
    desired_pf.iLayerType = PFD_MAIN_PLANE;
    
    int suggested_pf_idx = ChoosePixelFormat(dc, &desired_pf);
    if (suggested_pf_idx == 0) 
    {
      error = S8("failed to choose pixel format");
    }
    
    if (error.value == 0) 
    {
      BOOL spf = SetPixelFormat(dc, suggested_pf_idx, &desired_pf);
      if (!spf) 
      {
        error = S8("failed to set pixel format");
      }
    }
    
    if (error.value == 0) 
    {
      HGLRC hglrc = w32_wglCreateContext(dc);
      if (hglrc == 0) 
      {
        error = S8("failed to create wgl context");
      }
      bootstrap_context = hglrc;
    }
    
    //- load and check gl core (1.1) functions
    if (error.value == 0)
    {
#define X(NAME, RETURN, PARAMS) GET_PROC_ADDR(w32_gl_module, NAME, #NAME);
#include "gfx/gl/gl_core_functions_x_list.h"
#undef X
      
#define X(NAME, RETURN, PARAMS) if (NAME == 0) { error = S8("missing core gl function " #NAME); goto end_gl_core_functions; }
#include "gfx/gl/gl_core_functions_x_list.h"
#undef X
      end_gl_core_functions:;
    }
    
    //- make wgl current to load extensions
    w32_wglMakeCurrent(dc, bootstrap_context);
    
    //- load wgl extensions
    if (error.value == 0) 
    {
#define X(NAME, RETURN, PARAMS, EXT_STR, REQUIRED) WGL_GET_PROC_ADDR(w32_##NAME, #NAME);
#include "gfx/gl/win32/win32_wgl_extensions_functions_x_list.h"
#undef X
      
#define X(NAME, RETURN, PARAMS, EXT_STR, REQUIRED) \
if (REQUIRED && w32_##NAME == 0) { error = S8("missing wgl extension function " #NAME); goto end_check_wgl_ext_functions; }
#include "gfx/gl/win32/win32_wgl_extensions_functions_x_list.h"
#undef X
      end_check_wgl_ext_functions:;
    }
    
    //- wgl extensions
    if (error.value == 0) 
    {
      Scratch ext_scratch = scratch_begin(0, 0);
      
      //- get wgl extension string and build hash table
      String8_Hash_Set ext_table = {0};
      if (error.value == 0 && w32_wglGetExtensionsStringARB) 
      {
        String8 ext_str = S8CStr((char *)w32_wglGetExtensionsStringARB(dc));
        ext_table = build_gl_extension_table(ext_scratch.arena, ext_str);
        if (ext_str.value == 0 || ext_table.slot_count == 0) 
        {
          error = S8("couldn't get extension string");
        }
      }
      
      //- validate required extensions
      if (error.value == 0) 
      {
#define X(NAME, RETURN, PARAMS, EXT_STR, REQUIRED) { \
B32 is_required = (REQUIRED != false); \
if (is_required && !str8_hash_set_contains(ext_table, S8(EXT_STR))) \
{ error = S8("wgl extension string is missing required "##EXT_STR); }  \
}
#include "gfx/gl/win32/win32_wgl_extensions_functions_x_list.h"
#undef X
      }
      
      //- set the swap interval if we can
      if (w32_wglSwapIntervalEXT && str8_hash_set_contains(ext_table, S8("WGL_EXT_swap_control")))
      {
        w32_wglSwapIntervalEXT(1);
      }
      
      scratch_end(&ext_scratch);
    }
    
    //- load and gl modern functions (3.3 and above I think) 
    if (error.value == 0)
    {
#define X(NAME, RETURN, PARAMS) WGL_GET_PROC_ADDR(NAME, #NAME);
#include "gfx/gl/gl_modern_functions_x_list.h"
#undef X
      
#define X(NAME, RETURN, PARAMS) if (NAME == 0) { error = S8("missing modern gl function " #NAME); goto end_gl_modern_functions; }
#include "gfx/gl/gl_modern_functions_x_list.h"
#undef X
      end_gl_modern_functions:;
    }
    
    ReleaseDC(bootstrap_window, dc);
  }
  
  //- destroy boostrapping window and context
  {
    HDC dc = GetDC(bootstrap_window);
    w32_wglMakeCurrent(dc, 0);
    ReleaseDC(bootstrap_window, dc);
    
    BOOL deleted_context = w32_wglDeleteContext(bootstrap_context);
    Assert(deleted_context);
    
    BOOL destoyed_window = DestroyWindow(bootstrap_window);
    Assert(destoyed_window);
    
    BOOL unregistered = UnregisterClassA(bootstrap_class_name, instance);
    Assert(unregistered);
  }
  
  //- create single global opengl context
  
  //- create dummy window
  HWND dummy_window = 0;
  if (error.value == 0) 
  {
    char *w32_window_class_name = w32_get_registered_window_class_name();
    if (!w32_window_class_name)
    {
      error = S8("failed to get registered window");
    }
    else
    {
      dummy_window = CreateWindowA(w32_window_class_name,
                                   "dummy_window",
                                   WS_TILEDWINDOW, 
                                   CW_USEDEFAULT, CW_USEDEFAULT, // x, y
                                   CW_USEDEFAULT, CW_USEDEFAULT, // w, h
                                   0, 0, instance, 0); // parent, menu, instance, param
      if (dummy_window == INVALID_HANDLE_VALUE) 
      {
        error = S8("failed to create window");
      }
    }
  }
  
  HGLRC dummy_context = 0;
  if (error.value == 0)
  {
    HDC dc = GetDC(dummy_window);
    
    if (!win32_gl_choose_wgl_pixel_format(dc))
    {
      error = err_get_last_error();
    }
    else
    {
      int attribs[] = 
      {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0,
      };
      
      dummy_context = w32_wglCreateContextAttribsARB(dc, 0, attribs);
      if (dummy_context == 0) 
      {
        error = S8("failed to create acontext arb");
      }
    }
    
    ReleaseDC(dummy_window, dc);
  }
  
  if(error.value == 0)
  {
#if DEBUG
    //- gl strings
    if (error.value == 0)
    {
      os_logf("GL Vendor          : %s", glGetString(GL_VENDOR));
      os_logf("GL Renderer        : %s", glGetString(GL_RENDERER));
      os_logf("GL Version         : %s", glGetString(GL_VERSION));
      os_logf("GL Shading Language: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
    }
#endif
  }
  
  if (error.value == 0)
  {
    ZeroStruct(&w32_gl_state);
    w32_gl_state.dummy_window = dummy_window;
    w32_gl_state.dummy_context = dummy_context;
    w32_gl_state.gl_module = w32_gl_module;
    if (w32_gl_module != 0)
    {
      FreeLibrary(w32_gl_module);
    }
  }
  else
  {
    HDC dc = GetDC(dummy_window);
    {
      w32_wglMakeCurrent(dc, 0);
    }
    ReleaseDC(dummy_window, dc);
    w32_wglDeleteContext(dummy_context);
    DestroyWindow(dummy_window);
    err_set_error(error);
  }
  B32 result = (error.value == 0);
  return result;
}

B32
gfx_gl_unequip(GFX_Window handle)
{
  B32 result = false;
  if (gfx_window_is_valid(handle))
  {
    w32_gl_state.current_dc = 0;
    result = true;
  }
  else
  {
    err_set_error(S8("windows window is not valid"));
  }
  return(result);
}

B32  
gfx_gl_equip(GFX_Window handle)
{
  String8 error = {0};
  
  HWND window = 0;
  if (gfx_window_is_valid(handle))
  {
    W32_Window *slot = w32_slot_from_gfx_handle(handle);
    slot->release = gfx_gl_unequip;
    window = slot->hwnd;
  }
  else
  {
    error = S8("windows window is not valid");
  }
  
  if (error.value == 0)
  {
    HDC dc = GetDC(window);
    if (!win32_gl_choose_wgl_pixel_format(dc))
    {
      error = err_get_last_error();
    }
    ReleaseDC(window, dc);
  }
  
  B32 result = true;
  if (error.value != 0)
  {
    result = false;
    err_set_error(error);
  }
  return(result);
}

void
w32_window_make_gl_current(HWND hwnd)
{
  w32_gl_state.current_dc = GetDC(hwnd);
  BOOL made_current = w32_wglMakeCurrent(w32_gl_state.current_dc, w32_gl_state.dummy_context);
  Assert(made_current);
}

void
w32_window_clear_gl_current(HWND hwnd)
{
  ReleaseDC(hwnd, w32_gl_state.current_dc);
  BOOL unmade_current = w32_wglMakeCurrent(0, 0);
  Assert(unmade_current);
  w32_gl_state.current_dc = 0;
}

void 
gfx_gl_attach_begin(void)
{
  w32_window_make_gl_current(w32_gl_state.dummy_window);
}

void 
gfx_gl_attach_end(void)
{
  w32_window_clear_gl_current(w32_gl_state.dummy_window);
}

void
gfx_gl_render_begin(GFX_Window handle)
{
  if (gfx_window_is_valid(handle))
  {
    W32_Window *window_slot = w32_slot_from_gfx_handle(handle);
    w32_window_make_gl_current(window_slot->hwnd);
  }
}

void
gfx_gl_render_end(GFX_Window handle)
{
  if (gfx_window_is_valid(handle))
  {
    W32_Window *window_slot = w32_slot_from_gfx_handle(handle);
    
    BOOL swapped = SwapBuffers(w32_gl_state.current_dc);
    Assert(swapped);
    
    w32_window_clear_gl_current(window_slot->hwnd);
  }
}
