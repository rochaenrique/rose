/* Date = December 6th 2025 8:27 am */

#ifndef UI_H
#define UI_H

#include "marker.h"
#include "marker_platform.h"

typedef enum UI_Flags
{
	UIFlags_None = 0,
	UIFlags_CrossOverlay = (1<<0),
	UIFlags_DrawText = (1<<1),
	UIFlags_DrawBackGround = (1<<2),
	UIFlags_DrawBorder = (1<<3),
	UIFlags_DrawRounded = (1<<4),
	UIFlags_Clickable = (1<<5),
	UIFlags_Floating = (1<<6),
  UIFlags_ClipRectangle = (1<<7),
} UI_Flags;

typedef struct UI_Key
{
  U64 content;
	U32 iter;
  U64 parent;
} UI_Key;

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

typedef struct UI_Sizing_Value
{
	UI_Sizing sizing;
	F32 value;
} UI_Sizing_Value;

typedef struct UI_Line_Node
{
  struct UI_Line_Node *next;
  String content;
} UI_Line_Node;

typedef struct UI_Text
{
	V4f color;
  F32 font_size;
  F32 line_spacing;
  F32 spacing;
  String content;
  Font_Data *font;
  U32 line_count;
  
  UI_Line_Node *first_line;
  UI_Line_Node *last_line;
} UI_Text; 

typedef struct Text_Cmd
{
  char single_char;
  Range_U64 replace_range;
  String replace_str;
  
  String copy_str;
  I64 new_cursor;
  I64 new_mark;
} Text_Cmd;

typedef struct Text_Edit_State
{
  I64 cursor;
  I64 mark;
} Text_Edit_State;

typedef enum Text_Action_Flags
{
  TextActionFlags_None = 0,
  TextActionFlags_Copy                = (1 << 0),
  TextActionFlags_Paste               = (1 << 1),
  TextActionFlags_Cut                 = (1 << 2),
  TextActionFlags_Delete              = (1 << 3),
  TextActionFlags_KeepMark            = (1 << 4),
  TextActionFlags_ZeroDeltaWithSelect = (1 << 5),
  TextActionFlags_WordScan            = (1 << 6),
  TextActionFlags_DeltaPickSelectSide = (1 << 7),
} Text_Action_Flags;

typedef struct Text_Action
{
  Text_Action_Flags flags;
  I64 delta;
  I32 code_point;
} Text_Action;

typedef enum UI_Draw_Kind
{
	UIDrawKind_Text,
	UIDrawKind_Rect,
	UIDrawKind_RectLines,
} UI_Draw_Kind;

typedef struct UI_Draw_Cmd 
{
	UI_Draw_Kind kind;
  Screen_Rect_F32 rect;
	V4f color;
	F32 border_thickness;
	F32 corner_radius;
	
  Font_Data *font;
	String content;
	F32 font_size;
	F32 spacing;
	
  struct UI_Draw_Cmd *next;
} UI_Draw_Cmd;

typedef struct UI_Draw_Cmd_List
{
  UI_Draw_Cmd *first;
  UI_Draw_Cmd *last;
} UI_Draw_Cmd_List;

typedef struct UI_Element
{
	V2f pos;
	V2f size;
	V2f min_size;
	V2f max_size;
  
  UI_Align align[2];
	UI_Sizing sizing[2];
	Axis2 layout_axis;
	
	UI_Flags flags;
	
	V4f background_color;
  Screen_Rect_F32 padding;
	V4f border_color;
	F32 border_thickness;
	F32 corner_radius;
  F32 child_gap;
  UI_Text *text;
  
  U32 children_count;
  
	struct UI_Element *first_child;
	struct UI_Element *last_child;
	struct UI_Element *parent;
	struct UI_Element *next;
	
	struct UI_Element *next_hash;
	
	UI_Key key;
  B32 toggled;
  U64 frames_pressed;
  
} UI_Element;

typedef struct UI_Element_Out
{
  UI_Element *element;
	B8 clicked;
	B8 hovered;
	B8 pressed;
  B8 pressed_outside;
  B8 dragging;
} UI_Element_Out;

#define UI_STACK_VALUE(label, Value_Type) \
Value_Type label##_stack[16]; \
U32 label##_stack_count

typedef struct UI_Ctx
{
	Arena arena;
	Arena last_arena;
  Font_Face default_font_face;
  
  Input_State *input;
  
	UI_Element *element_cache[256];
  UI_Draw_Cmd_List defered_draw;
  Text_Edit_State text_state;
	
	UI_Element *current;
	UI_Element *root;
	V2f root_pos;
	U32 element_count;
	
  UI_Key active_element;
	UI_Key hot_element;
	
	// NOTE(erb): values stack
  
  UI_STACK_VALUE(font_size, F32);
  UI_STACK_VALUE(font_spacing, F32);
	UI_STACK_VALUE(line_spacing, F32);
	UI_STACK_VALUE(child_gap, F32);
	UI_STACK_VALUE(font_color, V4f);
	UI_STACK_VALUE(background_color, V4f);
	UI_STACK_VALUE(border_color, V4f);
  UI_STACK_VALUE(padding, Screen_Rect_F32);
	UI_STACK_VALUE(sizing_x, UI_Sizing_Value);
  UI_STACK_VALUE(align_x, UI_Align);
  UI_STACK_VALUE(align_y, UI_Align);
	UI_STACK_VALUE(border_thickness, F32);
	UI_STACK_VALUE(corner_radius, F32);
  UI_STACK_VALUE(seed_key, U64);
  
	// NOTE(erb): immediatly setting values
	
	B32 has_next_layout_axis;
	Axis2 next_layout_axis;
	
	B32 has_next_floating_pos;
	V2f next_floating_pos;
	
	B32 has_next_fixed_size;
	V2f next_fixed_size;
} UI_Ctx;

#endif //UI_H
