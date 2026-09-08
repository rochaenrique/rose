/* date = April 22nd 2026 6:15 pm */

#ifndef WIN32_OS_H
#define WIN32_OS_H

// NOTE(erb): this is a hack
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <shellapi.h>
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdI32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shell32.lib")

typedef struct W32_State W32_State;
struct W32_State
{
  DWORD thread_storage_index;
  HANDLE stdout_handle;
  HINSTANCE instance;
  U64 clock_frequency;
  B32 sleep_is_granular;
  U32 sleep_granularity_ms;
};

#define OS_ENTRY_POINT() \
int __stdcall WinMain(HINSTANCE instance, \
HINSTANCE prev_instance, \
LPSTR cmd_line, int show) \

#define GET_PROC_ADDR(MODULE, VALUE, STR) (*(PROC *)(&(VALUE)) = GetProcAddress((MODULE), (STR)))

// TODO(erb): think of a better way of grouping clock/frame stuff together
// this is a really lazy way of doing it, sleep_is_granular, target_fps and target_seconds_per_frame are initalized once and the rest is calculated every frame. 
typedef struct W32_Counters W32_Counters;
struct W32_Counters
{
  B32 sleep_is_granular;
  U32 target_fps;
  F32 target_seconds_per_frame;
  
  U64 last_cycles;
  LARGE_INTEGER last_clock;
};

#endif //WIN32_OS_H

