/* Date = December 6th 2025 8:27 am */

#ifndef UI_H
#define UI_H

typedef enum UI_Flags
{
	UIFlags_CrossOverlay   = (1<<0),
	UIFlags_DrawText       = (1<<1),
	UIFlags_DrawBackGround = (1<<2),
	UIFlags_DrawBorder     = (1<<3),
	UIFlags_DrawRounded    = (1<<4),
	UIFlags_Comm           = (1<<5),
	UIFlags_Floating       = (1<<6),
  UIFlags_ClipRectangle  = (1<<7),
  UIFlags_DontSeedKey    = (1<<8),
  UIFlags_HoverEffect    = (1<<9),
  UIFlags_CommRectFromCached = (1<<10),
} UI_Flags;

typedef struct UI_Key UI_Key;
struct UI_Key
{
  U64 u64;
};

typedef enum UI_Sizing
{
	UISizing_Fit,
	UISizing_Grow,
	UISizing_Shrink,
	UISizing_Fixed,
} UI_Sizing;

typedef enum UI_Align
{
  UIAlign_Start,
  UIAlign_Center,
  UIAlign_End,
} UI_Align;

typedef struct UI_Sizing_Value UI_Sizing_Value;
struct UI_Sizing_Value
{
	UI_Sizing sizing;
	F32 value;
};

typedef struct UI_Line_Node UI_Line_Node;
struct UI_Line_Node
{
  UI_Line_Node *next;
  String8 content;
};

typedef struct UI_Font_Face UI_Font_Face;
struct UI_Font_Face
{
  Q2D_Font *font;
  F32 blank_spacing;
  F32 char_spacing;
  F32 line_spacing;
  F32 size;
};

typedef struct UI_Text UI_Text;
struct UI_Text
{
	V4f color;
  UI_Font_Face face;
  String8 content;
  
  U32 line_count;
  UI_Line_Node *first_line;
  UI_Line_Node *last_line;
};

typedef struct Text_Cmd Text_Cmd;
struct Text_Cmd
{
  Range_U64 replace_range;
  String8 replace_str;
  
  String8 copy_str;
  I64 new_cursor;
  I64 new_mark;
};

typedef struct Text_Edit_State Text_Edit_State;
struct Text_Edit_State
{
  I64 cursor;
  I64 mark;
};

typedef enum Text_Action_Flags 
{
  TextActionFlags_Copy                = (1<<0),
  TextActionFlags_Paste               = (1<<1),
  TextActionFlags_Cut                 = (1<<2),
  TextActionFlags_Delete              = (1<<3),
  TextActionFlags_KeepMark            = (1<<4),
  TextActionFlags_ZeroDeltaWithSelect = (1<<5),
  TextActionFlags_WordScan            = (1<<6),
  TextActionFlags_DeltaPickSelectSide = (1<<7),
} Text_Action_Flags;

typedef struct Text_Action Text_Action;
struct Text_Action
{
  Text_Action_Flags flags;
  I64 delta;
  I32 codepoint;
};

typedef struct Gradient2 Gradient2;
struct Gradient2
{
  V4f color0;
  V4f color1;
  Orientation2 orientation;
};

typedef enum UI_Draw_Kind
{
	UIDrawKind_Text,
	UIDrawKind_Rect,
	UIDrawKind_RectLines,
	UIDrawKind_ClipBegin,
	UIDrawKind_ClipEnd,
} UI_Draw_Kind;

typedef struct UI_Draw_Cmd  UI_Draw_Cmd;
struct UI_Draw_Cmd
{
  UI_Draw_Cmd *next;
	UI_Draw_Kind kind;
  Range_V2f range;
  union
  {
    V4f color;
    Gradient2 background;
  };
  union
  {
    struct
    {
      F32 border_thickness;
      F32 corner_radius;
    } rect;
    struct
    {
      V4f color;
      Q2D_Font *font;
      String8 content;
      F32 font_size;
      F32 blank_spacing;
      F32 char_spacing;
    } text;
  };
};

typedef struct UI_Draw_Cmd_List UI_Draw_Cmd_List;
struct UI_Draw_Cmd_List
{
  UI_Draw_Cmd *first;
  UI_Draw_Cmd *last;
};

typedef union Padding2 Padding2;
union Padding2
{
  struct
  {
    F32 left;
    F32 top;
    F32 right;
    F32 bottom;
  };
  struct
  {
    V2f left_top;
    V2f right_bottom;
  };
  F32 c[4];
};

typedef struct UI_Node UI_Node;
struct UI_Node
{
#if DEBUG
  // NOTE(erb): debug
  String8 debug_str;
#endif
  
  UI_Flags flags;
  
	V2f pos;
	V2f size;
	V2f min_size;
	V2f max_size;
  
  UI_Align align[2];
	UI_Sizing sizing[2];
	Axis2 layout_axis;
	
  GFX_Cursor_Kind hover_cursor;
	Padding2 padding;
	V4f border_color;
	F32 border_thickness;
	F32 corner_radius;
  F32 child_gap;
  Gradient2 background;
  F32 scroll_offset;
  
  UI_Text *text;
  
  U32 layout_children_count;
  
  UI_Node *first_child;
  UI_Node *last_child;
  UI_Node *parent;
  UI_Node *next;
	
  UI_Node *next_hash;
	
	UI_Key key;
};

typedef struct UI_Comm UI_Comm;
struct UI_Comm
{
  UI_Node *node;
	B8 hovered;
  B8 pressed_out;
  
	B8 left_clicked;
	B8 left_pressed;
	B8 left_down;
  B8 left_dragging;
  
  B8 right_clicked;
	B8 right_pressed;
	B8 right_down;
  B8 right_dragging;
  
  V2f scroll;
};

typedef struct Button_State Button_State;
struct Button_State
{
  B32 ended_down;
  U32 half_transition_count;
};

typedef struct UI_Event_State UI_Event_State;
struct UI_Event_State
{
  V2f past_cursor_delta;
  V2f cursor_delta;
  V2f cursor;
  V2f scroll;
  V2i scroll_clicks;
  Button_State keys[GFXKey_COUNT];
  Button_State buttons[GFXMouseCode_COUNT];
};

#define UI_INPUT_STATE_SCROLL_TIME 0.1f
typedef struct UI_Line_Input_State UI_Line_Input_State;
struct UI_Line_Input_State
{
  U8 buffer[KB(1)];
  String8_Writer writer;
  
  F32 scroll_timer;
  F32 collected_scroll_x;
  Text_Edit_State edit_state;
  
  UI_Key key;
  UI_Line_Input_State *next;
  
  U64 last_changed_frame;
  U64 created_frame;
};

typedef struct UI_Ctx UI_Ctx;
struct UI_Ctx
{
  // NOTE(erb): tick
  U64 frame_tick;
  F64 frame_delta_time;
  F64 frame_total_time;
  
  // NOTE(erb): external api
  UI_Font_Face default_font_face;
  GFX_Event_List event_list;
  
  // NOTE(erb): input
  UI_Event_State input;
  
  // NOTE(erb): apis
  Clipboard_API clipboard_api;
  Cursor_API cursor_api;
  
  // NOTE(erb): node tree
	Arena arena;
	Arena last_arena;
	UI_Node *current;
	UI_Node *root;
	V2f root_pos;
	U32 node_count;
  UI_Node *center_popup;
	
  // NOTE(erb): node cache
  UI_Node *node_cache[256];
  
  // NOTE(erb): input fields cache
  Arena input_state_arena;
  UI_Line_Input_State *input_state_cache[64];
  UI_Line_Input_State *first_free_input_state;
  U32 input_state_count;
  
  // NOTE(erb): interactions
  UI_Key active_node;
	UI_Key hot_node;
  
  // NOTE(erb): macro helpers
#define UI_NEXT_VALUE(Value_Type, label) struct { B32 has_next_##label; Value_Type next_##label; }
#define UI_STACK_VALUE_SIZE(Value_Type, label, size) struct { Value_Type label##_stack[size]; U32 label##_stack_count; }
  
#define UI_STACK_AND_NEXT_VALUE(Value_Type, label) \
UI_NEXT_VALUE(Value_Type, label); \
UI_STACK_VALUE_SIZE(Value_Type, label, 16)
  
	// NOTE(erb): immediatly setting values
  UI_NEXT_VALUE(V2f, floating_pos);
  UI_NEXT_VALUE(String8, debug_str);
  UI_NEXT_VALUE(GFX_Cursor_Kind, hover_cursor);
  
  // NOTE(erb): values stack
  UI_STACK_AND_NEXT_VALUE(F32, font_size);
  UI_STACK_AND_NEXT_VALUE(F32, font_char_spacing);
  UI_STACK_AND_NEXT_VALUE(F32, font_blank_spacing);
  UI_STACK_AND_NEXT_VALUE(F32, line_spacing);
  UI_STACK_AND_NEXT_VALUE(F32, child_gap);
  UI_STACK_AND_NEXT_VALUE(F32, border_thickness);
  UI_STACK_AND_NEXT_VALUE(F32, corner_radius);
  UI_STACK_AND_NEXT_VALUE(V4f, font_color);
  UI_STACK_AND_NEXT_VALUE(V4f, border_color);
  UI_STACK_AND_NEXT_VALUE(Axis2, layout_axis);
  UI_STACK_AND_NEXT_VALUE(UI_Key, seed_key);
  UI_STACK_AND_NEXT_VALUE(UI_Align, align_x);
  UI_STACK_AND_NEXT_VALUE(UI_Align, align_y);
  UI_STACK_AND_NEXT_VALUE(Padding2, padding);
  UI_STACK_AND_NEXT_VALUE(Gradient2, background);
  UI_STACK_AND_NEXT_VALUE(UI_Sizing_Value, sizing_x);
  UI_STACK_AND_NEXT_VALUE(UI_Sizing_Value, sizing_y);
  
  // NOTE(erb): navigation
  UI_STACK_VALUE_SIZE(UI_Key, nav_interface_key, 256);
};

#endif //UI_H
