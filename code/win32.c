void *
w32_memory_commit(U64 size)
{
  void *result = VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
  assert(result); // TODO(erb): think of adding some error reporting here
  return result;
}

Codebase_File_Paths
w32_get_codebase_file_paths(Arena *arena, HINSTANCE instance)
{
  char module_path_buffer[256];
  DWORD module_path_length = GetModuleFileNameA(instance,
                                                module_path_buffer,
                                                array_size(module_path_buffer));
  Codebase_File_Paths paths = {0};
  
  char sep = '\\';
  
  if (module_path_length != 0) 
  {
    paths.executable = str_copy_buffer_length(arena, module_path_buffer, module_path_length);
    
    paths.build_dir = str_pop_dir(paths.executable, sep);
    paths.code_dir = str_pop_dir(paths.build_dir, sep);
    paths.project_dir = str_pop_dir(paths.code_dir, sep);
    
    paths.data_dir = str_concat(arena, paths.project_dir, S("\\data"));
    
    paths.default_font = str_concat(arena, paths.data_dir, S("\\calibri.ttf"));
    
    paths.library = str_concat(arena, paths.build_dir, S("\\marker_app.dll"));
    paths.temp_library = str_concat(arena, paths.build_dir, S("\\marker_app_temp.dll"));
    
    paths.vertex_shader = str_concat(arena, paths.code_dir, S("\\marker_vertex_shader_rect.glsl"));
    paths.fragment_shader = str_concat(arena, paths.code_dir, S("\\marker_fragment_shader_rect.glsl"));
  }
  
  if (module_path_length == 0)
  {
    // TODO(erb): add some terminal error message here
    fprintf(stdout, "failed to get module file name\n");
    ExitProcess(1);
  }
  
  return paths;
}

FILETIME 
w32_get_last_write_time(char *path)
{
  FILETIME result = {0};
  WIN32_FIND_DATA find_data;
  HANDLE find_handle = FindFirstFileA(path, &find_data);
  if (find_handle != INVALID_HANDLE_VALUE) 
  {
    result = find_data.ftLastWriteTime;
    FindClose(find_handle);
  }
  return result;
}

#if 0
// TODO(erb): think of error reporting here
W32_App_Library 
w32_load_app_library(String path, String temp_path)
{
  W32_App_Library library = {0};
  
  Scratch scratch = scratch_begin(0, 0);
  char *cpath = push_cstr(scratch.arena, path);
  char *temp_cpath = push_cstr(scratch.arena, temp_path);
  
  if (!CopyFileA(cpath, temp_cpath, false))
  {
    // TODO(erb): log error
    OutputDebugStringA("w32_load_library CopyFileA() failed\n");
  }
  
  library.last_write_time = w32_get_last_write_time(cpath);
  library.module = LoadLibraryA(temp_cpath);
  if (library.module != 0) 
  {
    library.update_and_render = (Update_And_Render_Func *)GetProcAddress(library.module, "update_and_render");
    library.valid = (library.update_and_render != 0);
  }
  else
  {
    OutputDebugStringA("w32_load_library LoadLibraryA() failed\n");
  }
  
  if (library.valid == false) 
  {
    // TODO(erb): have a stub here
  }
  
  scratch_end(&scratch);
  return library;
}
#endif

I32
w32_get_display_refresh_rate()
{
  I32 refresh_rate = 0;
  DEVMODEA dev_mode;
  if (EnumDisplaySettingsA(0, ENUM_CURRENT_SETTINGS, &dev_mode) &&
      (dev_mode.dmFields & DM_DISPLAYFREQUENCY)) 
  {
    // TODO(erb): think of error reporing here with fallback
    refresh_rate = dev_mode.dmDisplayFrequency;
  }
  return refresh_rate;
}

LARGE_INTEGER 
w32_get_wall_clock()
{
  LARGE_INTEGER result;
  QueryPerformanceCounter(&result);
  return result;
}

global U64 w32_clock_frequency = 0;

F32 w32_get_seconds_elapsed(LARGE_INTEGER begin, LARGE_INTEGER end)
{
  assert(w32_clock_frequency != 0);
  F32 result = ((F32)(end.QuadPart - begin.QuadPart) / w32_clock_frequency);
  return result;
}

// TODO(erb): think of a better way to split this stuff. 
W32_Counters
w32_init_counters()
{
  W32_Counters counters = {0};
  
  //- clock frequency
  LARGE_INTEGER freq_result;
  QueryPerformanceFrequency(&freq_result);
  w32_clock_frequency = freq_result.QuadPart;
  
  //-granular sleep
  U32 desired_scheduler_ms = 1;
  counters.sleep_is_granular = (timeBeginPeriod(desired_scheduler_ms) == TIMERR_NOERROR);
  counters.target_seconds_per_frame = 1.0f/(F32)w32_get_display_refresh_rate();
  
  return counters;
}

B32
w32_wait_for_target_frame_time(W32_Counters *counters)
{
  B32 skipped_frame = false;
  
  F32 work_seconds_elapsed = w32_get_seconds_elapsed(counters->last_clock, w32_get_wall_clock());
  if (work_seconds_elapsed < counters->target_seconds_per_frame) 
  {
    if (counters->sleep_is_granular)
    {
      DWORD sleep_ms = (DWORD)((counters->target_seconds_per_frame - work_seconds_elapsed) * 1000.0f);
      if (sleep_ms > 0) 
      {
        Sleep(sleep_ms);
      }
    }
    
    //assert(w32_get_seconds_elapsed(last_counter, w32_get_wall_clock()) < target_seconds_per_frame_goal);
    
    while (work_seconds_elapsed < counters->target_seconds_per_frame)
    {
      work_seconds_elapsed = w32_get_seconds_elapsed(counters->last_clock, w32_get_wall_clock());
    }
  }
  else
  {
    // NOTE(erb): missed frame
    // TODO(erb): think about handling this???
    skipped_frame = true;
  }
  
  return skipped_frame;
}

void
w32_gather_counters(W32_Counters *counters)
{
  LARGE_INTEGER end_counter = w32_get_wall_clock();
  F32 s_elapsed = w32_get_seconds_elapsed(counters->last_clock, end_counter);
  counters->last_clock = end_counter;
  
  U64 end_cycle_counter = __rdtsc();
  U64 cycles_elapsed = (end_cycle_counter - counters->last_cycles);
  counters->last_cycles = end_cycle_counter;
  {
    F32 ms_elapsed = 1000.0f * s_elapsed;
    F32 fps = 1.0f / s_elapsed;
    I32 mega_cycles_per_frame = (I32)(cycles_elapsed / (1000 * 1000));
    fprintf(stdout, "%.2fms/f | %.2ff/s | %dmcyc/f \n", ms_elapsed, fps, mega_cycles_per_frame);
  }
}

void
w32_pre_first_frame(W32_Counters *counters)
{
  counters->last_cycles = __rdtsc();
  counters->last_clock = w32_get_wall_clock();
}


void w32_debug_free_file_memory(void *memory)
{
  if (memory != 0)
  {
    VirtualFree(memory, 0, MEM_RELEASE);
  }
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

B32 w32_debug_write_entire_file(char *file_path, void *memory, U32 memory_size)
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

