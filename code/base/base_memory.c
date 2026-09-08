U8 *
mem_copy(U8 *source, U8 *dest, U64 count)
{
  if(dest && source)
  {
    while (count-- > 0)
    {
      *dest++ = *source++;
    }
  }
	return(dest);
}

U8 *
mem_set(U8 *bytes, U64 count, U8 value)
{
	while (count-- > 0)
	{
		*bytes++ = value;
	}
  return(bytes);
}

B32
mem_equals(U8 *a, U8 *b, U64 count)
{
  B32 result = true;
  for(U64Each(idx, count))
  {
    if(a[idx] != b[idx])
    {
      result = false;
      break;
    }
  }
  return(result);
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
  Assert(arena->used + size <= arena->size);
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
  ZeroStruct(scratch);
}

void
scratch_end(Scratch *scratch)
{
  arena_scratch_reset(scratch);
}
