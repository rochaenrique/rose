#ifndef OS_H
#define OS_H

/////////////////////////////////////////////////////////
// NOTE(erb): basic
B32     os_init(void);
void    os_release(void);
void    os_handle_bad_error(String8 error);
void    os_log(String8 str);
void    os_logf(const char *format, ...);

/////////////////////////////////////////////////////////
// NOTE(erb): thread 
typedef struct Thread_Context Thread_Context;
struct Thread_Context
{
  Arena arenas[4];
};
typedef void* OS_Thread_Handle;
typedef U32 OS_Thread_Proc(void *data);

Thread_Context  *os_thread_context_get(void);
void             os_thread_context_set(Thread_Context *context);
OS_Thread_Handle os_thread_create_and_run(OS_Thread_Proc *proc, void *data);
B32              os_thread_join_threads(OS_Thread_Handle *handles, U32 handle_count);
Scratch          scratch_begin(Arena **conflicts, U32 conflict_count);

/////////////////////////////////////////////////////////
// NOTE(erb): memory
void *os_memory_commit(U64 size);
void *os_memory_reserve(U64 size);
void  os_memory_decommit(void *memory, U64 size);
void  os_memory_release(void *memory);

/////////////////////////////////////////////////////////
// NOTE(erb): files

typedef enum File_Kind
{
  FileKind_None,
  FileKind_Text,
  FileKind_Directory,
} File_Kind;

typedef struct Full_Path Full_Path;
struct Full_Path
{
  String8 path;
  String8 file_part;
};

typedef struct Directory_Entry Directory_Entry;
struct Directory_Entry
{
  File_Kind kind;
  String8 path;
  Directory_Entry *next;
};

typedef struct Directory_Entry_List Directory_Entry_List;
struct Directory_Entry_List
{
  Directory_Entry *first;
  Directory_Entry *last;
  U32 count;
  String8 path;
  Directory_Entry_List *next;
};


B32          os_delete_directory(String8 path);
B32          os_delete_directory_recur(String8 path);
B32          os_create_directory(String8 path);
B32          os_file_contains_str_case_insens(String8 path, String8 needle);
File_Kind    os_get_file_kind(String8 path);
String8_List         os_get_directory_files(Arena *arena, String8 directory);
String8_List         os_get_directory_paths(Arena *arena, String8 directory);
Directory_Entry_List os_get_directory_entries(Arena *arena, String8 directory);

B32          os_set_current_directory(String8 path);
String8      os_get_current_directory(Arena *arena);
Full_Path    os_get_full_path(Arena *arena, String8 file_name);

B32          os_debug_write_entire_file(String8 path, String8 contents);
String8      os_debug_read_entire_file(String8 path);
void         os_debug_free_file_memory(void *memory);

/////////////////////////////////////////////////////////
// NOTE(erb): misc

String8 os_get_enviornment_variable(Arena *arena, char *name);

/////////////////////////////////////////////////////////
// NOTE(erb): time

// TODO(erb): maybe just having this as U64 could be good
typedef U64 OS_Clock_Time;
OS_Clock_Time os_get_clock_time();
F32           os_seconds_elapsed(OS_Clock_Time begin, OS_Clock_Time end);
void          os_sleep_ms(U32 ms);

/////////////////////////////////////////////////////////
// NOTE(erb): date
Date_Time os_get_utc_date_time();
Date_Time os_get_local_date_time();

/////////////////////////////////////////////////////////
// NOTE(erb): os kind interop
typedef enum OS_Kind
{
  OSKind_Linux,
  OSKind_Win32,
  OSKind_OSX,
  OSKind_COUNT,
} OS_Kind;

#if OS_WIN32
global OS_Kind os_kind_this           = OSKind_Win32;
global String8 os_path_separator_str  = S8Lit("\\");
global char    os_path_separator_char = '\\';
#include "win32/win32_os.h"

#elif OS_LINUX
global OS_Kind os_kind_this           = OSKind_Linux;
global String8 os_path_separator_str  = S8Lit("/");
global char    os_path_separator_char = '/';
#include "linux/linux_os.h"

#else
#error "platform not supported yet"
#endif

#ifndef OS_ENTRY_POINT 
// TODO(erb): make this a real system!
#error "entry point macro must be design"
#endif

#endif //OS_H
