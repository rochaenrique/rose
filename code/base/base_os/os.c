void 
thread_context_init(Thread_Context *context)
{
  for(EachElement(Arena, arena, context->arenas))
  {
    U64 size = MB(1);
    arena_init(arena, os_memory_commit(size), size);
  }
}

Scratch
thread_context_get_scratch(Thread_Context *context, Arena **conflicts, U32 conflict_count)
{
  Scratch result = {0};
  for(EachElement(Arena, thread_arena, context->arenas))
  {
    B32 not_conflict = true;
    for(U32Each(conflict_idx, conflict_count)) 
    {
      Arena *conflict_arena = conflicts[conflict_idx];
      if(thread_arena == conflict_arena) 
      {
        not_conflict = false;
        break;
      }
    }
    if(not_conflict) 
    {
      result = arena_get_scratch(thread_arena);
      break;
    }
  }
  return(result);
}

Scratch 
scratch_begin(Arena **conflicts, U32 conflict_count)
{
  Thread_Context *tctx = os_thread_context_get();
  Scratch result = thread_context_get_scratch(tctx, conflicts, conflict_count);
  return(result);
}

String8
os_concat_path(Arena *arena, String8 a, String8 b)
{
  String8 result = str8_concat_3(arena, a, os_path_separator_str, b);
  return(result);
}

#if defined(OS_WIN32)
#include "win32/win32_os.c"
#elif defined(OS_LINUX)
#include "linux/linux_os.c"
#endif