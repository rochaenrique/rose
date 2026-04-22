#include <windows.h>
#include <psapi.h>
#include <stdio.h>

#include "marker.h"
#include "marker_opengl.c"

#include "marker_platform.c"

#include <ft2build.h>
#include FT_FREETYPE_H

#define true 1
#define false 0

#undef abs
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Loaded_Image w32_stb_load_image(String path)
{
  stbi_set_flip_vertically_on_load(true);
  Loaded_Image image = {0};
  image.data = stbi_load(str_to_temp512_cstr(path), 
                         &image.size.x, &image.size.y, 
                         &image.channels, 0);
  return image;
}

void w32_stb_free_image(Loaded_Image *image)
{
  stbi_image_free(image->data);
  image->data = 0;
}

typedef struct Win32_App_Library
{
  Update_And_Render_Func *update_and_render;
  HMODULE module;
  FILETIME last_write_time;
  B32 valid;
} Win32_App_Library;

typedef struct Input_Event_Array
{
  Input_Event events[64];
  U32 count;
} Input_Event_Array;

file_local U64 g_tick;
file_local B32 g_running;
file_local Render_Data g_renderer;
file_local U64 g_perf_count_freq;
file_local HDC g_device_context;

V2i w32_get_frame_buffer_size(HWND window)
{
  RECT rect;
  GetClientRect(window, &rect);
  V2i result = v2i((I32)(rect.right - rect.left),
                   (I32)(rect.bottom - rect.top));
  return result;
}

V2i w32_get_window_size(HWND window)
{
  RECT rect;
  GetWindowRect(window, &rect);
  V2i result = v2i((I32)(rect.right - rect.left - 1),
                   (I32)(rect.bottom - rect.top - 1));
  return result;
}

FT_Library freetype_library;
Font_Data *w32_freetype_load_font(Arena *arena, String path)
{
  if (freetype_library == 0) 
  {
    if (FT_Init_FreeType(&freetype_library))
    {
      OutputDebugStringA("Failed to initialize freetype!\n");
      assert(false);
    }
  }
  
  FT_Face font_face;
  
  {
    char *cstr_path = str_to_temp512_cstr(path);
    if (FT_New_Face(freetype_library, cstr_path, 0, &font_face))
    {
      char msg[255];
      sprintf_s(msg, sizeof(msg), "Failed to load font (%s)!\n", cstr_path);
      OutputDebugStringA(msg);
      assert(false);
    }
  }
  
  FT_Set_Pixel_Sizes(font_face, 0, 48);
  
  Font_Data *font_data = push_struct(arena, Font_Data);
  font_data->height = font_face->height >> 6;
  font_data->below_base_line_offset = -(font_face->bbox.yMin >> 6);
  font_data->above_base_line_offset = font_face->bbox.yMax >> 6;
  
  for (U32 char_idx = 0;
       char_idx < array_size(font_data->ascii_glyph_table);
       char_idx++) 
  {
    if (!FT_Load_Char(font_face, char_idx, FT_LOAD_RENDER))
    {
      
      FT_Glyph_Metrics metrics = font_face->glyph->metrics;
      Glyph_Info info = {0};
      info.advance_x = metrics.horiAdvance >> 6;
      info.bearing = v2i(metrics.horiBearingX >> 6, 
                         metrics.horiBearingY >> 6);
      
      //info.size = v2i(metrics.width >> 6, metrics.height >> 6);
      
      info.size = v2i(font_face->glyph->bitmap.width,
                      font_face->glyph->bitmap.rows);
      
      info.texture_id = gl_load_glyph_texture(arena, 
                                              font_face->glyph->bitmap.buffer,
                                              font_face->glyph->bitmap.width,
                                              font_face->glyph->bitmap.rows);
      
      font_data->ascii_glyph_table[char_idx] = info;
    }
    else
    {
      char msg[255];
      sprintf_s(msg, sizeof(msg), "Error: Freetype: Failed to load Glyph %c\n", char_idx);
      OutputDebugStringA(msg);
    }
  }
  
  FT_Done_Face(font_face);
  
  return font_data;
}

void w32_init_opengl(HDC device_context)
{
  PIXELFORMATDESCRIPTOR desired_pixel_format = {0};
  desired_pixel_format.nSize = sizeof(PIXELFORMATDESCRIPTOR);
  desired_pixel_format.nVersion = 1;
  desired_pixel_format.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
  desired_pixel_format.iPixelType = PFD_TYPE_RGBA;
  desired_pixel_format.cColorBits = 32;
  desired_pixel_format.cAlphaBits = 8;
  desired_pixel_format.iLayerType = PFD_MAIN_PLANE;
  
  I32 suggested_pixel_format_idx = ChoosePixelFormat(device_context, &desired_pixel_format);
  
  PIXELFORMATDESCRIPTOR suggested_pixel_format;
  DescribePixelFormat(device_context, 
                      suggested_pixel_format_idx, 
                      sizeof(PIXELFORMATDESCRIPTOR), 
                      &suggested_pixel_format);
  SetPixelFormat(device_context, suggested_pixel_format_idx, &suggested_pixel_format);
  
  HGLRC opengl_rc = wglCreateContext(device_context);
  if (wglMakeCurrent(device_context, opengl_rc))
  {
    gl_load_functions();
  }
  else
  {
    assert(false);
  }
}

void w32_update_window(HDC device_context, Render_Data *renderer, V2i viewport)
{
  gl_render(renderer, viewport);
  SwapBuffers(device_context);
}

Input_Event *append_event(Input_Event_Array *array)
{
  assert(array->count <= array_size(array->events));
  Input_Event *event = (array->events + array->count++);
  return event;
}

LRESULT CALLBACK w32_main_window_callback(HWND window,
                                          UINT message,
                                          WPARAM w_param,
                                          LPARAM l_param)
{
  Input_Event_Array *event_array = (Input_Event_Array *)GetWindowLongPtrA(window, GWLP_USERDATA);
  
  char msg[256];
  LRESULT result = 0;
  switch (message)
  {
    case WM_SIZE: 
    if (event_array)
    {
      U32 lower_16 = ((1 << 16) - 1);
      I32 width = (I32)(l_param & lower_16);
      I32 height = (I32)(l_param >> 16);
      
      Input_Event *event = append_event(event_array);
      event->kind = InputEventKind_WindowResize;
      event->window_resize.screen_space = v2i(width, height);
      
      sprintf_s(msg, sizeof(msg), "wm_size = (%d, %d)\n", width, height);
      OutputDebugStringA(msg);
      
      /* NOTE(erb): window sends additional information of the context of the resize which could be usefull
            switch (w_param)
            {
              case SIZE_MAXHIDE: event->window_resize.kind = WindowResizeKind_MaxHide; break;
              case SIZE_MAXSHOW: event->window_resize.kind = WindowResizeKind_MaxShow; break;
              case SIZE_MAXIMIZED: event->window_resize.kind = WindowResizeKind_Maximized; break;
              case SIZE_MINIMIZED: event->window_resize.kind = WindowResizeKind_Minimized; break;
              case SIZE_RESTORED: event->window_resize.kind = WindowResizeKind_Restored; break;
            }
             */
    } break;
    
    case WM_CLOSE: 
    case WM_DESTROY: 
    {         
      g_running = false;
    } break;
    
    case WM_ACTIVATEAPP: 
    {
      OutputDebugStringA("Message WM_ACTIVATEAPP\n");
    } break;
    
    case WM_MOUSEMOVE:
    {
      U32 lower_16 = ((1 << 16) - 1);
      // NOTE(erb): values in pixels
      I32 x_pos = (I32)(l_param & lower_16);
      I32 y_pos = (I32)(l_param >> 16);
      
      sprintf_s(msg, sizeof(msg), "mouse_move = (%d, %d)\n", x_pos, y_pos);
      OutputDebugStringA(msg);
    } break;
    
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    {
      U32 lower_16 = ((1 << 16) - 1);
      // NOTE(erb): values in pixels
      I32 x_pos = (I32)(l_param & lower_16);
      I32 y_pos = (I32)(l_param >> 16);
      
#if 0
      B8 control_down = (w_param & MK_CONTROL);
      B8 shift_down = (w_param & MK_SHIFT);
      
      B8 left_down = (w_param & MK_LBUTTON);
      B8 middle_down = (w_param & MK_MBUTTON);
      B8 right_down = (w_param & MK_RBUTTON);
      B8 xbutton1_down = (w_param & MK_XBUTTON1);
      B8 xbutton2_down = (w_param & MK_XBUTTON2);
#endif
      
      sprintf_s(msg, sizeof(msg), "mouse_move = (%d, %d)\n", x_pos, y_pos);
      OutputDebugStringA(msg);
    } break;
    
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
      U32 virtual_key_code = (U32)w_param;
      
      B8 was_down = (l_param & (1 << 30)) != 0;
      B8 is_down = (l_param & (1 << 31)) == 0;
      
#if 0
      U32 mask_16 = ((1 << 16) - 1);
      B8 is_extended = (l_param & (1 << 24)) != 0; // NOTE(erb): leflt alt, left control, etc
      B8 key_repeat = (was_down == is_down);
      U32 repeat_count = (l_param & mask_16);
      U32 scan_code = ((l_param >> 16) & mask_16);
#endif
      
      sprintf_s(msg, sizeof(msg), 
                "key = '%c', was_down = %d, is_down = %d\n", 
                virtual_key_code, was_down, is_down);
      OutputDebugStringA(msg);
    } break;
    
    
    case WM_CHAR:
    { 
      U32 mask_8 = 0xFF;
      char text[4];
      text[0] = (char)(w_param & mask_8);
      text[1] = (char)((w_param >> 8) & mask_8);
      text[2] = (char)((w_param >> 16) & mask_8);
      text[3] = (char)((w_param >> 24) & mask_8);
      sprintf_s(msg, sizeof(msg), "text = '%s'\n", text);
    } break;
    
    
    case WM_PAINT: 
    {
      OutputDebugStringA("Message WM_PAINT\n");
      /* 
            V2i viewport = w32_get_frame_buffer_size(window);
            w32_update_window(g_device_context, &g_renderer, viewport);
             */
      
      // NOTE(erb): required to validate the paint call
      PAINTSTRUCT paint;
      BeginPaint(window, &paint);
      EndPaint(window, &paint);
    } break;
    
    case WM_ERASEBKGND: 
    {
      //NOTE(erb): do nothing here
      result = 1;
    } break;
    
    default: 
    {
      result = DefWindowProcW(window, message, w_param, l_param);
    } break;
  }
  return result;
}

typedef struct Read_File_Result
{
  void *memory;
  U32 size;
} Read_File_Result;

void w32_debug_free_file_memory(void *memory)
{
  if (memory != 0)
  {
    VirtualFree(memory, 0, MEM_RELEASE);
  }
}

U32 safe_truncate_U64_to_U32(U64 value)
{
  assert(value <= 0xFFFFFFFF);
  U32 result = (U32)value;
  return result;
}

Read_File_Result w32_debug_read_entire_file(char *file_path)
{
  // TODO(erb): protect against size(file_name) < MAX_PATH
  // TODO(erb): add flags and have a "should null terminate" flag
  
  Read_File_Result result = {0};
  
  HANDLE file_handle = CreateFileA(file_path,
                                   GENERIC_READ, FILE_SHARE_READ,
                                   0, OPEN_EXISTING,
                                   0, 0);
  
  if (file_handle != INVALID_HANDLE_VALUE) 
  {
    LARGE_INTEGER file_size_result;
    if (GetFileSizeEx(file_handle, &file_size_result)) 
    {
      U32 size = safe_truncate_U64_to_U32(file_size_result.QuadPart);
      void *memory = VirtualAlloc(0, size + 1, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
      if (memory)
      {
        DWORD bytes_read = 0;
        if (ReadFile(file_handle, memory, size, &bytes_read, 0) &&
            bytes_read == size)
        {
          result.memory = memory;
          result.size = size;
          ((U8 *)result.memory)[size] = 0;
        }
        else
        {
          w32_debug_free_file_memory(memory);
          OutputDebugStringA("Error: ReadFile()\n");
        }
      }
      else
      {
        OutputDebugStringA("Error: VirtualAlloc()\n");
      }
    }
    else
    {
      OutputDebugStringA("Error: GetFileSizeEx()\n");
    }
    
    if (!CloseHandle(file_handle))
    {
      OutputDebugStringA("Error: CloseHandle()\n");
    }
  }
  else
  {
    OutputDebugStringA("Error: CreateFileA()\n");
  }
  
  return result;
}

B32 debug_write_entire_file(char *file_path, void *memory, U32 memory_size)
{
  // TODO(erb): protect against size(file_name) < MAX_PATH
  B32 result = false;
  HANDLE file_handle = CreateFileA(file_path,
                                   GENERIC_WRITE, 
                                   0, 0, 
                                   CREATE_ALWAYS,
                                   0, 0);
  
  if (file_handle != INVALID_HANDLE_VALUE) 
  {
    DWORD bytes_written;
    if (WriteFile(file_handle, memory, memory_size, &bytes_written, 0) &&
        bytes_written == memory_size)
    {
      result = true;
    }
    else
    {
      OutputDebugStringA("Error: ReadFile()\n");
    }
    
    if (!CloseHandle(file_handle))
    {
      OutputDebugStringA("Error: CloseHandle()\n");
    }
  }
  else
  {
    OutputDebugStringA("Error: CreateFileA()\n");
  }
  
  return result;
}

void w32_unload_library(Win32_App_Library *library)
{
  library->valid = false;
  FreeLibrary(library->module);
}

B32 w32_update_last_file_time(char *path, FILETIME *last_write_time)
{
  FILETIME new_write_time = w32_get_last_write_time(path);
  LONG diff = CompareFileTime(&new_write_time, last_write_time);
  B32 result = (diff == 1);
  if (result) 
  {
    *last_write_time = new_write_time;
  }
  return result;
}

U32 w32_pack_rgba_U32(V4f color)
{
  U32 result = 0;
  result |= (U32)(cap01(color.a) * 255) << 24;
  result |= (U32)(cap01(color.b) * 255) << 16;
  result |= (U32)(cap01(color.g) * 255) << 8;
  result |= (U32)(cap01(color.r) * 255);
  return result;
}

int __stdcall WinMain(HINSTANCE instance, 
                      HINSTANCE prev_instance, 
                      LPSTR cmd_line, 
                      int show)
{
  Arena permanent_arena = {0};
  Arena transient_arena = {0};
  {
    U64 permanent_arena_size = mb(128);
    void *permanent_arena_memory = w32_allocate(permanent_arena_size);
    arena_init(&permanent_arena, permanent_arena_memory, permanent_arena_size);
    
    U64 transient_arena_size = mb(128);
    void *transient_arena_memory = w32_allocate(transient_arena_size);
    arena_init(&transient_arena, transient_arena_memory, transient_arena_size);
  }
  
  // TODO(erb): add some kind of fallback here. this might be only for debug or a something 
  // for the final build, anyways add some terminal error message here
  Win32_App_Library app_library = w32_load_app_library(&transient_arena, library_path, temp_library_path);
  assert(app_library.valid);
  
  WNDCLASSA window_class = {0};
  window_class.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
  window_class.hInstance = instance;
  window_class.lpfnWndProc = w32_main_window_callback;
  window_class.lpszClassName = "MainWindowClass";
  window_class.cbWndExtra = sizeof(Input_Event_Array);
  
  LARGE_INTEGER perf_count_freq_result;
  QueryPerformanceFrequency(&perf_count_freq_result);
  g_perf_count_freq = perf_count_freq_result.QuadPart;
  
  U32 desired_scheduler_ms = 1;
  B32 sleep_is_granular = (timeBeginPeriod(desired_scheduler_ms) == TIMERR_NOERROR);
  
  I32 monitor_hz = 60;
  F32 target_seconds_per_frame_goal = 1.0f/(F32)monitor_hz;
  
  if (RegisterClass(&window_class)) 
  {
    HWND window_handle = CreateWindowExA(0, 
                                         window_class.lpszClassName,
                                         "Window",
                                         WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                                         CW_USEDEFAULT,
                                         CW_USEDEFAULT,
                                         CW_USEDEFAULT,
                                         CW_USEDEFAULT,
                                         0,
                                         0,
                                         instance,
                                         0);
    if (window_handle)
    {
      g_device_context = GetDC(window_handle);
      
      w32_init_opengl(g_device_context);
      
      // NOTE(erb): window data
      
      V4f debug_colors[4] = 
      {
        color_blue,
        color_red,
        color_green,
        color_cyan,
      };
      Loaded_Image debug_images[4];
      Image_Texture debug_textures[4];
      for (U32 idx = 0;
           idx < array_size(debug_textures);
           ++idx)
      {
        U32 packed_color = w32_pack_rgba_U32(debug_colors[idx]);
        debug_images[idx] = generate_debug_image(&permanent_arena, 100, 100, packed_color);
        debug_textures[idx] = gl_make_texture_from_image(&debug_images[idx]);
      }
      
      for (U32 idx = 0;
           idx < array_size(debug_textures);
           ++idx)
      {
        U32 packed_color = w32_pack_rgba_U32(debug_colors[idx]);
        debug_images[idx] = generate_debug_image(&permanent_arena, 100, 100, packed_color);
        debug_textures[idx] = gl_make_texture_from_image(&debug_images[idx]);
      }
      
      for (U32 idx = 0;
           idx < array_size(debug_textures);
           ++idx) 
      {
        Image_Texture *tex = (debug_textures + idx);
        gl_debug_texture(&transient_arena, tex->id, (U32)tex->size.x, (U32)tex->size.y, 4);
      }
      
      Font_Data *default_font = w32_freetype_load_font(&permanent_arena, font_path);
      supress_unused(default_font);
      
      Input_Event_Array *event_array = push_struct(&permanent_arena, Input_Event_Array);
      SetLastError(0);
      SetWindowLongPtrA(window_handle, GWLP_USERDATA, (LONG_PTR)event_array);
      assert(GetLastError() == 0);
      
      V2f resolution = v2f(1920, 1080);
      DEVMODEA dev_mode;
      if (EnumDisplaySettingsA(0, ENUM_CURRENT_SETTINGS, &dev_mode) &&
          (dev_mode.dmFields & DM_PELSWIDTH & DM_PELSHEIGHT)) 
      {
        resolution.x = (F32)dev_mode.dmPelsWidth;
        resolution.y = (F32)dev_mode.dmPelsHeight;
      }
      
      // NOTE(erb): renderer
      FILETIME shader_last_write_time = {0};
      {
        Scratch scratch = scratch_begin(&transient_arena);
        char *vertex_cpath = push_cstr(scratch.arena, vertex_shader_path);
        char *fragment_cpath = push_cstr(scratch.arena, fragment_shader_path);
        
        Read_File_Result vertex_file = w32_debug_read_entire_file(vertex_cpath);
        Read_File_Result fragment_file = w32_debug_read_entire_file(fragment_cpath);
        g_renderer = gl_make_renderer(&permanent_arena, 
                                      (char *)vertex_file.memory, 
                                      (char *)fragment_file.memory);
        w32_debug_free_file_memory(vertex_file.memory);
        w32_debug_free_file_memory(fragment_file.memory);
        
        w32_update_last_file_time(vertex_cpath, &shader_last_write_time);
        w32_update_last_file_time(fragment_cpath, &shader_last_write_time);
        scratch_end(&scratch);
      }
      
      w32_update_window(g_device_context, &g_renderer, v2i(1280, 900));
      
      // NOTE(erb): application memory setup
      App_Memory app_memory = {0};
      {
        app_memory.permanent_memory_size = mb(64);
        app_memory.permanent_memory = w32_allocate(app_memory.permanent_memory_size);
        
        app_memory.transient_memory_size = gb(1);
        app_memory.transient_memory = w32_allocate(app_memory.transient_memory_size);
      }
      
      U64 last_cycle_counter = __rdtsc();
      LARGE_INTEGER last_counter = w32_get_wall_clock();
      
      g_tick = 0;
      g_running = true;
      
      while (g_running) 
      {
        // NOTE(erb): update library
        if (w32_update_last_file_time(str_to_temp256_cstr(library_path),
                                      &app_library.last_write_time)) 
        {
          w32_unload_library(&app_library);
          app_library = w32_load_app_library(&transient_arena, library_path, temp_library_path);
        }
        
        // NOTE(erb): update shaders
        {
          Scratch scratch = scratch_begin(&transient_arena);
          char *vertex_cpath = push_cstr(scratch.arena, vertex_shader_path);
          char *fragment_cpath = push_cstr(scratch.arena, fragment_shader_path);
          
          if (w32_update_last_file_time(vertex_cpath, &shader_last_write_time) || 
              w32_update_last_file_time(fragment_cpath, &shader_last_write_time)) 
          {
            Read_File_Result vertex_file = w32_debug_read_entire_file(vertex_cpath);
            Read_File_Result fragment_file = w32_debug_read_entire_file(fragment_cpath);
            gl_update_shaders(&g_renderer, 
                              (char *)vertex_file.memory, 
                              (char *)fragment_file.memory);
            
            w32_debug_free_file_memory(vertex_file.memory);
            w32_debug_free_file_memory(fragment_file.memory);
          }
          scratch_end(&scratch);
        }
        
        // NOTE(erb): poll messages
        MSG message = {0};
        while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
        {
          if (message.message == WM_QUIT) 
          {
            g_running = 0;
          }
          
          TranslateMessage(&message);
          DispatchMessage(&message);
        }
        
        Input_State input = {0};
        
        V2i viewport = w32_get_frame_buffer_size(window_handle);
        
        // NOTE(erb): step frame
        {
          // NOTE(erb): begin frame
          input.screen_space_size.end = input.frame_buffer_size.end = w32_get_window_size(window_handle);
          g_renderer.batch_count = 0;
          g_renderer.rects.count = 0;
          g_renderer.screen_size = w32_get_window_size(window_handle);
          
          
          F32 gap = 20;
          F32 x_coord = 100;
          V2f size = v2ff(150);
          for (U32 idx = 0;
               idx < array_size(debug_textures);
               ++idx)
          {
            render_texture(&g_renderer, 
                           rect_top_left(v2f(x_coord, 100), size),
                           debug_textures[idx]);
            x_coord += (size.x + gap);
          }
          
#if 0
          Debug_Counters frame_debug_counters = {0};
          App_Update_Result update_res = app_library.update_and_render(&app_memory, 
                                                                       default_font, 
                                                                       &input, 
                                                                       &g_renderer, 
                                                                       frame_debug_counters);
          supress_unused(update_res);
#endif
        }
        
        // NOTE(erb): wait for frame time
        {
          F32 work_seconds_elapsed = w32_get_seconds_elapsed(last_counter, w32_get_wall_clock());
          if (work_seconds_elapsed < target_seconds_per_frame_goal) 
          {
            if (sleep_is_granular)
            {
              DWORD sleep_ms = (DWORD)((target_seconds_per_frame_goal - work_seconds_elapsed) * 1000.0f);
              if (sleep_ms > 0) 
              {
                Sleep(sleep_ms);
              }
            }
            
            //assert(w32_get_seconds_elapsed(last_counter, w32_get_wall_clock()) < target_seconds_per_frame_goal);
            
            while (work_seconds_elapsed < target_seconds_per_frame_goal)
            {
              work_seconds_elapsed = w32_get_seconds_elapsed(last_counter, w32_get_wall_clock());
            }
          }
          else
          {
            // NOTE(erb): missed frame
          }
        }
        
        // NOTE(erb): render
        {
          w32_update_window(g_device_context, &g_renderer, viewport);
        }
        
        // NOTE(erb): update counters and print debug
        {
          LARGE_INTEGER end_counter = w32_get_wall_clock();
          //F32 s_elapsed = w32_get_seconds_elapsed(last_counter, end_counter);
          last_counter = end_counter;
          
          U64 end_cycle_counter = __rdtsc();
          //U64 cycles_elapsed = end_cycle_counter - last_cycle_counter;
          last_cycle_counter = end_cycle_counter;
          
#if 0
          {
            F32 ms_elapsed = 1000.0f * s_elapsed;
            F32 fps = 1.0f / s_elapsed;
            I32 mega_cycles_per_frame = (I32)(cycles_elapsed / (1000 * 1000));
            char msg[256];
            sprintf_s(msg, 0 sizeof(msg), "%.2fms/f | %.2ff/s | %dmcyc/f \n", 
                      ms_elapsed, fps, mega_cycles_per_frame);
            OutputDebugStringA(msg);
          }
#endif
        }
        
        g_tick++;
      } 
      
      ReleaseDC(window_handle, g_device_context);
      wglDeleteContext(wglGetCurrentContext());
      DestroyWindow(window_handle);
    } 
    else 
    {
      // TODO(erb): log
    }
  } else 
  {
    // TODO(erb): log
  }
  
  return 0;
}
