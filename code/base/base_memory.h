/* date = April 24th 2026 0:54 pm */

#ifndef BASE_MEMORY_H
#define BASE_MEMORY_H

typedef struct Buffer Buffer;
struct Buffer
{
  void *base;
  U64 size;
};

typedef struct Arena Arena;
struct Arena
{
	U64 used;
	Buffer buffer;
};

typedef struct Scratch Scratch;
struct Scratch
{
  Arena *arena;
  U64 position;
};

typedef void *Allocate_Func(U64 size);

// TODO(erb): have a push_not_zero_initialized

#define PushStruct(arena, Type) ((Type *)arena_push_size((arena), sizeof(Type)))
#define PushArray(arena, count, Type) ((Type *)arena_push_size((arena), sizeof(Type)*(count)))
#define mem_zero(memory, size) mem_set(memory, size, 0)
#define CopyStruct(src, dest) mem_copy((U8 *)src, (U8 *)dest, sizeof(*src))
#define ZeroStruct(stru_ptr) mem_zero((U8 *)(stru_ptr), sizeof(*(stru_ptr)))

#endif //BASE_MEMORY_H
