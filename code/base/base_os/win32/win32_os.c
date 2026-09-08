#include <timeapi.h>
global W32_State w32_state;

void *
os_memory_commit(U64 size)
{
  void *result = VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
  Assert(result); // TODO(erb): think of adding some error reporting here
  return result;
}

void *
os_memory_reserve(U64 size)
{
  void *result = VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
  Assert(result); // TODO(erb): think of adding some error reporting here
  return result;
}

void
os_memory_decommit(void *memory, U64 size)
{
  VirtualFree(memory, size, MEM_DECOMMIT);
}

void
os_memory_release(void *memory)
{
  VirtualFree(memory, 0, MEM_RELEASE);
}

B32
os_init()
{
  String8 error = {0};
  
  HINSTANCE instance = 0;
  {
    instance = GetModuleHandle(0);
    if(instance == 0)
    {
      error = S8("failed to get module instance");
    }
  }
  
  HANDLE stdout_handle = 0;
  if(error.value == 0)
  {
    stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (stdout_handle == INVALID_HANDLE_VALUE)
    {
      error = S8("failed to get stdout handle");
    }
  }
  
  DWORD thread_storage_index = 0;
  if(error.value == 0)
  {
    thread_storage_index = TlsAlloc();
    if (thread_storage_index == TLS_OUT_OF_INDEXES)
    {
      error = S8("failed to allocated thread local storage");
    }
  }
  
  U64 clock_frequency = 0;
  if (error.value == 0)
  {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    clock_frequency = freq.QuadPart;
  }
  
  U32 sleep_granularity_ms = 1;
  B32 sleep_is_granular = false;
  {
    sleep_is_granular = (timeBeginPeriod(sleep_granularity_ms) == TIMERR_NOERROR);
  }
  
  if (error.value == 0)
  {
    w32_state.instance             = instance;
    w32_state.stdout_handle        = stdout_handle;
    w32_state.clock_frequency      = clock_frequency;
    w32_state.thread_storage_index = thread_storage_index;
    w32_state.sleep_is_granular    = sleep_is_granular;
    w32_state.sleep_granularity_ms = sleep_granularity_ms;
  }
  else
  {
    err_set_error(error);
  }
  B32 result = (error.value == 0);
  return(result);
}

void
os_release(void)
{
  TlsFree(w32_state.thread_storage_index);
}

void 
os_log(String8 str)
{
  Scratch scratch = scratch_begin(0, 0);
  char *cstr = push_cstr(scratch.arena, str);
  WriteFile(w32_state.stdout_handle, cstr, (DWORD)str.size, 0, 0);
  WriteFile(w32_state.stdout_handle, "\n", 1, 0, 0);
  
#if 1
  OutputDebugStringA(cstr);
  OutputDebugStringA("\n");
#endif
  
  scratch_end(&scratch);
}

void 
os_logf(const char *format, ...)
{
  Scratch scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, format);
  String8 null_terminated_str = push_str8f_varg(scratch.arena, format, args);
  va_end(args);
  WriteFile(w32_state.stdout_handle, null_terminated_str.value, 
            (U32)null_terminated_str.size, 0, 0);
  WriteFile(w32_state.stdout_handle, "\n", 1, 0, 0);
  
#if DEBUG
  OutputDebugStringA((char *)null_terminated_str.value);
  OutputDebugStringA("\n");
#endif
  
  scratch_end(&scratch);
}

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

void 
os_debug_free_file_memory(void *memory)
{
  if (memory != 0)
  {
    VirtualFree(memory, 0, MEM_RELEASE);
  }
}

String8
os_debug_read_entire_file(String8 file_path)
{
  // TODO(erb): protect against size(file_name) < MAX_PATH
  // TODO(erb): add flags and have a "should null terminate" flag
  
  String8 result = {0};
  
  Scratch scratch = scratch_begin(0, 0);
  HANDLE file_handle = CreateFileA(push_cstr(scratch.arena, file_path),
                                   GENERIC_READ, FILE_SHARE_READ,
                                   0, OPEN_EXISTING,
                                   0, 0);
  scratch_end(&scratch);
  
  if (file_handle != INVALID_HANDLE_VALUE) 
  {
    LARGE_INTEGER file_size_result;
    if (GetFileSizeEx(file_handle, &file_size_result)) 
    {
      U32 size = safe_truncate_U64_to_U32(file_size_result.QuadPart);
      void *memory = os_memory_commit(size+1);
      DWORD bytes_read = 0;
      if (ReadFile(file_handle, memory, size, &bytes_read, 0) &&
          bytes_read == size)
      {
        result.value = memory;
        result.size = size;
        result.value[size] = 0;
      }
      else
      {
        os_debug_free_file_memory(memory);
        os_logf("Error: ReadFile() (%S)", file_path);
        InvalidCodePath;
      }
    }
    else
    {
      os_logf("Error: GetFileSizeEx() (%S)", file_path);
      InvalidCodePath;
    }
    
    if (!CloseHandle(file_handle))
    {
      os_logf("Error: CloseHandle() (%S)", file_path);
      InvalidCodePath;
    }
  }
  else
  {
    os_logf("Error: CreateFileA() (%S)", file_path);
    InvalidCodePath;
  }
  return(result);
}

String8 
os_read_entire_file_str8_arena(Arena *arena, String8 file_path)
{
  // TODO(erb): protect against size(file_name) < MAX_PATH
  // TODO(erb): add flags and have a "should null terminate" flag
  
  String8 result = {0};
  
  Scratch scratch = scratch_begin(&arena, 1);
  HANDLE file_handle = CreateFileA(push_cstr(scratch.arena, file_path),
                                   GENERIC_READ, FILE_SHARE_READ,
                                   0, OPEN_EXISTING,
                                   0, 0);
  scratch_end(&scratch);
  
  if (file_handle != INVALID_HANDLE_VALUE) 
  {
    LARGE_INTEGER file_size_result;
    if (GetFileSizeEx(file_handle, &file_size_result)) 
    {
      U32 size = safe_truncate_U64_to_U32(file_size_result.QuadPart);
      U8 *memory = PushArray(arena, size+1, U8);
      DWORD bytes_read = 0;
      if (ReadFile(file_handle, memory, size, &bytes_read, 0) &&
          bytes_read == size)
      {
        result = S8Struct(memory, size);
        result.value[size] = 0;
      }
      else
      {
        os_logf("Error: ReadFile() (%S)", file_path);
      }
    }
    else
    {
      os_logf("Error: GetFileSizeEx() (%S)", file_path);
    }
    
    if(!CloseHandle(file_handle))
    {
      os_logf("Error: CloseHandle() (%S)", file_path);
    }
  }
  else
  {
    os_logf("Error: CreateFileA() (%S)", file_path);
  }
  return(result);
}

B32 
os_debug_write_entire_file(String8 file_path, String8 content)
{
  // TODO(erb): protect against size(file_name) < MAX_PATH
  B32 result = false;
  Scratch scratch = scratch_begin(0, 0);
  HANDLE file_handle = CreateFileA(push_cstr(scratch.arena, file_path),
                                   GENERIC_WRITE, 
                                   0, 0, 
                                   CREATE_ALWAYS,
                                   0, 0);
  scratch_end(&scratch);
  
  if (file_handle != INVALID_HANDLE_VALUE) 
  {
    DWORD bytes_written;
    U32 memory_count = safe_truncate_U64_to_U32(content.size);
    if (WriteFile(file_handle, content.value, memory_count, &bytes_written, 0) &&
        bytes_written == memory_count)
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


void 
os_thread_context_set(Thread_Context *context)
{
  TlsSetValue(w32_state.thread_storage_index, (void *)context);
}

Thread_Context *
os_thread_context_get(void)
{
  Thread_Context *result = (Thread_Context *)TlsGetValue(w32_state.thread_storage_index);
  return result;
}

// TODO(erb): it would be really cool to have a "try to re-launch" option 
// here, which forks into a process that launches the application
void
os_handle_bad_error(String8 str)
{
  char msg[1024];
  sprintf_s(msg, sizeof(msg), "[ERROR] %.*s\n", S8Expand(str));
  OutputDebugStringA(msg);
  
  sprintf_s(msg, sizeof(msg), "%.*s\n", S8Expand(str));
  MessageBoxA(0, msg,
              "Error Ocurred", 
              MB_OK|MB_ICONERROR);
  ExitProcess(1);
}


B32
os_delete_directory(String8 path)
{
  B32 result = false;
  Scratch scratch = scratch_begin(0, 0);
  char *cstr_path = push_cstr(scratch.arena, path);
  if (RemoveDirectoryA(cstr_path) != 0) 
  {
    result = true;
  }
  else
  {
    // TODO(erb): get error information here and set error string
  }
  scratch_end(&scratch);
  return(result);
}


B32
os_delete_directory_recur(String8 path)
{
  B32 result = false;
  Scratch scratch = scratch_begin(0, 0);
  // NOTE(erb): double null termination
  U64 op_path_len = path.size + 2;
  char *op_path = op_path = PushArray(scratch.arena, op_path_len, char);
  mem_copy((U8 *)path.value, (U8 *)op_path, path.size);
  
  SHFILEOPSTRUCTA file_op = {0};
  file_op.wFunc = FO_DELETE;
  file_op.pFrom = op_path;
  file_op.fFlags = FOF_NO_UI;
  
  
  String8 error = {0};
  if (SHFileOperation(&file_op) != 0) 
  {
    // TODO(erb): get error information here and set error string
    error = S8("file operation failed");
  }
  if (error.value == 0 && file_op.fAnyOperationsAborted)
  {
    // TODO(erb): get error information here and set error string
    error = S8("file operation aborted");
  }
  
  if (error.value == 0)
  {
    result = true;
  }
  else
  {
    err_set_error(error);
  }
  scratch_end(&scratch);
  return(result);
}

B32
os_create_directory(String8 path)
{
  B32 result = false;
  Scratch scratch = scratch_begin(0, 0);
  char *cstr_path = push_cstr(scratch.arena, path);
  if (CreateDirectoryA(cstr_path,0) != 0)
  {
    result = true;
  }
  else 
  {
    // TODO(erb): get error information here and set error string
  }
  scratch_end(&scratch);
  return(result);
}


String8_List 
os_get_directory_paths(Arena *arena, String8 directory)
{
  String8_List result = {0};
  
  Scratch scratch = scratch_begin(&arena, 1);
  
  //NOTE(erb): trick for inserting a null terminating char
  U8 *search_path = str8_concat(scratch.arena, directory, S8("\\*\0")).value;
  WIN32_FIND_DATA find_data = {0};
  HANDLE find_handle = FindFirstFile((char *)search_path, &find_data);
  
  scratch_end(&scratch);
  
  if (find_handle != INVALID_HANDLE_VALUE)
  {
    for(;;)
    {
      String8_Node *node = PushStruct(arena, String8_Node);
      node->str = push_str8_from_cstr(arena, find_data.cFileName);
      SLLQueueAppend(result.first, result.last, node);
      
      if (!FindNextFile(find_handle, &find_data))
      {
        break;
      }
    }
  }
  else
  {
    InvalidCodePath;
    // TODO(erb): get error information here and ser error string
  }
  return(result);
}

String8_List 
os_get_directory_files(Arena *arena, String8 directory)
{
  String8_List result = {0};
  
  Scratch scratch = scratch_begin(&arena, 1);
  
  //NOTE(erb): trick for inserting a null terminating char
  U8 *search_path = str8_concat(scratch.arena, directory, S8("\\*\0")).value;
  WIN32_FIND_DATA find_data = {0};
  HANDLE find_handle = FindFirstFile((char *)search_path, &find_data);
  
  scratch_end(&scratch);
  
  if (find_handle != INVALID_HANDLE_VALUE)
  {
    for(;;)
    {
      if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
      {
        String8_Node *node = PushStruct(arena, String8_Node);
        node->str = push_str8_from_cstr(arena, find_data.cFileName);
        SLLQueueAppend(result.first, result.last, node);
      }
      
      if (!FindNextFile(find_handle, &find_data))
      {
        break;
      }
    }
  }
  else
  {
    // TODO(erb): get error information here and ser error string
  }
  return(result);
}

Directory_Entry_List
os_get_directory_entries(Arena *arena, String8 directory)
{
  Directory_Entry_List result = {0};
  result.path = directory;
  
  Scratch scratch = scratch_begin(&arena, 1);
  
  //NOTE(erb): trick for inserting a null terminating char
  U8 *search_path = str8_concat(scratch.arena, directory, S8("\\*\0")).value;
  WIN32_FIND_DATA find_data = {0};
  HANDLE find_handle = FindFirstFile((char *)search_path, &find_data);
  
  scratch_end(&scratch);
  
  if(find_handle != INVALID_HANDLE_VALUE)
  {
    for(;;)
    {
      Directory_Entry *entry = PushStruct(arena, Directory_Entry);
      entry->path = push_str8_from_cstr(arena, find_data.cFileName);
      entry->kind = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? 
                     FileKind_Directory : FileKind_Text);
      SLLQueueAppend(result.first, result.last, entry);
      result.count++;
      
      if(!FindNextFile(find_handle, &find_data))
      {
        break;
      }
    }
  }
  else
  {
    // TODO(erb): get error information here and ser error string
  }
  return(result);
}

B32
os_file_contains_str_case_insens(String8 path, String8 needle)
{
  Scratch scratch = scratch_begin(0, 0);
  String8 file_str = os_debug_read_entire_file(path);
  
  B32 result = str8_find(file_str, needle, MatchFlags_CaseInsens).exists;
  os_debug_free_file_memory(file_str.value);
  scratch_end(&scratch);
  return(result);
}

File_Kind
os_get_file_kind(String8 path)
{
  HANDLE find_handle = 0;
  WIN32_FIND_DATA find_data = {0};
  
  Scratch scratch = scratch_begin(0, 0);
  char *search_path = push_cstr(scratch.arena, path);
  find_handle = FindFirstFile(search_path, &find_data);
  scratch_end(&scratch);
  
  File_Kind result = FileKind_None;
  if (find_handle != INVALID_HANDLE_VALUE)
  {
    result = ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
              ? FileKind_Directory : FileKind_Text);
  }
  else
  {
    // TODO(erb): get error information here and ser error string
  }
  return(result);
}

B32
os_set_current_directory(String8 path)
{
  Scratch scratch = scratch_begin(0, 0);
  B32 result = (SetCurrentDirectory(push_cstr(scratch.arena, path)) != 0);
  scratch_end(&scratch);
  return(result);
}

String8
os_get_current_directory(Arena *arena)
{
  // NOTE(erb): from the docs: buffer_size is the needed buffer size 
  // including the null terminating character
  DWORD buffer_size_with_null_terminate = GetCurrentDirectory(0, 0);
  String8 result = push_str8(arena, buffer_size_with_null_terminate - 1);
  GetCurrentDirectory(buffer_size_with_null_terminate, (char *)result.value);
  return(result);
}

Full_Path
os_get_full_path(Arena *arena, String8 file_name)
{
  // NOTE(erb): from the docs: 
  // - nBufferLength (argument) : The size of the buffer to receive the null-terminated 
  //   string for the drive and path, in TCHARs.
  // - RETURN (bytes written): the length, in TCHARs, of the string copied to lpBuffer, not 
  //   including the terminating null character
  // This is why there is a -1 in the 'String8' size calculations.
  
  Scratch scratch = scratch_begin(&arena, 1);
  
  char *cstr_file = push_cstr(scratch.arena, file_name);
  U32 bytes_written = GetFullPathNameA(cstr_file, 0, 0, 0);
  char *buffer = PushArray(arena, bytes_written, char);
  char *file_part = 0;
  GetFullPathNameA(cstr_file, bytes_written, buffer, &file_part);
  
  scratch_end(&scratch);
  
  Full_Path result = {0};
  result.path = S8Struct(buffer, bytes_written -1);
  
  if (*file_part != 0)
  {
    U64 size = bytes_written - (file_part - buffer) - 1;
    result.file_part = S8Struct(file_part, size);
  }
  return(result);
}

String8
os_get_enviornment_variable(Arena *arena, char *name)
{
  String8 result = {0};
  DWORD bytes_written = GetEnvironmentVariable(name, 0, 0);
  if (bytes_written != 0)
  {
    result = push_str8(arena, bytes_written+1);
    result.size = GetEnvironmentVariable(name, (char *)result.value, safe_truncate_U64_to_U32(result.size));
  }
  return(result);
}

// NOTE(erb): returns raw performance counter
OS_Clock_Time
os_get_clock_time()
{
  LARGE_INTEGER t;
  QueryPerformanceCounter(&t);
  OS_Clock_Time result = (OS_Clock_Time)t.QuadPart;
  return(result);
}

F32 
os_seconds_elapsed(OS_Clock_Time begin, OS_Clock_Time end)
{
  Assert(w32_state.clock_frequency != 0);
  F32 result = ((F32)(end - begin) / w32_state.clock_frequency);
  return(result);
}


OS_Thread_Handle
os_thread_create_and_run(OS_Thread_Proc proc, void *data)
{
  HANDLE handle = CreateThread(0, 0,  //lpThreadAttributes, dwStackSize,
                               (LPTHREAD_START_ROUTINE)proc,
                               data,  //data
                               0, 0); //creationFlags (0 means run immediatly), threadId
  Assert(handle != INVALID_HANDLE_VALUE);
  return((OS_Thread_Handle)handle);
}

B32
os_thread_join_threads(OS_Thread_Handle *handles, U32 handle_count)
{
  HANDLE *w32_handles = (HANDLE *)handles;
  DWORD wait_result = WaitForMultipleObjects(handle_count, w32_handles, true, INFINITE);// waitAll, miliseconds
  B32 result = (wait_result < WAIT_OBJECT_0 || 
                wait_result >= WAIT_OBJECT_0 + handle_count);
  return(result);
}

void
os_sleep_ms(U32 ms)
{
  Assert(w32_state.sleep_is_granular);
  Assert(ms >= 0);
  Sleep(ms);
}

String8
w32_get_last_error_str8(Arena *arena, DWORD error_code)
{
  char *buffer = 0;
  DWORD size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER| 
                             FORMAT_MESSAGE_FROM_SYSTEM|
                             FORMAT_MESSAGE_IGNORE_INSERTS, //dwFlags
                             0, error_code, //lpSource, dwMessageId
                             MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), //dwLanguageId
                             (LPTSTR)&buffer, //lpBuffer
                             0, 0); //nSize, Arguments
  String8 str = push_str8(arena, size);
  ArrayCopy(buffer, str.value, size);
  LocalFree(buffer);
  return(str);
}

Date_Time
w32_system_time_to_date_time(SYSTEMTIME *st)
{
  Date_Time result = {0};
  result.date.year        = st->wYear; 
  result.date.month       = st->wMonth;
  //result.day_of_week = st->wDayOfWeek;
  result.date.day         = st->wDay;
  result.time.hour        = st->wHour;
  result.time.minute      = st->wMinute;
  result.time.second      = st->wSecond;
  result.time.ms          = st->wMilliseconds;
  return(result);
}

SYSTEMTIME
w32_date_time_to_system_time(Date_Time *dt)
{
  SYSTEMTIME result = {0};
  result.wYear         = (WORD)dt->date.year; 
  result.wMonth        = (WORD)dt->date.month;
  result.wDayOfWeek    = (WORD)weekday_from_days(cal_date_to_days(dt->date));
  result.wDay          = (WORD)dt->date.day;
  result.wHour         = (WORD)dt->time.hour;
  result.wMinute       = (WORD)dt->time.minute;
  result.wSecond       = (WORD)dt->time.second;
  result.wMilliseconds = (WORD)dt->time.ms;
  return(result);
}

Date_Time
os_get_utc_date_time()
{
  SYSTEMTIME st = {0};
  GetSystemTime(&st);
  Date_Time result = w32_system_time_to_date_time(&st);
  return(result);
}

Date_Time
os_get_local_date_time()
{
  SYSTEMTIME st = {0};
  GetLocalTime(&st);
  Date_Time result = w32_system_time_to_date_time(&st);
  return(result);
}