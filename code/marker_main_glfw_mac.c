#include <stdio.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <limits.h>
#include <copyfile.h>
#include <mach-o/dyld.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <unistd.h>
#include <mach/mach_time.h>

#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <OpenGL/gl3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "core.c"
#include "platform.c"
#include "debug.h"

#include <ft2build.h>
#include FT_FREETYPE_H

B32 is_glfw_initialized;
B32 is_opengl_initialized;

#define glfw_context(glfw_window) defer_loop(glfwMakeContextCurrent(glfw_window), glfwMakeContextCurrent(0))

typedef struct GLFW_Payload GLFW_Payload;
struct GLFW_Payload
{
  Input_Event event_stream[64];
  U32 event_stream_count;
};

typedef struct OSX_Window OSX_Window;
struct OSX_Window
{
  Window_ID id;
  
  Input_State input;
  Q2D_State renderer;
  
  struct OSX_Window *next;
  struct OSX_Window *hash_next;
  
  Font_Data *default_font;
  
  GLFW_Payload glfw_payload;
  GLFWwindow *glfw;
};

typedef struct OSX_State OSX_State;
struct OSX_State
{
  String8 default_font_path;
  
  String8 vertex_shader_path;
  String8 fragment_shader_path;
  I64 shaders_load_time;
  
  String8 input_file_path;
  I32 input_file_fd;
  Debug_State kind;
  
  Arena permanent_arena;
  Arena transient_arena;
  
  U32 next_window_id;
  OSX_Window *window_table[8];
  
  OSX_Window *first;
  OSX_Window *first_free;
};

typedef struct OSX_App_Library OSX_App_Library;
struct OSX_App_Library
{
  void *handle;
  Update_And_Render_Func *update_and_render;
  I64 load_time;
};

typedef struct OSX_Read_File_Result OSX_Read_File_Result;
struct OSX_Read_File_Result
{
  void *data;
  U64 size;
  I64 load_time;
};

void *osx_allocate(U64 size)
{
  void *result = 0;
  I32 protection = PROT_READ|PROT_WRITE;
  I32 flags = MAP_PRIVATE|MAP_ANON;
  I32 fd = -1;
  I32 offset = 0;
  
#if 0
  // NOTE(erb): start_address and MAP_FIXED flag fix the address for the allocation (for debuging)
  void *start_address = (void *)0x1;
  result = mmap(start_address, size, protection, flags|MAP_FIXED, fd, offset);
#else 
  result = mmap(0, size, protection, flags, fd, offset);
#endif
  
  pf_Assert(result);
  return result;
}

void osx_free(void *memory, U64 size)
{
  I32 result = munmap(memory, size);
  pf_Assert(result != 0);
}

Date osx_tm_to_date(struct tm *time)
{
	Date result = {0};
	result.seconds  = time->tm_sec;
	result.minutes  = time->tm_min;
	result.hours    = time->tm_hour;
	result.month_day = time->tm_mday;
	result.month    = time->tm_mon;
	result.year     = time->tm_year + 1900;
	result.week_day  = time->tm_wday;
	result.year_day  = time->tm_yday;
	return result;
}

Date osx_get_today()
{
	struct timeval today;
	pf_Assert(gettimeofday(&today, 0) == 0);
	struct tm *time = localtime(&today.tv_sec);
	
	Date result = osx_tm_to_date(time);
	return result;
}

I32 osx_ensure_open(String8 path, I32 flags)
{
  I32 result = open(str_to_temp512_cstr(path), flags);
  if (result == -1)
  {
    pf_log("Failed to get file stat (%.*s): %s\n", S8Expand(path), strerror(errno));
    pf_Assert(0);
  }
  return result;
}

OSX_Read_File_Result osx_read_file(Arena *arena, String8 file_path)
{
  OSX_Read_File_Result result = {};
  
  I32 file_descriptor = open(str_to_temp512_cstr(file_path), O_RDONLY);
  if (file_descriptor != -1)
  {
    struct stat file_stat = {};
    if (fstat(file_descriptor, &file_stat) == 0)
    {
      U64 size = (U64)file_stat.st_size;
      void *buffer = (void *)PushArray(arena, size+1, U8);
      
      I64 bytes_read = read(file_descriptor, buffer, size);
      pf_Assert(bytes_read != -1);
      ((U8 *)buffer)[size] = 0;
      
      result.data = buffer;
      result.size = size;
      result.load_time = file_stat.st_atimespec.tv_sec;
    }
    else
    {
      pf_log("Failed to get file stat (%.*s): %s\n", S8Expand(file_path), strerror(errno));
    }
    
    close(file_descriptor);
  }
  else
  {
    pf_log("Failed to open file (%.*s): %s\n", S8Expand(file_path), strerror(errno));
  }
  
  return result;
}

OSX_Read_File_Result fallback_vertex_shader(OSX_Read_File_Result file)
{
  if (!file.data)
  {
    file.data = (U8 *)fallback_vertex_shader_source;
    file.size = S8(fallback_vertex_shader_source).length;
  }
  return file;
}

OSX_Read_File_Result fallback_fragment_shader(OSX_Read_File_Result file)
{
  if (!file.data)
  {
    file.data = (U8 *)fallback_fragment_shader_source;
    file.size = S8(fallback_fragment_shader_source).length;
  }
  return file;
}

FT_Library freetype_library;

Font_Data *osx_freetype_load_font(Arena *arena, String8 path)
{
  if (freetype_library == 0) 
  {
    if (FT_Init_FreeType(&freetype_library))
    {
      pf_log("Failed to initialize freetype!\n");
      pf_Assert(false);
    }
  }
  
  FT_Face font_face;
  
  Scratch scratch = scratch_begin(arena);
  {
    char *cstr_path = push_cstr(scratch.arena, path);
    if (FT_New_Face(freetype_library, cstr_path, 0, &font_face))
    {
      pf_log("Failed to load font (%s)!\n", cstr_path);
      pf_Assert(false);
    }
  }
  scratch_end(&scratch);
  
  FT_Set_Pixel_Sizes(font_face, 0, 48);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  
  Font_Data *font_data = PushStruct(arena, Font_Data);
  font_data->height = font_face->height >> 6;
  font_data->below_base_line_offset = -(font_face->bbox.yMin >> 6);
  font_data->above_base_line_offset = font_face->bbox.yMax >> 6;
  
  for (U32 char_idx = 0;
       char_idx < ArraySize(font_data->ascii_glyph_table);
       char_idx++) 
  {
    if (!FT_Load_Char(font_face, char_idx, FT_LOAD_RENDER))
    {
      
      FT_Glyph_Metrics metrics = font_face->glyph->metrics;
      Glyph_Info info = {0};
      info.advance_x = metrics.horiAdvance >> 6;
      info.bearing = v2i(metrics.horiBearingX >> 6, metrics.horiBearingY >> 6);
      info.size = v2i(metrics.width >> 6, metrics.height >> 6);
      info.texture_id = texture_id;
      
      U32 texture_id;
      gl(glGenTextures(1, &texture_id));
      gl(glBindTexture(GL_TEXTURE_2D, texture_id));
      gl(glTexImage2D(GL_TEXTURE_2D, 
                      0, 
                      GL_R8, 
                      font_face->glyph->bitmap.width,
                      font_face->glyph->bitmap.rows,
                      0,
                      GL_RED,
                      GL_UNSIGNED_BYTE, 
                      font_face->glyph->bitmap.buffer));
      
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ONE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ONE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ONE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
      
      gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
      gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
      gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
      gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
      
      font_data->ascii_glyph_table[char_idx] = info;
    }
    else
    {
      pf_log("Error: Freetype: Failed to load Glyph %c\n", char_idx);
    }
  }
  
  FT_Done_Face(font_face);
  
  return font_data;
}

void verify_key_codes_with_glfw_keys()
{
  pf_Assert(KeyCode_Space == GLFW_KEY_SPACE);
  pf_Assert(KeyCode_Apostrophe == GLFW_KEY_APOSTROPHE);
  pf_Assert(KeyCode_Comma == GLFW_KEY_COMMA);
  pf_Assert(KeyCode_Minus == GLFW_KEY_MINUS);
  pf_Assert(KeyCode_Period == GLFW_KEY_PERIOD);
  pf_Assert(KeyCode_Slash == GLFW_KEY_SLASH);
  pf_Assert(KeyCode_Zero == GLFW_KEY_0);
  pf_Assert(KeyCode_One == GLFW_KEY_1);
  pf_Assert(KeyCode_Two == GLFW_KEY_2);
  pf_Assert(KeyCode_Three == GLFW_KEY_3);
  pf_Assert(KeyCode_Four == GLFW_KEY_4);
  pf_Assert(KeyCode_Five == GLFW_KEY_5);
  pf_Assert(KeyCode_Six == GLFW_KEY_6);
  pf_Assert(KeyCode_Seven == GLFW_KEY_7);
  pf_Assert(KeyCode_Eight == GLFW_KEY_8);
  pf_Assert(KeyCode_Nine == GLFW_KEY_9);
  pf_Assert(KeyCode_Semicolon == GLFW_KEY_SEMICOLON);
  pf_Assert(KeyCode_Equal == GLFW_KEY_EQUAL);
  pf_Assert(KeyCode_A == GLFW_KEY_A);
  pf_Assert(KeyCode_B == GLFW_KEY_B);
  pf_Assert(KeyCode_C == GLFW_KEY_C);
  pf_Assert(KeyCode_D == GLFW_KEY_D);
  pf_Assert(KeyCode_E == GLFW_KEY_E);
  pf_Assert(KeyCode_F == GLFW_KEY_F);
  pf_Assert(KeyCode_G == GLFW_KEY_G);
  pf_Assert(KeyCode_H == GLFW_KEY_H);
  pf_Assert(KeyCode_I == GLFW_KEY_I);
  pf_Assert(KeyCode_J == GLFW_KEY_J);
  pf_Assert(KeyCode_K == GLFW_KEY_K);
  pf_Assert(KeyCode_L == GLFW_KEY_L);
  pf_Assert(KeyCode_M == GLFW_KEY_M);
  pf_Assert(KeyCode_N == GLFW_KEY_N);
  pf_Assert(KeyCode_O == GLFW_KEY_O);
  pf_Assert(KeyCode_P == GLFW_KEY_P);
  pf_Assert(KeyCode_Q == GLFW_KEY_Q);
  pf_Assert(KeyCode_R == GLFW_KEY_R);
  pf_Assert(KeyCode_S == GLFW_KEY_S);
  pf_Assert(KeyCode_T == GLFW_KEY_T);
  pf_Assert(KeyCode_U == GLFW_KEY_U);
  pf_Assert(KeyCode_V == GLFW_KEY_V);
  pf_Assert(KeyCode_W == GLFW_KEY_W);
  pf_Assert(KeyCode_X == GLFW_KEY_X);
  pf_Assert(KeyCode_Y == GLFW_KEY_Y);
  pf_Assert(KeyCode_Z == GLFW_KEY_Z);
  pf_Assert(KeyCode_LeftBracket == GLFW_KEY_LEFT_BRACKET);
  pf_Assert(KeyCode_Backslash == GLFW_KEY_BACKSLASH);
  pf_Assert(KeyCode_RightBracket == GLFW_KEY_RIGHT_BRACKET);
  pf_Assert(KeyCode_Backtick == GLFW_KEY_GRAVE_ACCENT);
  pf_Assert(KeyCode_Escape == GLFW_KEY_ESCAPE);
  pf_Assert(KeyCode_Enter == GLFW_KEY_ENTER);
  pf_Assert(KeyCode_Tab == GLFW_KEY_TAB);
  pf_Assert(KeyCode_Backspace == GLFW_KEY_BACKSPACE);
  pf_Assert(KeyCode_Insert == GLFW_KEY_INSERT);
  pf_Assert(KeyCode_Delete == GLFW_KEY_DELETE);
  pf_Assert(KeyCode_Right == GLFW_KEY_RIGHT);
  pf_Assert(KeyCode_Left == GLFW_KEY_LEFT);
  pf_Assert(KeyCode_Down == GLFW_KEY_DOWN);
  pf_Assert(KeyCode_Up == GLFW_KEY_UP);
  pf_Assert(KeyCode_PageUp == GLFW_KEY_PAGE_UP);
  pf_Assert(KeyCode_PageDown == GLFW_KEY_PAGE_DOWN);
  pf_Assert(KeyCode_Home == GLFW_KEY_HOME);
  pf_Assert(KeyCode_End == GLFW_KEY_END);
  pf_Assert(KeyCode_CapsLock == GLFW_KEY_CAPS_LOCK);
  pf_Assert(KeyCode_ScrollLock == GLFW_KEY_SCROLL_LOCK);
  pf_Assert(KeyCode_NumLock == GLFW_KEY_NUM_LOCK);
  pf_Assert(KeyCode_PrintScreen == GLFW_KEY_PRINT_SCREEN);
  pf_Assert(KeyCode_Pause == GLFW_KEY_PAUSE);
  pf_Assert(KeyCode_F1 == GLFW_KEY_F1);
  pf_Assert(KeyCode_F2 == GLFW_KEY_F2);
  pf_Assert(KeyCode_F3 == GLFW_KEY_F3);
  pf_Assert(KeyCode_F4 == GLFW_KEY_F4);
  pf_Assert(KeyCode_F5 == GLFW_KEY_F5);
  pf_Assert(KeyCode_F6 == GLFW_KEY_F6);
  pf_Assert(KeyCode_F7 == GLFW_KEY_F7);
  pf_Assert(KeyCode_F8 == GLFW_KEY_F8);
  pf_Assert(KeyCode_F9 == GLFW_KEY_F9);
  pf_Assert(KeyCode_F10 == GLFW_KEY_F10);
  pf_Assert(KeyCode_F11 == GLFW_KEY_F11);
  pf_Assert(KeyCode_F12 == GLFW_KEY_F12);
  pf_Assert(KeyCode_LeftShift == GLFW_KEY_LEFT_SHIFT);
  pf_Assert(KeyCode_LeftControl == GLFW_KEY_LEFT_CONTROL);
  pf_Assert(KeyCode_LeftAlt == GLFW_KEY_LEFT_ALT);
  pf_Assert(KeyCode_LeftSuper == GLFW_KEY_LEFT_SUPER);
  pf_Assert(KeyCode_RightShift == GLFW_KEY_RIGHT_SHIFT);
  pf_Assert(KeyCode_RightControl == GLFW_KEY_RIGHT_CONTROL);
  pf_Assert(KeyCode_RightAlt == GLFW_KEY_RIGHT_ALT);
  pf_Assert(KeyCode_RightSuper == GLFW_KEY_RIGHT_SUPER);
}

Modifier_Flags modifiers_from_glfw_mods(I32 mods)
{
  Modifier_Flags result = 0;
  
  if (mods & GLFW_MOD_SHIFT)
  {
    result |= ModifierFlags_Shift;
  }
  if (mods & GLFW_MOD_CONTROL)
  {
    result |= ModifierFlags_Control;
  }
  if (mods & GLFW_MOD_ALT)
  {
    result |= ModifierFlags_Alt;
  }
  if (mods & GLFW_MOD_SUPER)
  {
    result |= ModifierFlags_Super;
  }
  if (mods & GLFW_MOD_CAPS_LOCK)
  {
    result |= ModifierFlags_Caps;
  }
  if (mods & GLFW_MOD_NUM_LOCK)
  {
    result |= ModifierFlags_NumLock;
  }
  
  return result;
};

Key_Code glfw_key_to_key_code(I32 key)
{
  Key_Code result = KeyCode_None;
  
  if (0 < key && key < KeyCode_COUNT) 
  {
    result = (Key_Code)key;
  }
  
  return result;
}

Mouse_Code mouse_code_from_glfw(I32 button) 
{
  Mouse_Code code = MouseCode_None;
  switch (button)
  {
    case GLFW_MOUSE_BUTTON_LEFT: 
    {
      code = MouseCode_Left;
    } break;
    
    case GLFW_MOUSE_BUTTON_RIGHT: 
    {
      code = MouseCode_Right;
    } break;
    
    case GLFW_MOUSE_BUTTON_MIDDLE: 
    {
      code = MouseCode_Middle;
    } break;
  }
  
  return code;
}


Input_Event *push_payload_stream_event(GLFWwindow *glfw_window)
{
  GLFW_Payload *payload = (GLFW_Payload *)glfwGetWindowUserPointer(glfw_window);
  pf_Assert(payload);
  pf_Assert(payload->event_stream_count <= ArraySize(payload->event_stream));
  
  Input_Event *event = (payload->event_stream + payload->event_stream_count++);
  return event;
}

void key_callback(GLFWwindow *glfw_window, int key, int scancode, int action, int mods) 
{
  // NOTE(erb): scancode is ignored for now
  SupressUnused(scancode);
  
  Key_Code code = glfw_key_to_key_code(key);
  if (code != KeyCode_None && (action == GLFW_PRESS || action == GLFW_RELEASE))
  {
    Input_Event *event = push_payload_stream_event(glfw_window);
    if (action == GLFW_PRESS) 
    {
      event->kind = InputEventKind_KeyDown;
    }
    else if (action == GLFW_RELEASE) 
    {
      event->kind = InputEventKind_KeyUp;
    }
    event->key.code = code;
    event->key.modifiers = modifiers_from_glfw_mods(mods);
  }
}

void character_callback(GLFWwindow* glfw_window, unsigned int codepoint)
{
  Input_Event *event = push_payload_stream_event(glfw_window);
  event->kind = InputEventKind_Text;
  event->code_point = codepoint;
}

void mouse_callback(GLFWwindow* glfw_window, int button, int action, int mods)
{
  Mouse_Code code = mouse_code_from_glfw(button);
  
  if (code != MouseCode_None)
  {
    Input_Event *event = push_payload_stream_event(glfw_window);
    if (action == GLFW_PRESS) 
    {
      event->kind = InputEventKind_MouseDown;
    }
    else if (action == GLFW_RELEASE) 
    {
      event->kind = InputEventKind_MouseUp;
    }
    event->mouse.code = code;
    event->mouse.modifiers = modifiers_from_glfw_mods(mods);
  }
}

void mouse_scroll_callback(GLFWwindow* glfw_window, double xoffset, double yoffset)
{
  Input_Event *event = push_payload_stream_event(glfw_window);
  event->kind = InputEventKind_MouseWheel;
  event->wheel_delta = v2f((F32)xoffset, (F32)yoffset);
};

void cursor_position_callback(GLFWwindow* glfw_window, double xpos, double ypos)
{
  Input_Event *event = push_payload_stream_event(glfw_window);
  event->kind = InputEventKind_CursorMove;
  event->cursor_position = v2f((F32)xpos, (F32)ypos);
};

void glfw_error_callback(int error_code, const char* description)
{
  pf_log("GLFW ERROR (%d): '%s'\n", error_code, description);
}

V2i glfw_frame_buffer_size(GLFWwindow *window)
{
  I32 width, height;
  glfwGetFramebufferSize(window, &width, &height);
  V2i result = v2i(width, height);
  return result;
}

V2i glfw_window_size(GLFWwindow *window)
{
  I32 width, height;
  glfwGetWindowSize(window, &width, &height);
  V2i result = v2i(width, height);
  return result;
}

V2f glfw_cursor_position(GLFWwindow *window) 
{
  F64 x, y;
  glfwGetCursorPos(window, &x, &y);
  V2f result = v2f((F32)x, (F32)y);
  return result;
}

void osx_glfw_init_window(OSX_Window *window, String8 title, V2i size)
{
  if (glfwInit())
  {
    verify_key_codes_with_glfw_keys();
    
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE); // NOTE(erb): apple forward compatibilty
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    glfwSetErrorCallback(glfw_error_callback);
    
    // NOTE(erb): glfw_info
    pf_log("LOG: GLFW Compilation Version %i.%i.%i\n", 
           GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, GLFW_VERSION_REVISION);
    I32 major, minor, revision;
    glfwGetVersion(&major, &minor, &revision);
    printf("LOG: GLFW Runtime Version %i.%i.%i\n", major, minor, revision);
    
    GLFWwindow* glfw_window = glfwCreateWindow(size.x, size.y, str_to_temp256_cstr(title), 0, 0);
    if (glfw_window) 
    {
      glfw_context(glfw_window)
      {
        glfwSwapInterval(0); // NOTE(erb): no vsync
        
        is_opengl_initialized = true;
        // NOTE(erb): opengl info
        pf_log("LOG: OpenGL version: %s\n", glGetString8(GL_VERSION));
        pf_log("LOG: OpenGL Vendor: %s\n", glGetString8(GL_VENDOR));
        pf_log("LOG: OpenGL Renderer: %s\n", glGetString8(GL_RENDERER));
        pf_log("LOG: OpenGL Version: %s\n", glGetString8(GL_VERSION));
        pf_log("LOG: OpenGL GLSL Version: %s\n", glGetString8(GL_SHADING_LANGUAGE_VERSION));
        I32 max_vertex_attribs;
        gl(glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attribs));
        pf_log("LOG: OpenGL max vertex_shader Attributes Allowed: %d\n", max_vertex_attribs);
        
        window->glfw = (void *)glfw_window;
        window->input.screen_space_size.end = glfw_window_size(glfw_window);
        window->input.frame_buffer_size.end = glfw_frame_buffer_size(glfw_window);
        window->input.cursor_reverse_y.end = glfw_cursor_position(glfw_window);
        
        // NOTE(erb): callbacks
        glfwSetKeyCallback(glfw_window, key_callback);
        glfwSetCharCallback(glfw_window, character_callback);
        glfwSetMouseButtonCallback(glfw_window, mouse_callback);
        glfwSetCursorPosCallback(glfw_window, cursor_position_callback);
        glfwSetScrollCallback(glfw_window, mouse_scroll_callback);
        
        // NOTE(erb): opengl setup
        gl(glEnable(GL_BLEND));
        gl(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
      }
    }
  }
  else
  {
    pf_log("Failed to initialized glfw");
    pf_Assert(false);
  }
}

void close_glfw_window(OSX_Window *window)
{
  glfwDestroyWindow(window->glfw);
}

U64 cycle_counter()
{
  U64 val;
  asm volatile("mrs %0, cntvct_el0" : "=r" (val));
  return val;
}

U64 osx_timer_ns()
{
  mach_timebase_info_data_t info;
  kern_return_t kern_result = mach_timebase_info(&info);
  pf_Assert(kern_result == KERN_SUCCESS);
  
  U64 raw = mach_Absolute_time();
  U64 nanos = raw * info.numer / info.denom;
  
#if 0
  struct timespec get_time_s;
  clock_gettime(CLOCK_UPTIME_RAW, &get_time_s);
  
  U64 get_time_ns = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
#endif
  
  U64 result = nanos;
  return result;
}

UPDATE_AND_RENDER(update_and_render_stub) 
{
  // NOTE(erb): empty
  SupressUnused(memory);
  SupressUnused(frame_debug_counters);
  SupressUnused(exec_directory);
  
  App_Update_Result result = {0};
  return result;
}

B32 osx_has_file_changed(String8 file_path, I64 compare_to_time)
{
  B32 result = 0;
  
  struct stat file_stat = {};
  I32 error = stat(str_to_temp512_cstr(file_path), &file_stat);
  if (error == 0)
  {
    result = file_stat.st_mtimespec.tv_sec > compare_to_time;
  }
  else
  {
    pf_log("Failed to get file stats (%.*s)\n", S8Expand(file_path));
  }
  
  return result;
}

void osx_copy_file(String8 source_path, String8 dest_path)
{
  I32 source_fd = open(str_to_temp512_cstr(source_path), O_RDONLY);
  if (source_fd != -1) 
  {
    I32 dest_fd = open(str_to_temp512_cstr(dest_path), O_CREAT|O_TRUNC|O_WRONLY, S_IRWXU);
    if (dest_fd != -1)
    {
      I32 result = fcopyfile(source_fd, dest_fd, 0, COPYFILE_ALL);
      if (result < 0) 
      {
        pf_log("Failed to copy (%.*s) to (%.*s)\n", S8Expand(source_path), S8Expand(dest_path));
      }
      
      close(source_fd);
    }
    else
    {
      pf_log("Failed to open (%.*s)\n", S8Expand(dest_path));
    }
    
    close(source_fd);
  }
  else
  {
    pf_log("Failed to open (%.*s)\n", S8Expand(source_path));
  }
}

OSX_App_Library osx_load_app_library(String8 library_path, String8 temp_library_path)
{
  OSX_App_Library result = {0};
  result.update_and_render = update_and_render_stub;
  
  struct stat file_stat = {};
  I32 stat_error = stat(str_to_temp512_cstr(library_path), &file_stat);
  if (stat_error == 0)
  {
    // NOTE(erb): copy the file to a temporary path to avoid lock
    osx_copy_file(library_path, temp_library_path);
    
    void *handle = dlopen(str_to_temp512_cstr(temp_library_path), 0);
    if (handle)
    {
      result.update_and_render = (Update_And_Render_Func *)dlsym(handle, "update_and_render");
      result.handle = handle;
      result.load_time = file_stat.st_mtimespec.tv_sec;
    }
    else
    {
      pf_log("Failed to load library dll (%.*s), original: (%.*s)\n", 
             S8Expand(temp_library_path), 
             S8Expand(library_path));
    }
    
    if (!result.update_and_render) 
    {
      result.update_and_render = update_and_render_stub;
    }
  }
  
  return result;
}

void osx_unload_app_library(OSX_App_Library *library)
{
  I32 error = dlclose(library->handle);
  if (error)
  {
    pf_log("Failed to unload library\n");
  }
  
  library->handle = 0;
  library->update_and_render = update_and_render_stub;
}

String8 osx_get_executable_directory(Arena *arena)
{
  char path_buffer[PATH_MAX];
  U32 path_size = PATH_MAX;
  B32 error = _NSGetExecutablePath(path_buffer, &path_size);
  pf_Assert(!error);
  
  // NOTE(erb): get the directory
  String8 path = {0};
  path.value = path_buffer;
  path.length = cstr_length(path.value);
  
  Index_U64 path_sep = str_index_char_backward(path, '/');
  pf_Assert(path_sep.exists);
  
  String8 result = str_copy(arena, str_prefix(path, path_sep.value));
  return result;
}

Input_V2i v2i_input_add_end(Input_V2i current, V2i new_end)
{
  Input_V2i result = current;
  result.delta = v2i_add(current.delta, v2i_sub(new_end, current.end));
  result.end = new_end;
  return result;
}

Input_V2f v2f_input_add_end(Input_V2f current, V2f new_end)
{
  Input_V2f result = current;
  result.delta = v2f_add(current.delta, v2f_sub(new_end, current.end));
  result.end = new_end;
  return result;
}

void update_input_from_glfw_event_stream(GLFW_Payload *payload, Input_State *input)
{
  for (U32 event_idx = 0;
       event_idx < payload->event_stream_count;
       event_idx++) 
  {
    Input_Event *event = (payload->event_stream + event_idx);
    
    pf_Assert(input->event_stream_count <= ArraySize(input->event_stream));
    Input_Event *event_copy = (input->event_stream + input->event_stream_count++);
    *event_copy = *event;
    event_copy->next = 0;
    
    switch (event->kind)
    {
      case InputEventKind_None: pf_Assert(false && "Got a InputEventKind_None");
      
      case InputEventKind_Text: break;
      
      case InputEventKind_WindowClose:
      {
        input->should_kill = true;
      } break;
      
      case InputEventKind_WindowResize:
      {
        input->screen_space_size = v2i_input_add_end(input->screen_space_size, 
                                                     event->window_resize.screen_space);
        input->frame_buffer_size = v2i_input_add_end(input->frame_buffer_size, 
                                                     event->window_resize.frame_buffer);
      } break;
      
      case InputEventKind_KeyDown:
      case InputEventKind_KeyUp:
      {
        Input_Button *key = (input->keys + event->key.code);
        key->half_transition_count++;
        key->ended_down = (event->kind == InputEventKind_KeyDown);
        input->modifiers = event->key.modifiers;
      } break;
      
      case InputEventKind_MouseDown:
      case InputEventKind_MouseUp:
      {
        Input_Button *button = (input->mouse_buttons + event->mouse.code);
        button->half_transition_count++;
        button->ended_down = (event->kind == InputEventKind_MouseDown);
        input->modifiers = event->key.modifiers;
      } break;
      
      case InputEventKind_CursorMove:
      {
        input->cursor_reverse_y = v2f_input_add_end(input->cursor_reverse_y, 
                                                    event->cursor_position);
      } break;
      
      case InputEventKind_MouseWheel:
      {
        input->scroll_offset = v2f_add(input->scroll_offset, event->wheel_delta);
      } break;
    }
  }
}

void begin_event_record(OSX_State *state)
{
  pf_log("Started Recording (%.*s)\n", S8Expand(state->input_file_path));
  state->kind = DebugState_Recording;
  state->input_file_fd = open(str_to_temp512_cstr(state->input_file_path), O_CREAT|O_TRUNC|O_WRONLY, S_IRWXU);
  pf_Assert(state->input_file_fd != -1);
}

void end_event_record(OSX_State *state)
{
  pf_log("Ended Recording (%.*s)\n", S8Expand(state->input_file_path));
  state->kind = DebugState_None;
  I32 result = close(state->input_file_fd);
  state->input_file_fd = -1;
  pf_Assert(result != -1);
}

void begin_event_play_back(OSX_State *state)
{
  pf_log("Started Playback (%.*s)\n", S8Expand(state->input_file_path));
  state->kind = DebugState_Playing;
  state->input_file_fd = open(str_to_temp512_cstr(state->input_file_path), O_RDONLY);
  pf_Assert(state->input_file_fd != -1);
}

void end_event_play_back(OSX_State *state)
{
  pf_log("Ended Playback (%.*s)\n", S8Expand(state->input_file_path));
  state->kind = DebugState_None;
  I32 result = close(state->input_file_fd);
  state->input_file_fd = -1;
  pf_Assert(result != -1);
}

void read_input_play_back(OSX_State *state, Input_State *input)
{
  I64 bytes_read = read(state->input_file_fd, (void *)input, sizeof(Input_State));
  pf_Assert(bytes_read == 0 || bytes_read == sizeof(Input_State));
  
  if(bytes_read == 0)
  {
    lseek(state->input_file_fd, 0, SEEK_SET);
  }
}

void write_input_recording(OSX_State *state, Input_State *input)
{
  I64 bytes_written = write(state->input_file_fd, (void *)input, sizeof(Input_State));
  pf_Assert(bytes_written == sizeof(Input_State));
}


void clear_input(Input_State *input)
{
  // NOTE(erb): clear input transitions/delta
  input->event_stream_count = 0;
  mem_zero((U8 *)input->event_stream, ArraySize(input->event_stream)*sizeof(Input_Event));
  input->screen_space_size.delta = v2ii(0);
  input->frame_buffer_size.delta = v2ii(0);
  input->cursor_reverse_y.delta = v2ff(0);
  input->scroll_offset = v2ff(0);
  
  for (U32 key_idx = 0;
       key_idx < ArraySize(input->keys);
       key_idx++)
  {
    input->keys[key_idx].half_transition_count = 0;
  }
  
  for (U32 button_idx = 0;
       button_idx < ArraySize(input->mouse_buttons);
       button_idx++)
  {
    input->mouse_buttons[button_idx].half_transition_count = 0;
  }
}

void osx_frame_begin(OSX_State *state)
{
  
  for (OSX_Window *window = state->first;
       window;
       window = window->next)
  {
    // NOTE(erb): set the user pointer again
    glfwSetWindowUserPointer(window->glfw, &window->glfw_payload);
    
  }
  
  // NOTE(erb): poll all window events
  glfwPollEvents();
  
  for (OSX_Window *window = state->first;
       window;
       window = window->next)
  {
    // NOTE(erb): collect glfw events
    {
      V2i old_screen_space = window->input.screen_space_size.end;
      V2i old_frame_buffer = window->input.frame_buffer_size.end;
      V2i new_screen_space = glfw_window_size(window->glfw);
      V2i new_frame_buffer = glfw_frame_buffer_size(window->glfw);
      
      if (!v2i_equals(old_screen_space, new_screen_space) || 
          !v2i_equals(old_frame_buffer, new_frame_buffer))
      {
        Input_Event *event = push_payload_stream_event(window->glfw);
        event->kind = InputEventKind_WindowResize;
        event->window_resize.screen_space = new_screen_space;
        event->window_resize.frame_buffer = new_frame_buffer;
      }
      
      if (glfwWindowShouldClose(window->glfw)) 
      {
        Input_Event *event = push_payload_stream_event(window->glfw);
        event->kind = InputEventKind_WindowClose;
      }
    }
    
    // NOTE(erb): clear past frame window events
    clear_input(&window->input);
    
    // NOTE(erb): update events
    update_input_from_glfw_event_stream(&window->glfw_payload, &window->input);
    window->glfw_payload.event_stream_count = 0;
    
    // NOTE(erb): clear renderer
    Q2D_State *renderer = &window->renderer;
    {
      renderer->screen_size = window->input.screen_space_size.end;
      renderer->frame_buffer_size = window->input.frame_buffer_size.end;
      
      renderer->rects.count = 0;
      renderer->batch_count = 1;
      renderer->batches->start_rect_index = 0;
      renderer->batches->texture_count = 0;
    }
  }
  
#if 0
  // TODO(erb): fix input streaming
  // the issue is that since there is more than one window, we don't know which input corresponds to which window.
  // and since there can be more than one window, reading is variable length
  // 
  // A possible simple solution is to just stream inputs per window (one file per window), 
  // and hash inputs with either window id or window title
  
  Input_State glfw_input = window->input;
  update_input_from_glfw_event_stream(&window->payload, &glfw_input);
  
  B32 pressed_record_key = false;
  B32 pressed_play_key = false;
  
  pressed_record_key = (pressed_record_key || 
                        key_pressed_modded(&glfw_input, KeyCode_L, ModifierFlags_Control));
  pressed_play_key = (pressed_play_key || 
                      key_pressed_modded(&glfw_input, KeyCode_U, ModifierFlags_Control));
  
  if (pressed_record_key)
  {
    switch (state->kind) 
    {
      case DebugState_None: begin_event_record(state); break;
      case DebugState_Recording: end_event_record(state); break;
      case DebugState_Playing: break;
    }
  }
  else if (pressed_play_key)
  {
    switch (state->kind) 
    {
      case DebugState_Recording: break;
      case DebugState_None: begin_event_play_back(state); break;
      case DebugState_Playing: end_event_play_back(state); break;
    }
  }
  
  if (state->kind == DebugState_Playing)
  {
    read_input_play_back(state, input);
  }
  else
  {
    *input = glfw_input;
  }
  
  if (state->kind == DebugState_Recording)
  {
    write_input_recording(state, &window->input);
  }
#endif
  
}

void osx_frame_end(OSX_State *state)
{
  for (OSX_Window *window = state->first;
       window;
       window = window->next)
  {
    glfw_context(window->glfw)
    {
      Q2D_State *renderer = &window->renderer;
      renderer->frame_buffer_size = window->input.frame_buffer_size.end;
      renderer->frame_buffer_size = window->input.frame_buffer_size.end;
      renderer->screen_size =  window->input.screen_space_size.end;
      
      gl_render(renderer);
      
      glfwSwapBuffers(window->glfw);
    }
  }
}

void osx_attach_renderer(OSX_State *state, OSX_Window *window)
{
  glfw_context(window->glfw)
  {
    window->renderer = opengl_make_renderer(&state->permanent_arena, 
                                            state->vertex_shader_path, 
                                            state->fragment_shader_path, 
                                            &state->shaders_load_time);
  }
}

void osx_attach_font(OSX_State *state, OSX_Window *window)
{
  glfw_context(window->glfw)
  {
    window->default_font = osx_freetype_load_font(&state->permanent_arena, state->default_font_path);
  }
}

OSX_State *g_state;

PLATFORM_MAKE_WINDOW(osx_make_window)
{
  // NOTE(erb): allocation
  OSX_Window *window = 0;
  {
    if (g_state->first_free != 0) 
    {
      window = g_state->first_free;
      g_state->first_free = window->next;
    }
    else
    {
      window = PushStruct(&g_state->permanent_arena, OSX_Window);
    }
    window->next = g_state->first;
    g_state->first = window;
  }
  pf_Assert(window);
  
  // NOTE(erb): initialization
  {
    osx_glfw_init_window(window, title, size);
    osx_attach_renderer(g_state, window);
    osx_attach_font(g_state, window);
  }
  
  // NOTE(erb): id assignment
  g_state->next_window_id = CapBottom(g_state->next_window_id, 1);
  window->id = g_state->next_window_id++;
  
  // NOTE(erb): table insertion
  I32 hash_idx = window->id % ArraySize(g_state->window_table);
  OSX_Window **hash_slot = (g_state->window_table + hash_idx);
  if (*hash_slot != 0)
  {
    window->hash_next = (*hash_slot);
  }
  *hash_slot = window;
  
  return window->id;
}

OSX_Window *get_window_from_id(OSX_State *state, Window_ID id)
{
  OSX_Window *result = 0;
  if (id != 0)
  {
    I32 hash_idx = id % ArraySize(state->window_table);
    OSX_Window **hash_slot = (g_state->window_table + hash_idx);
    if (*hash_slot != 0)
    {
      for (OSX_Window *test_window = *hash_slot;
           test_window;
           test_window = test_window->next)
      {
        if (test_window->id == id) 
        {
          result = test_window;
          break;
        }
      }
    }
  }
  pf_Assert(result);
  return result;
}

PLATFORM_GET_WINDOW_EVENTS8(osx_get_window_events)
{
  OSX_Window *window = get_window_from_id(g_state, handle);
  Input_State *input = &window->input;
  return input;
}

PLATFORM_GET_RENDERER_BUFFER(osx_get_renderer_buffer)
{
  OSX_Window *window = get_window_from_id(g_state, handle);
  Q2D_State *renderer = &window->renderer;
  return renderer;
}

PLATFORM_GET_DEFAULT_FONT(osx_get_default_font)
{
  OSX_Window *window = get_window_from_id(g_state, handle);
  Font_Data *font = window->default_font;
  return font;
}

PLATFORM_GET_CLIPBOARD_STRING(osx_get_clipboard_str)
{
  OSX_Window *window = get_window_from_id(g_state, handle);
  const char* cstr = glfwGetClipboardString8(window->glfw);
  String8 result = push_cstr_copy(arena, (char *)cstr);
  return result;
}

PLATFORM_SET_CLIPBOARD_STRING(osx_set_clipboard_str)
{
  OSX_Window *window = get_window_from_id(g_state, handle);
  char *cstr = str_to_temp512_cstr(str);
  glfwSetClipboardString8(window->glfw, cstr);	
}

int main(void)
{
  OSX_State osx_state = {0};
  g_state = &osx_state;
  
  // NOTE(erb): platform arenas 
  {
    U64 permanent_arena_size = MB(64);
    void *permanent_arena_memory = osx_allocate(permanent_arena_size);
    arena_init(&osx_state.permanent_arena, permanent_arena_memory, permanent_arena_size);
    
    U64 transient_arena_size = MB(128);
    void *transient_arena_memory = osx_allocate(transient_arena_size);
    arena_init(&osx_state.transient_arena, transient_arena_memory, transient_arena_size);
  }
  
  // NOTE(erb): executable directory
  String8 exec_directory = osx_get_executable_directory(&osx_state.permanent_arena);
  
  // NOTE(erb): paths setup
  osx_state.vertex_shader_path = str_concat(&osx_state.permanent_arena, exec_directory, S8("/vertex_shader_rect.glsl"));
  osx_state.fragment_shader_path = str_concat(&osx_state.permanent_arena, exec_directory, S8("/fragment_shader_rect.glsl"));
  osx_state.input_file_path = str_concat(&osx_state.permanent_arena, exec_directory, S8("/input_record.dump"));
  osx_state.input_file_fd = -1;
  osx_state.default_font_path = str_concat(&osx_state.permanent_arena, exec_directory, S8("/arial.ttf"));
  String8 library_path = str_concat(&osx_state.permanent_arena, exec_directory, S8("/app.dylib"));
  String8 temp_library_path = str_concat(&osx_state.permanent_arena, exec_directory, S8("/temp_app.dylib"));
  
  // NOTE(erb): first app dll load
  OSX_App_Library app_library = osx_load_app_library(library_path, temp_library_path);
  
  // NOTE(erb): platform functions
  {
    g_platform.get_clipboard_str = osx_get_clipboard_str;
    g_platform.set_clipboard_str = osx_set_clipboard_str;
    g_platform.make_window = osx_make_window;
    g_platform.get_window_events = osx_get_window_events;
    g_platform.get_renderer_buffer = osx_get_renderer_buffer;
    g_platform.get_default_font = osx_get_default_font;
  }
  
  // NOTE(erb): application memory setup
  App_Memory memory = {0};
  {
    memory.permanent_memory_size = MB(64);
    memory.permanent_memory = osx_allocate(memory.permanent_memory_size);
    
    memory.transient_memory_size = GB(1);
    memory.transient_memory = osx_allocate(memory.transient_memory_size);
    
    memory.platform = g_platform;
  }
  
  Debug_Counters debug_counters = {0};
  
  F64 fps_goal = 240.0;
  F64 frame_time_goal_ns = 10e9/fps_goal;
  
  for (;;)
  {
    High_Freq_Time frame_start_time = high_freq_time_from_ns(osx_timer_ns());
    
    Debug_Counters start_counters = {0};
    {
      
      struct rusage usage;
      I32 result = getrusage(RUSAGE_SELF, &usage);
      pf_Assert(result == 0);
      
      start_counters.frame_time_ns = osx_timer_ns();
      start_counters.cycles = cycle_counter();
      start_counters.page_reclaims = usage.ru_minflt;
      start_counters.page_faults = usage.ru_majflt;
      start_counters.swaps = usage.ru_nswap;
      start_counters.fs_input = usage.ru_inblock;
      start_counters.fs_output = usage.ru_oublock;
    }
    
    App_Update_Result update_result = {0};
    
    {
      osx_frame_begin(&osx_state);
      update_result = app_library.update_and_render(&memory, 
                                                    exec_directory,
                                                    debug_counters);
      osx_frame_end(&osx_state);
    }
    
    // NOTE(erb): hot reloading
    {
      if (osx_has_file_changed(library_path, app_library.load_time))
      {
        osx_unload_app_library(&app_library);
        app_library = osx_load_app_library(library_path, temp_library_path);
      }
      
      if (osx_has_file_changed(osx_state.vertex_shader_path, osx_state.shaders_load_time) ||
          osx_has_file_changed(osx_state.fragment_shader_path, osx_state.shaders_load_time))
      {
        for (OSX_Window *window = osx_state.first;
             window;
             window = window->next)
        {
          window->renderer.program = make_shaderes_program(&osx_state.transient_arena, 
                                                           osx_state.vertex_shader_path, 
                                                           osx_state.fragment_shader_path, 
                                                           false, 
                                                           &osx_state.shaders_load_time);
        }
      }
    }
    
    arena_release(&osx_state.transient_arena);
    
    High_Freq_Time frame_end_time = high_freq_time_from_ns(osx_timer_ns());
    F64 frame_time_left_ns = frame_time_goal_ns - (frame_end_time.ns - frame_start_time.ns);
    if(frame_time_left_ns > 0.0f)
    {
      High_Freq_Time frame_time_left = high_freq_time_from_ns(frame_time_left_ns);
      struct timespec tp;
      tp.tv_sec = (U64)frame_time_left.s;
      tp.tv_nsec = frame_time_left.ns % (U64)1000000000;
      nanosleep(&tp, 0);
    }
    
    {
      struct rusage usage;
      I32 result = getrusage(RUSAGE_SELF, &usage);
      pf_Assert(result == 0);
      
      debug_counters.frame_time_ns = osx_timer_ns() - start_counters.frame_time_ns;
      debug_counters.cycles = cycle_counter() - start_counters.cycles;
      debug_counters.page_reclaims = usage.ru_minflt - start_counters.page_reclaims;
      debug_counters.page_faults = usage.ru_majflt - start_counters.page_faults;
      debug_counters.swaps = usage.ru_nswap - start_counters.swaps;
      debug_counters.fs_input = usage.ru_inblock - start_counters.fs_input;
      debug_counters.fs_output = usage.ru_oublock - start_counters.fs_output;
    }
    
    if (update_result.should_kill)
    {
      break;
    }
  }
  
  for (OSX_Window *window = osx_state.first;
       window;
       window = window->next)
  {
    gl(glDeleteVertexArrays(1, &window->renderer.vao));
    gl(glDeleteBuffers(1, &window->renderer.vbo));
    gl(glDeleteProgram(window->renderer.program));
    close_glfw_window(window);
  }
  FT_Done_FreeType(freetype_library);
  glfwTerminate();
  return 0;
}
