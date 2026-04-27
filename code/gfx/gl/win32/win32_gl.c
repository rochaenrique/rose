
B32
wgl_ext_table_contains(String_Table table, String ext)
{
  B32 result = false;
  
  U64 idx = str_hash(ext) % table.slot_count;
  String_Table_Node *slot = (table.slots + idx);
  
  for(;slot; 
      slot = slot->next)
  {
    if (str_equals(slot->str, ext)) 
    {
      result = true;
      break;
    }
  }
  
  return result;
}

String_Table
build_wgl_ext_table(Arena *arena, String haystack)
{
  String_Table table = {0};
  
  //- create the hash table array
  // NOTE(erb): dumb heuristic on how many extension strings there are to create the hash table size
  U32 ext_length_estimate = (U32)S("WGL_ARB_").length + 15;
  table.slot_count = (U32)(haystack.length / ext_length_estimate); 
  table.slots = push_array(arena, table.slot_count, String_Table_Node);
  
  U64 last_delim = 0;
  while (last_delim < haystack.length)
  {
    //- get next space seperator
    U64 next_delim = 0;
    Index_U64 delim_idx = str_index_char_start(haystack, ' ', last_delim);
    if (delim_idx.exists)
    {
      next_delim = delim_idx.value;
    }
    else
    {
      break;
    }
    
    String ext = str_slice(haystack, last_delim, next_delim);
    
    U64 idx = str_hash(ext) % table.slot_count;
    String_Table_Node *slot = (table.slots + idx);
    
    //- test if extension is allready in table
    B32 exists = false;
    for(;slot; 
        slot = slot->next)
    {
      if (str_equals(slot->str, ext)) 
      {
        exists = true;
        break;
      }
      if (!slot->next)
      {
        break;
      }
    }
    
    //- add extension if it isn't in the table
    if (!exists)
    {
      //- if the current slot is not empty add a next
      if (slot->str.length != 0)
      {
        slot = slot->next = push_struct(arena, String_Table_Node);
      }
      slot->str = ext;
      table.total_count++;
    }
    
    last_delim = (next_delim + 1);
  }
  
  return table;
}

// TODO(erb): fix error handling here, this should not exit the process, 
// it should allow the caller to handle the error
void
w32_init_opengl(HINSTANCE instance)
{
  String error = {0};
  
  //- opengl lib
  HMODULE gl_module = LoadLibraryA("opengl32.dll");
  if (gl_module == 0) 
  {
    error = S("failed to initialize opengl32.dll");
  }
  
  //- load wgl functions
  if (error.value == 0)
  {
#define X(NAME, RETURN, PARAMS) GET_PROC_ADDR(gl_module, w32_##NAME, #NAME);
#include "gfx/gl/win32/win32_wgl_functions_x_list.h"
#undef X
    
    B32 missing_wgl_func = false;
#define X(NAME, RETURN, PARAMS) if (w32_##NAME == 0){ missing_wgl_func = true; }
#include "gfx/gl/win32/win32_wgl_functions_x_list.h"
#undef X
    if (missing_wgl_func) 
    {
      error = S("missing wgl function");
    }
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
      error = S("failed to create bootstrap window class");
    }
    
    if (error.value == 0) 
    {
      bootstrap_window = CreateWindowA(bootstrap_class_name,
                                       "marker_bootstrap_window",
                                       0, 0, 0, 0, 0,
                                       0, 0, instance, 0);
      if (bootstrap_window == INVALID_HANDLE_VALUE) 
      {
        error = S("failed to create bostrap window");
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
    
    desired_pf.cRedShift =    0;
    desired_pf.cGreenShift =  8;
    desired_pf.cBlueShift =  16;
    desired_pf.cAlphaShift = 24;
    
    desired_pf.cRedBits =   8;
    desired_pf.cGreenBits = 8;
    desired_pf.cBlueBits =  8;
    desired_pf.cAlphaBits = 8;
    
    desired_pf.cDepthBits = 0;
    desired_pf.iLayerType = PFD_MAIN_PLANE;
    
    int suggested_pf_idx = ChoosePixelFormat(dc, &desired_pf);
    if (suggested_pf_idx == 0) 
    {
      error = S("failed to choose pixel format");
    }
    
    if (error.value == 0) 
    {
      BOOL spf = SetPixelFormat(dc, suggested_pf_idx, &desired_pf);
      if (!spf) 
      {
        error = S("failed to set pixel format");
      }
    }
    
    if (error.value == 0) 
    {
      HGLRC hglrc = w32_wglCreateContext(dc);
      if (hglrc == 0) 
      {
        error = S("failed to create wgl context");
      }
      bootstrap_context = hglrc;
    }
    
    if (error.value == 0)
    {
      //- load gl functions
#define X(NAME, RETURN, PARAMS) GET_PROC_ADDR(gl_module, NAME, #NAME);
#include "gfx/gl/gl_functions_x_list.h"
#undef X
      
#define X(NAME, RETURN, PARAMS) if (NAME == 0) { error = S("missing gl function "## #NAME); }
#include "gfx/gl/gl_functions_x_list.h"
#undef X
    }
    
    //- load opengl extensions
    if (error.value == 0) 
    {
      w32_wglMakeCurrent(dc, bootstrap_context);
      
#define X(NAME, RETURN, PARAMS, EXT_STR, REQUIRED) WGL_GET_PROC_ADDR(w32_##NAME, #NAME);
#include "gfx/gl/win32/win32_wgl_extensions_functions_x_list.h"
#undef X
      
#define X(NAME, RETURN, PARAMS, EXT_STR, REQUIRED) \
if (REQUIRED && w32_##NAME == 0) \
{ error = S("missing wgl extension function "## #NAME); }
#include "gfx/gl/win32/win32_wgl_extensions_functions_x_list.h"
#undef X
    }
    
    // NOTE(erb): glGetString deals with pure opengl extensions and WGL extensions do not appear in it. 
    // Since we don't use any pure opengl extensions, we won't need to use it.
    
    scratch_block(scratch, 0, 0)
    {
      //- get wgl extension string and build hash table
      String_Table ext_table = {0};
      if (error.value == 0 && w32_wglGetExtensionsStringARB) 
      {
        String ext_str = SC((char *)w32_wglGetExtensionsStringARB(dc));
        ext_table = build_wgl_ext_table(scratch.arena, ext_str);
        if (ext_str.value == 0 || ext_table.slot_count == 0) 
        {
          error = S("couldn't get extension string");
        }
      }
      
      //- check against extension string
      if (error.value == 0) 
      {
#define X(NAME, RETURN, PARAMS, EXT_STR, REQUIRED) \
if (REQUIRED && !wgl_ext_table_contains(ext_table, S(EXT_STR))) \
{ error = S("wgl extension string is missing required "##EXT_STR); } 
#include "gfx/gl/win32/win32_wgl_extensions_functions_x_list.h"
#undef X
      }
      
      //- set the swap interval if we can
      if (w32_wglSwapIntervalEXT && 
          wgl_ext_table_contains(ext_table, S("WGL_EXT_swap_control")))
      {
        w32_wglSwapIntervalEXT(1);
      }
    }
    
    ReleaseDC(bootstrap_window, dc);
  }
  
  
  //- destroy boostrapping window and context
  {
    HDC dc = GetDC(bootstrap_window);
    w32_wglMakeCurrent(0, 0);
    ReleaseDC(bootstrap_window, dc);
    
    BOOL deleted_context = w32_wglDeleteContext(bootstrap_context);
    assert(deleted_context);
    
    BOOL destoyed_window = DestroyWindow(bootstrap_window);
    assert(destoyed_window);
    
    BOOL unregistered = UnregisterClassA(bootstrap_class_name, instance);
    assert(unregistered);
  }
  
  if (error.length != 0) 
  {
    char msg[64];
    sprintf_s(msg, sizeof(msg), "%.*s\n", str_expand(error));
    OutputDebugStringA(msg);
    ExitProcess(1);
  }
}

// TODO(erb): fix error handling here, this should not exit the process, 
// it should allow the caller to handle the error
W32_GL_Window
w32_make_opengl_window(HINSTANCE instance)
{
  String error = {0};
  
  //- create window class and window
  HWND window = 0;
  {
    char *window_class_name = "marker_window_class";
    WNDCLASSA window_class = {0};
    //window_class.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    window_class.hInstance = instance;
    window_class.lpfnWndProc = DefWindowProc;
    window_class.lpszClassName = window_class_name;
    ATOM atom = RegisterClass(&window_class);
    if (atom == 0) 
    {
      OutputDebugStringA("failed to create window class\n");
      ExitProcess(1);
    }
    
    window = CreateWindowA(window_class_name,
                           "Marker",
                           WS_TILEDWINDOW, 
                           CW_USEDEFAULT, CW_USEDEFAULT, // x, y
                           CW_USEDEFAULT, CW_USEDEFAULT, // w, h
                           0, 0, instance, 0); // parent, menu, instance, param
    if (window == INVALID_HANDLE_VALUE) 
    {
      OutputDebugStringA("failed to create window\n");
      ExitProcess(1);
    }
  }
  
  //- load gl context
  HGLRC gl_context = 0;
  {
    HDC dc = GetDC(window);
    
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
    
    int format_idx = 0;
    UINT num_formats = 0; 
    BOOL did_pf = w32_wglChoosePixelFormatARB(dc,
                                              attrib_list_int,
                                              0,
                                              1,
                                              &format_idx,
                                              &num_formats);
    if (!did_pf || num_formats == 0) 
    {
      OutputDebugStringA("failed to choose wgl pixel format\n");
      ExitProcess(1);
    }
    
    PIXELFORMATDESCRIPTOR format_desc = {0};
    BOOL spf = SetPixelFormat(dc, format_idx, &format_desc);
    if (!spf) 
    {
      OutputDebugStringA("failed to set pixel format\n");
      ReleaseDC(window, dc);
      ExitProcess(1);
    }
    
    int attribs[] = {
      WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
      WGL_CONTEXT_MINOR_VERSION_ARB, 3,
      WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
      WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
      0,
    };
    
    HGLRC hglrc = w32_wglCreateContextAttribsARB(dc, 0, attribs);
    if (hglrc == 0) 
    {
      fprintf(stdout, "failed to create acontext arb");
      ReleaseDC(window, dc);
      ExitProcess(1);
    }
    gl_context = hglrc;
  }
  
  if (error.value != 0) 
  {
    char msg[64];
    sprintf_s(msg, sizeof(msg), "%.*s\n", str_expand(error));
    OutputDebugStringA(msg);
  }
  
  W32_GL_Window result = {0};
  if (error.value == 0)
  {
    result.window_handle = window;
    result.gl_context = gl_context;
  }
  
  return(result);
}

HDC w32_gl_render_begin(HWND window, HGLRC gl_context)
{
  HDC dc = GetDC(window);
  w32_wglMakeCurrent(dc, gl_context);
  return dc;
}

void
w32_gl_render_end(HWND window, HDC dc)
{
  SwapBuffers(dc);
  ReleaseDC(window, dc);
}