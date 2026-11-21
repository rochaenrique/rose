/* date = August 27th 2026 10:39 pm */

#ifndef ROSE_APP_H
#define ROSE_APP_H

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

typedef enum ROSE_UI_Rule
{
  ROSEUIRule_Calendar,
} ROSE_UI_Rule;

typedef struct Watch_Eval Watch_Eval;
struct Watch_Eval
{
  U8 str_buffer[64];
  U32 str_size;
  
  B32 has_data;
  B32 should_cache_interp;
  U8 memory[256];
  String8 error;
  Arena arena;
  
  IR_Op_List op_list;
  E_Value_Interp interp;
};

typedef struct Eval_Watch_ID Eval_Watch_ID;
struct Eval_Watch_ID
{
  U32 parent;
  U32 idx;
};

typedef struct Eval_Block_Tree Eval_Block_Tree;

typedef struct Eval_Block_Tree_List Eval_Block_Tree_List;
struct Eval_Block_Tree_List
{
  Eval_Block_Tree *first;
  Eval_Block_Tree *last;
  U32 count;
};

typedef struct Eval_Block_Tree Eval_Block_Tree;
struct Eval_Block_Tree
{
  Eval_Watch_ID id;
  
  U32 expansion_size;
  B32 expansion_is_not_slice; 
  
  String8 error;
  E_Value value;
  Buffer value_memory;
  String8 str;
  
  Eval_Block_Tree_List children;
  Eval_Block_Tree *next;
};

typedef struct Eval_Block_Range Eval_Block_Range;
struct Eval_Block_Range
{
  Eval_Watch_ID id;
  
  // NOTE(erb): this is basically only for base case where the watch eval
  // turns into a node that isnt a slice, its just the thing
  B32 is_header;
  
  U32 range_in_parent_start;
  U32 range_in_parent_size;
  
  U32 depth;
  
  String8 error;
  E_Value value;
  Buffer value_memory;
  String8 str;
  
  Eval_Block_Range *next;
};

typedef struct Eval_Block_Range_List Eval_Block_Range_List;
struct Eval_Block_Range_List
{
  Eval_Block_Range *first;
  Eval_Block_Range *last;
  U32 count;
  U32 total_expansions;
};

typedef struct Watch_Expansion Watch_Expansion;
struct Watch_Expansion
{
  Eval_Watch_ID id;
  Watch_Expansion *next;
};

typedef struct Watch_Expansion_State Watch_Expansion_State;
struct Watch_Expansion_State
{
  Watch_Expansion *slots[32];
  Watch_Expansion *first_free_exp;
  Watch_Expansion expansions[64];
};

typedef struct View View;
struct View
{
  U8 title[32];
  U32 title_size;
  B32 is_watch;
  
  U8 eval_memory[KB(1)];
  Arena eval_arena;
  E_Value_Interp eval_interp;
  
  // NOTE(erb): watch window
  B32 is_watch_first_time;
  Watch_Eval evals[16];
  U32 eval_count;
  
  Watch_Expansion_State expansion_state;
  
  I32 selection_cursor;
  U32 eval_max_depth;
  
  View *next;
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
  
  B32 is_editing_view_params;
  
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

typedef struct Thing Thing;
struct Thing
{
  U32 num1;
  U32 num2;
  B32 bool1;
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
  
  View views[32];
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
  
  // NOTE(erb): ir test stuff
  E_Global_Iden *top_global_iden;
  IR_Symbol_Type *top_symbol_type;
  IR_Type_Table type_table;
  
  IR_Symbol_Type func_symbol;
  E_Global_Iden func_addr;
  
  U32 iden_value;
  IR_Symbol_Type iden_symbol;
  E_Global_Iden iden_addr;
  
  Thing thing_arr[5];
  IR_Symbol_Type thing_symbol;
  E_Global_Iden thing_iden;
  
  // NOTE(erb): ir view stuff
  IR_Symbol_Type calendar_func_symbol;
  E_Global_Iden calendar_func_iden;
  
  IR_Type_ID ir_view_types[16];
  U32 ir_view_types_count;
  
  IR_Type_ID ir_type_id__calendar_date;
  IR_Type_ID ir_type_id__calendar_view;
  
#if DEBUG
  UI_Debug_Ctx debug_ctx;
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
