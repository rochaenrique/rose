U8 *mem_copy(U8 *source, U8 *dest, U64 count)
{
	while (count-- > 0)
	{
		*dest++ = *source++;
	}
	return dest;
}

void mem_set(U8 *bytes, U64 count, U8 value)
{
	while (count-- > 0)
	{
		*bytes++ = value;
	}
}

void mem_zero(U8 *bytes, U64 count)
{
	mem_set(bytes, count, 0);
}
Arena
arena_make(void *memory, U64 size)
{
  Arena arena = {0};
	arena.size = size;
	arena.base = memory;
	arena.used = 0;
  mem_set(memory, size, 0xDD);
  return(arena);
}


Arena
arena_alloc(U64 size, Memory_Commit_Func mem_commit)
{
  void *memory = mem_commit(size);
  assert(memory != 0); // TODO(erb): think of adding some error handling/reporting here
  Arena result = arena_make(memory, size);
  return result;
}

void 
arena_init(Arena *arena, void *memory, U64 size)
{
	arena->size = size;
	arena->base = memory;
	arena->used = 0;
  mem_set(memory, size, 0xDD);
}

void 
arena_release(Arena *arena)
{
	arena->used = 0;
}

void *
arena_push_size(Arena *arena, U64 size) 
{
  assert(arena->used + size < arena->size);
  void *result = (U8 *)arena->base + arena->used;
  mem_zero(result, size);
  arena->used += size;
  return((void *)result);
}

Arena 
push_sub_arena(Arena *parent, U64 size)
{
  Arena result = {0};
  void *memory = arena_push_size(parent, size);
  arena_init(&result, memory, size);
  return(result);
}

Scratch 
arena_get_scratch(Arena *arena)
{
  Scratch result = {0};
  result.arena = arena;
  result.position = arena->used;
  return(result);
}

void 
arena_scratch_reset(Scratch *scratch)
{
  scratch->arena->used = scratch->position;
  scratch->arena = 0;
  scratch->position = 0;
}

Scratch
thread_context_get_scratch(Thread_Context *context, Arena **conflicts, U32 conflict_count)
{
  Scratch result = {0};
  for (U32 arena_idx = 0;
       arena_idx < array_size(context->arenas);
       ++arena_idx)
  {
    Arena *thread_arena = (context->arenas + arena_idx);
    B32 not_conflict = true;
    
    for (U32 conflict_idx = 0;
         conflict_idx < conflict_count;
         ++conflict_idx) 
    {
      Arena *conflict_arena = *(conflicts + conflict_idx);
      if (thread_arena == conflict_arena) 
      {
        not_conflict = false;
        break;
      }
    }
    
    if (not_conflict) 
    {
      result = arena_get_scratch(thread_arena);
      break;
    }
  }
  
  return(result);
}

Thread_Context *thread_context_get(void);
Scratch scratch_begin(Arena **conflicts, U32 conflict_count)
{
  Thread_Context *tctx = thread_context_get();
  Scratch result = thread_context_get_scratch(tctx, conflicts, conflict_count);
  return result;
}

void
scratch_end(Scratch *scratch)
{
  arena_scratch_reset(scratch);
}


void thread_context_init(Thread_Context *context, Memory_Commit_Func *mem_commit)
{
  for (U32 arena_idx = 0;
       arena_idx < array_size(context->arenas);
       ++arena_idx)
  {
    context->arenas[arena_idx] = arena_alloc(mb(1), mem_commit);
  }
}
