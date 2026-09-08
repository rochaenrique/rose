/* date = June 24th 2026 0:58 pm */

#ifndef APP_BRIDGE_H
#define APP_BRIDGE_H

typedef struct App_Memory App_Memory;
struct App_Memory
{
  void *buffer;
  U64 size;
  B32 first_attach;
};

#define READ_ENTIRE_FILE_STR8(name) String8 name(Arena *arena, String8 file_path)
typedef READ_ENTIRE_FILE_STR8(Read_Entire_File_Str8_Func);

#define WRITE_ENTIRE_FILE_STR8(name) void name(String8 file_path, String8 contents)
typedef WRITE_ENTIRE_FILE_STR8(Write_Entire_File_Str8_Func);

#define PUSH_CLIPBOARD_STR8_FUNC(name) String8 name(Arena *arena, void *data)
typedef PUSH_CLIPBOARD_STR8_FUNC(Push_Clipboard_Str8_Func);

#define SET_CLIPBOARD_STR8_FUNC(name) void name(String8 str, void *data)
typedef SET_CLIPBOARD_STR8_FUNC(Set_Clipboard_Str8_Func);

typedef struct Clipboard_API Clipboard_API;
struct Clipboard_API
{
  Push_Clipboard_Str8_Func *push_str8_func;
  Set_Clipboard_Str8_Func *set_str8_func;
  void *data;
};

#define SET_CURSOR_FUNC(name) void name(GFX_Cursor_Kind kind, void *data)
typedef SET_CURSOR_FUNC(Set_Cursor_Func);
typedef struct Cursor_API Cursor_API;
struct Cursor_API
{
  Set_Cursor_Func *set_func;
  void *data;
};

typedef struct Platform_API Platform_API;
struct Platform_API
{
  Clipboard_API clipboard;
  Cursor_API cursor;
  Read_Entire_File_Str8_Func *read_entire_file_str8;
  Write_Entire_File_Str8_Func *write_entire_file_str8;
}; 

#define UPDATE_AND_RENDER_FUNC(name) void name(App_Memory *memory, Q2D_State *render_state, \
Q2D_Font *font, V2i client_size, GFX_Event_List *events, \
Platform_API platform_api, \
F32 delta_time, \
String8 current_directory) 
typedef UPDATE_AND_RENDER_FUNC(Update_And_Render_Func);

#endif //APP_BRIDGE_H
