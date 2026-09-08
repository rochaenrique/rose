/* date = August 27th 2026 10:39 pm */

#ifndef ROSE_APP_H
#define ROSE_APP_H

#if DEBUG
typedef struct Debug_Ctx Debug_Ctx;
struct Debug_Ctx
{
  B32 is_expanded;
  
  B32 draw_text_highlight;
  B32 draw_outer_size_line;
  B32 draw_inner_size_line;
};

global Debug_Ctx *global_debug_ctx = 0;
#endif

typedef struct Column Column;
struct Column
{
  String8 label;
  String8 eval_str;
  F32 pct_of_row;
};

typedef enum View_Kind
{
  ViewKind_Calendar,
  ViewKind_Lister,
  ViewKind_Table,
  ViewKind_Watch,
  ViewKind_EventFocus,
  ViewKind_Tags,
  ViewKind_COUNT,
} View_Kind;

typedef struct View_ID View_ID;
struct View_ID 
{
  U32 u32;
};

typedef struct View View;
struct View
{
  View_Kind kind;
  View *next;
  
  B32 is_initialized;
  
  V4f new_tag_color;
  B32 is_color_picker_open;
  
  union
  {
    B32 is_lister_filtering;
    struct
    {
      V2i calendar_grid_size;
      Calendar_Date calendar_start;
    };
    struct
    {
      Column columns[16];
      U32 column_count;
      B32 has_column_popup;
      V2f column_popup_pos;
    };
    struct
    {
      Event_ID event_id;
      Date_Input_State date_input_state;
    };
    struct
    {
      Tag_ID open_color_picker_tag;
      B32 tag_color_picker_is_open;
    };
    struct
    {
      String8_List expr_list;
    };
  };
};

typedef struct Tab Tab;
struct Tab
{
  View_ID view;
  Tab *next;
  Tab *prev;
  Tab *next_free;
};

typedef struct Panel Panel;
struct Panel
{
  Panel *first;
  Panel *last;
  Panel *next;
  Panel *prev;
  Panel *parent;
  
  // NOTE(erb): parent stuff
  Axis2 split_axis;
  U32 child_count;
  
  // NOTE(erb): child stuff
  F32 pct_of_parent;
  
  Tab tab_storage[32];
  Tab *first_free_tab;
  Tab *first_tab;
  Tab *last_tab;
  
  View_ID current_view;
  B32 is_picking_new_tab;
  
  View_ID move_view_ref;
  B32 move_view_before_ref;
};

typedef struct Directory_Entries_Hash_Table Directory_Entries_Hash_Table;
struct Directory_Entries_Hash_Table
{
  Directory_Entry_List slots[64];
};

////////////////////////////////////////////
// NOTE(erb): command kinds

typedef enum Command_Kind
{
  CommandKind_None,
  CommandKind_LoadFile,
  CommandKind_SaveToFile,
  CommandKind_CommandLauncher,
  CommandKind_GenerateStubFile,
  CommandKind_COUNT,
} Command_Kind;


////////////////////////////////////////////
// NOTE(erb): app state

typedef struct Command_Event Command_Event;
struct Command_Event
{
  Command_Kind kind;
  B32 not_first_time;
  Command_Event *prev;
  Command_Event *next;
};

typedef struct App_State App_State;
struct App_State
{
  Arena arena;
  UI_Ctx ui;
  UI_Font_Face font_face;
  B32 initialized;
  Thread_Context thread_context;
  
  String8 current_directory;
  
  String8 load_from_file_path;
  String8 save_to_file_path;
  Read_Entire_File_Str8_Func *read_entire_file_str8;
  Write_Entire_File_Str8_Func *write_entire_file_str8;
  
  Arena name_chunk_arena;
  Name_Chunk *first_free_name_chunk;
  
  Arena event_arena;
  Event_List event_list;
  U32 global_id_counter;
  
  Arena panel_arena;
  Panel *root_panel;
  Panel *first_free_panel;
  
  View views[64];
  View *first_free_view;
  
  Event_Tag tags[256];
  Event_Tag *first_free_tag;
  
  View_ID moving_view;
  
  Arena directory_arena;
  Directory_Entries_Hash_Table dir_entries_cache;
  
  Key_Combo_Set key_maps[CommandKind_COUNT];
  
  Arena *command_event_arena;
  Command_Event *first_free_event;
  Command_Event *first_event;
  Command_Event *last_event;
  
#if DEBUG
  Debug_Ctx debug_ctx;
#endif
};

// NOTE(erb): command kinds

#define COMMAND_FUNC(name) B32 name(App_State *app_state)
typedef COMMAND_FUNC(Command_Func);

typedef struct Command_Info Command_Info;
struct Command_Info
{
  String8 description;
  String8 name;
  Key_Combo default_bind;
  Command_Func *func;
};

#define EachCmdKind(it) Command_Kind it = CommandKind_None+1; (it)<CommandKind_COUNT; (it)+=1

#endif //ROSE_APP_H
