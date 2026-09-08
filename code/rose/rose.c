///////////////////////////////////////////////////////////
// NOTE(erb): event id

Event_ID
event_id_make(U32 u32)
{
  Event_ID result = {0};
  result.u32 = u32;
  return(result);
}

B32
event_id_is_null(Event_ID id)
{
  B32 result = (id.u32 == 0);
  return(result);
}

B32
event_id_equals(Event_ID a, Event_ID b)
{
  B32 result = (a.u32 == b.u32);
  return(result);
}

U32
hash_from_event_id(Event_ID id)
{
  U32 result = id.u32;
  return(result);
}

///////////////////////////////////////////////////////////
// NOTE(erb): tag id

Tag_ID
tag_id_make(U32 u32)
{
  Tag_ID result = {0};
  result.u32 = u32;
  return(result);
}

B32
tag_id_equals(Tag_ID a, Tag_ID b)
{
  B32 result = (a.u32 == b.u32);
  return(result);
}

Tag_ID
tag_id_null()
{
  Tag_ID result = {0};
  return(result);
}

B32
tag_id_is_valid(Tag_ID id)
{
  B32 result = (id.u32 != 0);
  return(result);
}

///////////////////////////////////////////////////////////
// NOTE(erb): name chunks

Name_Chunk_List
push_name_chunk_list(Arena *arena, Name_Chunk **first_free, String8 str)
{
  Name_Chunk_List result = {0};
  U64 chunks_needed = (str.size+NAME_CHUNK_PAYLOAD_SIZE-1)/NAME_CHUNK_PAYLOAD_SIZE;
  U64 bytes_left = str.size;
  for(U32Each(idx, chunks_needed))
  {
    Name_Chunk *chunk = *first_free;
    if(chunk)
    {
      SLLStackPop(*first_free);
    }
    else
    {
      chunk = (Name_Chunk *)arena_push_size(arena, sizeof(Name_Chunk)+NAME_CHUNK_PAYLOAD_SIZE);
    }
    ZeroStruct(chunk);
    SLLQueueAppend(result.first, result.last, chunk);
    U64 bytes_to_copy = Min(bytes_left, NAME_CHUNK_PAYLOAD_SIZE);
    U8 *src = str.value + (str.size - bytes_left);
    U8 *dest = ((U8 *)chunk+sizeof(Name_Chunk));
    ArrayCopy(src, dest, bytes_to_copy);
    bytes_left -= bytes_to_copy;
  }
  result.count = chunks_needed;
  result.total_size = str.size;
  return(result);
}

String8
str8_from_name_chunk_list(Arena *arena, Name_Chunk_List *chunk_list)
{
  String8_Writer sw = push_str8_writer(arena, chunk_list->total_size);
  U64 bytes_left = chunk_list->total_size;
  for(EachNode(Name_Chunk, chunk, chunk_list->first))
  {
    U64 to_copy = Min(bytes_left, NAME_CHUNK_PAYLOAD_SIZE);
    U8 *src = ((U8 *)chunk+sizeof(Name_Chunk));
    str8_writer_str(&sw, S8Struct(src, to_copy));
    bytes_left -= to_copy;
  }
  String8 result = sw.str;
  return(result);
}

void
name_chunk_list_free_all(Name_Chunk_List *chunk_list, Name_Chunk **first_free)
{
  for(U32Each(idx, chunk_list->count))
  {
    Name_Chunk *chunk = chunk_list->first;
    SLLStackPop(chunk_list->first);
    SLLStackPush(*first_free, chunk);
  }
  ZeroStruct(chunk_list);
}

///////////////////////////////////////////////////////////
// NOTE(erb): events

Event *
push_event(Arena *event_arena, Event_List *event_list, Arena *name_chunk_arena, Name_Chunk **first_free_name_chunk, String8 title, Event_ID id, Date_Time start, Date_Time end)
{
  Event *new_event = PushStruct(event_arena, Event);
  DLLQueueAppend(event_list->first, event_list->last, new_event);
  event_list->count++;
  new_event->id = id;
  new_event->title_chunk_list = push_name_chunk_list(name_chunk_arena, 
                                                     first_free_name_chunk, title);
  new_event->start = start;
  new_event->end = end;
  return(new_event);
}

#include "rose_eval.c"
#include "rose_ui_widgets.c"
#include "rose_serial.c"
#include "rose_command.c"
#include "rose_app.c"