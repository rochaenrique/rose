#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <gl/gl.h>

typedef uint8_t u8;
typedef int8_t i8;

typedef uint8_t u16;
typedef int8_t i16;

typedef uint32_t u32;
typedef int32_t i32;

typedef uint64_t u64;
typedef int64_t i64;

typedef uint32_t b8;
typedef uint32_t b16;
typedef uint32_t b32;

typedef float f32;
typedef double f64;

#define true 1
#define false 0

#define assert(condition) if (!(condition)) {*(int *)0 = 0;}
#define unused(expr) ((void)(expr))

static int g_running = true;

typedef struct Win32_Screen_Buffer
{
  BITMAPINFO info;
  void *memory;
  int width;
  int height;
  int bytes_per_pixel;
  int pitch;
} Win32_Screen_Buffer;

typedef struct Win32_Window_Dimension
{
  int width;
  int height;
} Win32_Window_Dimension;

static Win32_Screen_Buffer g_screen_buffer;
static u64 g_perf_count_freq;

Win32_Window_Dimension win32_get_window_dimension(HWND window)
{
  RECT client_rect;
  GetClientRect(window, &client_rect);
  
  Win32_Window_Dimension result;
  result.height = client_rect.bottom - client_rect.top;
  result.width = client_rect.right - client_rect.left;
  return result;
}

LARGE_INTEGER win32_get_wall_clock()
{
  LARGE_INTEGER result;
  QueryPerformanceCounter(&result);
  return result;
}

f32 win32_get_seconds_elapsed(LARGE_INTEGER begin, LARGE_INTEGER end)
{
  f32 result = ((f32)(end.QuadPart - begin.QuadPart) / g_perf_count_freq);
  return result;
}

void render_weird_gradient(Win32_Screen_Buffer *buffer, int x_offset, int y_offset)
{
  u8 *row = (u8 *)buffer->memory;
  for (int y = 0;
       y < buffer->height;
       ++y) 
  {
    u32 *pixel = (u32 *)row;
    for (int x = 0;
         x < buffer->width;
         ++x) 
    {
      /*
memory:    BB GG RR xx
register:  xx RR GG BB
*/
      u8 blue = (u8)(x + x_offset);
      u8 green = (u8)(y + y_offset);
      *pixel++ = (u32)(green << 8 | blue);
    }
    row += buffer->pitch;
  }
}

void win32_initialize_opengl(HWND window_handle)
{
  HDC window_dc = GetDC(window_handle);
  HGLRC opengl_rc = wglCreateContext(window_dc);
  if (wglMakeCurrent(window_dc, opengl_rc))
  {
  }
  else
  {
    assert(false);
  }
  ReleaseDC(window_handle, window_dc);
}

void win32_resize_dib_section(Win32_Screen_Buffer *buffer, int width, int height)
{
  if (buffer->memory) 
  {
    VirtualFree(buffer->memory, 0, MEM_RELEASE);
  }
  
  buffer->width = width;
  buffer->height = height;
  buffer->bytes_per_pixel = 4;
  buffer->pitch = buffer->width * buffer->bytes_per_pixel;
  
  buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
  buffer->info.bmiHeader.biWidth = buffer->width;
  buffer->info.bmiHeader.biHeight = -buffer->height;
  buffer->info.bmiHeader.biPlanes = 1;
  buffer->info.bmiHeader.biBitCount = 32;
  buffer->info.bmiHeader.biCompression = BI_RGB;
  
  int bitmap_memory_size = (buffer->width * buffer->height * buffer->bytes_per_pixel);
  buffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
}

void win32_update_window(HDC device_context, Win32_Screen_Buffer *buffer, int width, int height)
{
#if 0
  StretchDIBits(device_context,
                0, 0, width, height,
                0, 0, buffer->width, buffer->height,
                buffer->memory,
                &buffer->info,
                DIB_RGB_COLORS,
                SRCCOPY);
#else
  unused(device_context);
  unused(buffer);
  unused(width);
  unused(height);
  
  glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  SwapBuffers(device_context);
#endif
}

LRESULT CALLBACK win32_main_window_callback(HWND window,
                                            UINT message,
                                            WPARAM w_param,
                                            LPARAM l_param)
{
  char msg[256];
  LRESULT result = 0;
  switch (message)
  {
    case WM_SIZE: 
    {
      OutputDebugStringA("Message WM_SIZE\n");
    } break;
    
    case WM_CLOSE: 
    case WM_DESTROY: 
    {         
      g_running = 0;
    } break;
    
    case WM_ACTIVATEAPP: 
    {
      OutputDebugStringA("Message WM_ACTIVATEAPP\n");
    } break;
    
    case WM_MOUSEMOVE:
    {
      u32 lower_16 = ((1 << 16) - 1);
      // NOTE(erb): values in pixels
      i32 x_pos = (i32)(l_param & lower_16);
      i32 y_pos = (i32)(l_param >> 16);
      
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
      u32 lower_16 = ((1 << 16) - 1);
      // NOTE(erb): values in pixels
      i32 x_pos = (i32)(l_param & lower_16);
      i32 y_pos = (i32)(l_param >> 16);
      
#if 0
      b8 control_down = (w_param & MK_CONTROL);
      b8 shift_down = (w_param & MK_SHIFT);
      
      b8 left_down = (w_param & MK_LBUTTON);
      b8 middle_down = (w_param & MK_MBUTTON);
      b8 right_down = (w_param & MK_RBUTTON);
      b8 xbutton1_down = (w_param & MK_XBUTTON1);
      b8 xbutton2_down = (w_param & MK_XBUTTON2);
#endif
      
      sprintf_s(msg, sizeof(msg), "mouse_move = (%d, %d)\n", x_pos, y_pos);
      OutputDebugStringA(msg);
    } break;
    
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
      u32 virtual_key_code = (u32)w_param;
      
      b8 was_down = (l_param & (1 << 30)) != 0;
      b8 is_down = (l_param & (1 << 31)) == 0;
      
#if 0
      u32 mask_16 = ((1 << 16) - 1);
      b8 is_extended = (l_param & (1 << 24)) != 0; // NOTE(erb): leflt alt, left control, etc
      b8 key_repeat = (was_down == is_down);
      u32 repeat_count = (l_param & mask_16);
      u32 scan_code = ((l_param >> 16) & mask_16);
#endif
      
      sprintf_s(msg, sizeof(msg), 
                "key = '%c', was_down = %d, is_down = %d\n", 
                virtual_key_code, was_down, is_down);
      OutputDebugStringA(msg);
    } break;
    
    
    case WM_CHAR:
    { 
      u32 mask_8 = 0xFF;
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
      PAINTSTRUCT paint = {0};
      HDC device_context = BeginPaint(window, &paint);
      {
        Win32_Window_Dimension dimension = win32_get_window_dimension(window);
        win32_update_window(device_context, &g_screen_buffer, dimension.width, dimension.height);
      }
      BOOL paint_result = EndPaint(window, &paint);
      if (paint_result == 0) 
      {
        OutputDebugStringA("EndPaint Zero\n");
      }
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
  u32 size;
} Read_File_Result;

void debug_free_file_memory(void *memory)
{
  if (memory != 0)
  {
    VirtualFree(memory, 0, MEM_RELEASE);
  }
}

u32 safe_truncate_u64_to_u32(u64 value)
{
  assert(value <= 0xFFFFFFFF);
  u32 result = (u32)value;
  return result;
}

Read_File_Result debug_read_entire_file(char *file_path)
{
  // TODO(erb): protect against size(file_name) < MAX_PATH
  
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
      u32 size = safe_truncate_u64_to_u32(file_size_result.QuadPart);
      void *memory = VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
      if (memory)
      {
        DWORD bytes_read = 0;
        if (ReadFile(file_handle, memory, size, &bytes_read, 0) &&
            bytes_read == size)
        {
          result.memory = memory;
          result.size = size;
        }
        else
        {
          debug_free_file_memory(memory);
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

b32 debug_write_entire_file(char *file_path, void *memory, u32 memory_size)
{
  // TODO(erb): protect against size(file_name) < MAX_PATH
  b32 result = false;
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

int __stdcall WinMain(HINSTANCE instance, 
                      HINSTANCE prev_instance, 
                      LPSTR cmd_line, 
                      int show)
{
  unused(prev_instance);
  unused(cmd_line);
  unused(show);
  
  win32_resize_dib_section(&g_screen_buffer, 1280, 720);
  
  WNDCLASSA window_class = {0};
  window_class.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
  window_class.hInstance = instance;
  window_class.lpfnWndProc = win32_main_window_callback;
  window_class.lpszClassName = "MainWindowClass";
  
  LARGE_INTEGER perf_count_freq_result;
  QueryPerformanceFrequency(&perf_count_freq_result);
  g_perf_count_freq = perf_count_freq_result.QuadPart;
  
  u32 desired_scheduler_ms = 1;
  b32 sleep_is_granular = (timeBeginPeriod(desired_scheduler_ms) == TIMERR_NOERROR);
  
  i32 monitor_hz = 60;
  f32 target_seconds_per_frame_goal = 1.0f/(f32)monitor_hz;
  
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
      int x_offset = 0;
      int y_offset = 0;
      
      HDC device_context = GetDC(window_handle);
      
      u64 last_cycle_counter = __rdtsc();
      LARGE_INTEGER last_counter = win32_get_wall_clock();
      
      while (g_running) 
      {
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
        
        render_weird_gradient(&g_screen_buffer, x_offset, y_offset);
        
        ++x_offset;
        ++y_offset;
        
        // NOTE(erb): wait for frame time
        {
          f32 work_seconds_elapsed = win32_get_seconds_elapsed(last_counter, win32_get_wall_clock());
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
            
            //assert(win32_get_seconds_elapsed(last_counter, win32_get_wall_clock()) < target_seconds_per_frame_goal);
            
            while (work_seconds_elapsed < target_seconds_per_frame_goal)
            {
              work_seconds_elapsed = win32_get_seconds_elapsed(last_counter, win32_get_wall_clock());
            }
          }
          else
          {
            // NOTE(erb): missed frame
          }
        }
        
        // NOTE(erb): render
        {
          Win32_Window_Dimension dimension = win32_get_window_dimension(window_handle);
          win32_update_window(device_context, 
                              &g_screen_buffer,
                              dimension.width, dimension.height);
        }
        
        
        // NOTE(erb): update counters and print debug
        {
          LARGE_INTEGER end_counter = win32_get_wall_clock();
          f32 ms_elapsed = 1000.0f * win32_get_seconds_elapsed(last_counter, end_counter);
          last_counter = end_counter;
          
          u64 end_cycle_counter = __rdtsc();
          u64 cycles_elapsed = end_cycle_counter - last_cycle_counter;
          last_cycle_counter = end_cycle_counter;
          {
            f32 fps = 0.0f;
            i32 mega_cycles_per_frame = (i32)(cycles_elapsed / (1000 * 1000));
            char msg[256];
            sprintf_s(msg, sizeof(msg), "%.2fms/f | %.2ff/s | %dmcyc/f \n", ms_elapsed, fps, mega_cycles_per_frame);
            OutputDebugStringA(msg);
          }
        }
        
      } 
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
