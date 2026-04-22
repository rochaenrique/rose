#include "marker.c"

#include <windows.h>
#include <psapi.h>
#include <stdio.h>

#include "win32.c"
#include "gl_defs.h"
#include "win32_wgl_defs.h"

#define GET_PROC_ADDR(MODULE, VALUE, STR) (*(PROC *)(&(VALUE)) = GetProcAddress((MODULE), (STR)))
#define WGL_GET_PROC_ADDR(VALUE, STR) (*(PROC *)(&(VALUE)) = w32_wglGetProcAddress((STR)))

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
#include "win32_wgl_funcs.h"
#undef X
    
    int missing_wgl_func = 0;
#define X(NAME, RETURN, PARAMS) if (w32_##NAME == 0){ missing_wgl_func = 1; }
#include "win32_wgl_funcs.h"
#undef X
    
    if (missing_wgl_func) 
    {
      error = S("missingl wgl function");
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
    
    
    //- load opengl extensions
    if (error.value == 0) 
    {
      w32_wglMakeCurrent(dc, bootstrap_context);
      
#define X(NAME, RETURN, PARAMS) GET_PROC_ADDR(gl_module, w32_##NAME, #NAME);
#include "win32_wgl_extensions_funcs.h"
#undef X
      
      B32 missing_wgl_ext_func = false;
#define X(NAME, RETURN, PARAMS) if (w32_##NAME == 0){ missing_wgl_ext_func = 1; }
#include "win32_wgl_extensions_funcs.h"
#undef X
      
      if (missing_wgl_ext_func) 
      {
        error = S("missingl wgl function");
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
  
  //- load gl functions
#define X(NAME, RETURN, PARAMS) GET_PROC_ADDR(gl_module, gl_##NAME, #NAME);
#include "gl_funcs.h"
#undef X
  
  if (error.length != 0) 
  {
    fprintf(stdout, "%.*s\n", str_expand(error));
    ExitProcess(1);
  }
}

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
      fprintf(stdout, "failed to create window class");
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
      fprintf(stdout, "failed to create window");
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
      fprintf(stdout, "failed to choose wgl pixel format");
      ExitProcess(1);
    }
    
    PIXELFORMATDESCRIPTOR format_desc = {0};
    BOOL spf = SetPixelFormat(dc, format_idx, &format_desc);
    if (!spf) 
    {
      fprintf(stdout, "failed to set pixel format");
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
  
  if (error.value == 0) 
  {
    fprintf(stdout, "%.*s\n", str_expand(error));
  }
  
  W32_GL_Window result = {0};
  if (error.value != 0)
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

B32
w32_poll_events()
{
  MSG message = {0};
  BOOL got_message = GetMessage(&message, 0, 0, 0);
  B32 should_stop = (got_message <= 0 || message.message == WM_CLOSE);
  if (!should_stop) 
  {
    DispatchMessage(&message);
  }
  return should_stop;
}

int __stdcall WinMain(HINSTANCE instance, 
                      HINSTANCE prev_instance, 
                      LPSTR cmd_line, 
                      int show)
{
  //- clock setup
  W32_Counters counters = w32_init_counters();
  
  //- opengl init
  // TODO(erb): have error handling with message box
  w32_init_opengl(instance);
  
  //- window creation
  // TODO(erb): have error handling with message box
  W32_GL_Window gl_window = w32_make_opengl_window(instance); 
  HWND window = gl_window.window_handle;
  HGLRC gl_context = gl_window.gl_context;
  
  //- project stuff setup
  Arena permanent_arena = w32_arena_alloc(mb(128));
  Arena transient_arena = w32_arena_alloc(mb(128));
  Codebase_File_Paths paths = w32_get_codebase_file_paths(&permanent_arena, instance);
  
  //- load app layer
  // TODO(erb): handle this error with message box
  W32_App_Library app_library = w32_load_app_library(&transient_arena, paths.library, paths.temp_library);
  
  Render_Data renderer = {0};
  {
    Read_File_Result vertex_file = w32_debug_read_entire_file(vertex_cpath);
    Read_File_Result fragment_file = w32_debug_read_entire_file(fragment_cpath);
    renderer = gl_make_renderer(&permanent_arena, 
                                (char *)vertex_file.memory, 
                                (char *)fragment_file.memory);
    w32_debug_free_file_memory(vertex_file.memory);
    w32_debug_free_file_memory(fragment_file.memory);
  }
  
  //- update loop
  ShowWindow(window, show); // TODO(erb): think of a good place to put this function, maybe a pre-first-frame?
  w32_pre_first_frame(&counters);
  for (;;) 
  {
    B32 should_stop = w32_poll_events();
    if (should_stop) 
    {
      break;
    }
    
    //- w32 gl begin
    HDC dc = w32_gl_render_begin(window, gl_context);
    
    //- render
    gl_glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
    gl_glClear(GL_COLOR_BUFFER_BIT);
    
    //- w32 gl end
    w32_wait_for_target_frame_time(&counters);
    w32_gl_render_end(window, dc);
    
    w32_gather_counters(&counters);
  }
  
  return 0;
}
