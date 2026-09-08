#include <time.h> //clock_gettime
#include <errno.h> //errno
#include <string.h> //strerror()
#include <unistd.h> //read()
#include <fcntl.h> //open()
#include <sys/mman.h> //mmap(), munmap()
#include <sys/stat.h> //struct stat, fstat();
#include <pthread.h> //pthread_key_t, pthread_key_create(), pthread_key_delete()
global pthread_key_t lnx_thread_storage_key;

String8
os_get_current_directory(Arena *arena)
{
  Scratch scratch = scratch_begin(&arena, 1);
  U32 buff_size = 1024;
  char *buff = PushArray(scratch.arena, buff_size, char);
  char *cwd = getcwd(buff, buff_size);
  String8 result = {0};
  if(cwd != 0)
  {
    result = push_str8_from_cstr(arena, cwd);
  }
  else
  {
    InvalidCodePath;
  }
  scratch_end(&scratch);
  return(result);
}

B32
os_init()
{
  I32 thread_key_failed = pthread_key_create(&lnx_thread_storage_key, 0);
  if (thread_key_failed != 0)
  {
    err_set_error(S8("failed to create thread local storage"));
  }
  return(thread_key_failed == 0);
}

void
os_release(void)
{
  // NOTE(erb): placeholder
  pthread_key_delete(lnx_thread_storage_key);
}

// TODO(erb): it would be really cool to have a "try to re-launch" option 
// here, which forks into a process that launches the application

// TODO(erb): try a more interesting callback like xmessage or gtk_message_dialog

void
os_handle_bad_error(String8 str)
{
  fprintf(stderr, "[ERROR] %.*s\n", S8Expand(str));
  exit(1);
}

void 
os_log(String8 str)
{
  Scratch scratch = scratch_begin(0, 0);
  puts(push_cstr(scratch.arena, str));
  scratch_end(&scratch);
}

void 
os_logf(const char *format, ...)
{
  Scratch scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, format);
  String8 null_terminated_str = push_str8f_varg(scratch.arena, format, args);
  //vprintf(format, args);
  va_end(args);
  
  puts((char *)null_terminated_str.value);
  
  scratch_end(&scratch);
}

void *
os_memory_commit(U64 size)
{
  U64 total_size = (size + sizeof(LNX_Alloc_Header));
  
  void *memory = mmap(0, total_size, //offset, size
                      PROT_READ|PROT_WRITE, //protection
                      MAP_PRIVATE|MAP_ANON, //flags
                      -1, 0); //fd, offset
  Assert(memory != MAP_FAILED); // TODO(erb): think of adding some error reporting here
  
  LNX_Alloc_Header *header = (LNX_Alloc_Header *)memory;
  header->size = size;
  void *result = (void *)(header + 1);
  return(result);
}

void *
os_memory_reserve(U64 size)
{
  void *result = os_memory_commit(size);
  return(result);
}

void
os_memory_decommit(void *memory, U64 size)
{
  SupressUnused(size);
  os_memory_release(memory);
}

void
os_memory_release(void *memory)
{
  LNX_Alloc_Header *header = ((LNX_Alloc_Header *)memory - 1);
  U64 size = header->size;
  int unmap_error = munmap((U8 *)header, size);
  Assert(unmap_error == 0);
}

void 
os_thread_context_set(Thread_Context *context)
{
  I32 error = pthread_setspecific(lnx_thread_storage_key, (void *)context);
  Assert(error == 0);
}

Thread_Context *
os_thread_context_get(void)
{
  Thread_Context *result = (Thread_Context *)pthread_getspecific(lnx_thread_storage_key);
  return result;
}


void
os_debug_free_file_memory(void *memory)
{
  os_memory_release(memory);
}

String8
os_debug_read_entire_file(String8 path)
{
  String8 result = {0};
  
  Scratch scratch = scratch_begin(0, 0);
  I32 file_descriptor = open(push_cstr(scratch.arena, path), O_RDONLY);
  scratch_end(&scratch);
  
  if(file_descriptor != -1)
  {
    struct stat file_stat = {0};
    if (fstat(file_descriptor, &file_stat) == 0)
    {
      I64 size = file_stat.st_size;
      void *buffer = os_memory_commit(size);
      
      I64 bytes_read = read(file_descriptor, buffer, size);
      if (bytes_read == size)
      {
        ((U8 *)buffer)[size] = 0;
        result.value = buffer;
        result.size = (U64)size;
      }
      else
      {
        os_debug_free_file_memory(buffer);
        os_logf("read() failed: (%.*s): %s\n", S8Expand(path), strerror(errno));
        Assert(false);
      }
    }
    else
    {
      os_logf("Failed to get file stat (%.*s): %s\n", S8Expand(path), strerror(errno));
      Assert(false);
    }
    close(file_descriptor);
  }
  else
  {
    os_logf("Failed to open file (%.*s): %s\n", S8Expand(path), strerror(errno));
    Assert(false);
  }
  return(result);
}

String8
os_read_entire_file_str8_arena(Arena *arena, String8 file_path)
{
  Scratch scratch = scratch_begin(&arena, 1);
  I32 file_descriptor = open(push_cstr(scratch.arena, path), O_RDONLY);
  scratch_end(&scratch);
  
  String8 result = {0};
  if(file_descriptor != -1)
  {
    struct stat file_stat = {0};
    if (fstat(file_descriptor, &file_stat) == 0)
    {
      I64 size = file_stat.st_size;
      U8 *buffer = PushArray(arena, size, U8);
      
      I64 bytes_read = read(file_descriptor, buffer, size);
      if (bytes_read == size)
      {
        ((U8 *)buffer)[size] = 0;
        result = S8Struct(buffer, (U64)size);
      }
      else
      {
        os_logf("read() failed: (%S): %s\n", path, strerror(errno));
        InvalidCodePath;
      }
    }
    else
    {
      os_logf("Failed to get file stat (%S): %s\n", path, strerror(errno));
      InvalidCodePath;
    }
    close(file_descriptor);
  }
  else
  {
    os_logf("Failed to open file (%S): %s\n", path, strerror(errno));
    InvalidCodePath;
  }
  return(result);
}

// NOTE(erb): returns nanoseconds
OS_Clock_Time 
os_get_clock_time()
{
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  OS_Clock_Time result = ((U64)ts.tv_sec*1000000000 + ts.tv_nsec);
  return(result);
}

F32
os_seconds_elapsed(OS_Clock_Time begin, OS_Clock_Time end)
{
  F32 result = (begin - end) / 1000000000.0f;
  return(result);
}

void
os_sleep_ms(U32 ms)
{
}


