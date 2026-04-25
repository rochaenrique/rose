/* date = April 24th 2026 0:54 pm */

#ifndef BASE_MEMORY_H
#define BASE_MEMORY_H

typedef struct Arena
{
	U64 size;
	U64 used;
	void *base;
} Arena;

typedef struct Scratch
{
  Arena *arena;
  U64 position;
} Scratch;

typedef struct Thread_Context
{
  Arena arenas[4];
} Thread_Context;

typedef void *Memory_Commit_Func(U64 size);

#define push_struct(arena, Type) ((Type *)arena_push_size((arena), sizeof(Type)))
#define push_array(arena, count, Type) ((Type *)arena_push_size((arena), sizeof(Type)*(count)))
#define sll_push_allocate(arena, list, Type) \
((!(list)->first) \
? ((list)->first = (list)->last = push_struct((arena), Type)) \
: ((list)->last = (list)->last->next = push_struct((arena), Type)))
#define copy_struct(src, dest) mem_copy((U8 *)src, (U8 *)dest, sizeof(*src))
#define zero_struct(stru) mem_zero(stru, sizeof(*stru))
#define scratch_block(NAME, CONFLICTS, CONFLICT_COUNT) \
Scratch NAME = {0}; \
defer_loop((NAME) = scratch_begin((CONFLICTS), (CONFLICT_COUNT)), scratch_end(&(NAME)))

#endif //BASE_MEMORY_H
