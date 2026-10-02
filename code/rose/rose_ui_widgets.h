/* date = August 26th 2026 0:23 pm */

#ifndef ROSE_UI_WIDGETS_H
#define ROSE_UI_WIDGETS_H

typedef struct Date_Input_State Date_Input_State;
struct Date_Input_State
{
  B32 date_end_follows_start;
  B32 time_end_follows_start;
  String8 last_error;
};

typedef struct Fancy_String8 Fancy_String8;
struct Fancy_String8
{
  String8 str;
  V4f color;
  
  Fancy_String8 *next;
};

typedef struct Fancy_String8_List Fancy_String8_List;
struct Fancy_String8_List
{
  Fancy_String8 *first;
  Fancy_String8 *last;
};

typedef enum UI_Cell_Flags
{
  UICellFlags_Clickable            = (1<<0),
  UICellFlags_DrawSubLabel         = (1<<1),
  UICellFlags_MoreSpace            = (1<<2),
  UICellFlags_DrawExpandArrowClose = (1<<3),
  UICellFlags_DrawExpandArrowOpen  = (1<<4),
  UICellFlags_DrawKeyMaps          = (1<<5),
  UICellFlags_FancyStrLabel        = (1<<6),
  UICellFlags_LineInput            = (1<<7),
  UICellFlags_BoolInput            = (1<<8),
  UICellFlags_RangeInput           = (1<<9),
} UI_Cell_Flags;

typedef struct UI_Cell UI_Cell;
struct UI_Cell
{
  UI_Cell_Flags flags;
  String8 label;
  String8 sub_label;
  Fancy_String8_List label_fancy_str;
  String8 filter_needle;
  Key_Combo_Set *key_map;
  void *data;
  
  UI_Line_Input_State *line_input;
  
  F32 *f32_out;
  Range_F32 f32_rng;
  
  B32 *b32_out;
  
  UI_Cell *next;
};

typedef struct UI_Lister_Result UI_Lister_Result;
struct UI_Lister_Result
{
  UI_Node *lister;
  UI_Cell *selected;
  UI_Cell *first;
  String8 input_str;
};

#endif //ROSE_UI_WIDGETS_H
