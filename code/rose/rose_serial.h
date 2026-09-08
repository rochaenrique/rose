/* date = August 26th 2026 1:27 pm */

#ifndef ROSE_SERIAL_H
#define ROSE_SERIAL_H

typedef struct SRL_Tag_Ref SRL_Tag_Ref;
struct SRL_Tag_Ref
{
  U32 id;
  SRL_Tag_Ref *next;
};

typedef struct SRL_Event SRL_Event;
struct SRL_Event
{
  String8 name;
  Date_Time start;
  Date_Time end;
  U32 id;
  SRL_Tag_Ref *first_tag;
  
  SRL_Event *next;
};

typedef struct SRL_Tag SRL_Tag;
struct SRL_Tag
{
  U64 id;
  String8 name;
  V4f color;
  
  SRL_Tag *next;
};

typedef struct SRL_Project_File SRL_Project_File;
struct SRL_Project_File
{
  SRL_Event *first_event;
  SRL_Tag *first_tag;
  U32 global_id_counter;
  
  String8 error;
  U32 error_line;
  U32 error_location;
};

#endif //ROSE_SERIAL_H
