/* date = August 3rd 2026 11:17 am */

#ifndef ROSE_H
#define ROSE_H

#define NAME_CHUNK_PAYLOAD_SIZE 56

typedef struct Name_Chunk Name_Chunk;
struct Name_Chunk
{
  Name_Chunk *next;
};

typedef struct Name_Chunk_List Name_Chunk_List;
struct Name_Chunk_List
{
  Name_Chunk *first;
  Name_Chunk *last;
  U64 count;
  U64 total_size;
};

typedef struct Tag_ID Tag_ID;
struct Tag_ID
{
  U32 u32;
};

typedef struct Event_Tag Event_Tag;
struct Event_Tag
{
  U8 name[256];
  U32 name_size;
  V4f color;
  B32 used;
  Event_Tag *next;
};

typedef struct Event_ID Event_ID;
struct Event_ID
{
  U32 u32;
};

typedef struct Event Event;
struct Event
{
  Event_ID id;
  Name_Chunk_List title_chunk_list;
  
  Date_Time start;
  Date_Time end;
  
  Tag_ID tags[16];
  U32 tag_count;
  
  Event *prev;
  Event *next;
};

typedef struct Event_List Event_List;
struct Event_List
{
  Event *first;
  Event *last;
  U32 count;
};

typedef struct Key_Combo Key_Combo;
struct Key_Combo
{
  GFX_Key key;
  GFX_Modifiers mods;
};

typedef struct Key_Combo_Set Key_Combo_Set;
struct Key_Combo_Set
{
  Key_Combo combos[4];
  U32 count;
};

#include "rose_ui_widgets.h"
#include "rose_serial.h"
#include "rose_command.h"
#include "rose_app.h"


#endif //ROSE_H
