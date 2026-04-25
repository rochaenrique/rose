/* date = April 22nd 2026 6:15 pm */

#ifndef WIN32_H
#define WIN32_H

#include "base/base_inc.h"
#include <windows.h>

typedef struct W32_GL_Window
{
  HWND window_handle;
  HGLRC gl_context;
} W32_GL_Window;

#if 0
#define UPDATE_AND_RENDER(name) App_Update_Result name(App_Memory *memory, \
Font_Data *default_font, \
Input_State *input, \
Render_Data *renderer, \
Debug_Counters frame_debug_counters)
typedef UPDATE_AND_RENDER(Update_And_Render_Func);

typedef struct W32_App_Library
{
  Update_And_Render_Func *update_and_render;
  HMODULE module;
  FILETIME last_write_time;
  B32 valid;
} W32_App_Library;
#endif

// TODO(erb): think of a better way of grouping clock/frame stuff together
// this is a really lazy way of doing it, sleep_is_granular, target_fps and target_seconds_per_frame are initalized once and the rest is calculated every frame. 
typedef struct W32_Counters
{
  B32 sleep_is_granular;
  U32 target_fps;
  F32 target_seconds_per_frame;
  
  U64 last_cycles;
  LARGE_INTEGER last_clock;
} W32_Counters;

// TODO(erb): move to os layer
typedef struct Codebase_File_Paths
{
  String build_dir;
  String code_dir;
  String data_dir;
  String project_dir;
  
  String executable;
  String default_font;
  String library;
  String temp_library;
  String vertex_shader;
  String fragment_shader;
} Codebase_File_Paths;

#endif //WIN32_H
