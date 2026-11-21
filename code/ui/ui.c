void ui_pop_parent(UI_Ctx *ctx);
UI_Node *ui_nodef(UI_Ctx *ctx, UI_Flags flags, String8 label, ...);

// ///////////////////////////////////////////////
// NOTE(erb): helpers
// ///////////////////////////////////////////////

UI_Key
ui_key_null()
{
  UI_Key result = {0};
  return(result);
}

UI_Key
ui_key_make(U64 v)
{
  UI_Key result = {0};
  result.u64 = v;
  return(result);
}

U64
ui_hash_from_key(UI_Key key)
{
  U64 result = key.u64;
  return(result);
}

B32
ui_key_equals(UI_Key a, UI_Key b)
{
  B32 result = (a.u64 == b.u64);
  return(result);
}

B32
ui_key_is_null(UI_Key key)
{
  B32 result = (key.u64 == 0);
  return(result);
}

U64
hash_combine(U64 a, U64 b)
{
  U64 result = a ^ (b + 0x9e3779b97f4a7c15ULL + (a<<6) + (a>>2));
  return(result);
}

UI_Key
ui_key_seed_with(UI_Key a, UI_Key b)
{
  U64 ha = ui_hash_from_key(a);
  U64 hb = ui_hash_from_key(b);
  UI_Key result = ui_key_make(hash_combine(ha, hb));
  return(result);
}

#define ui_parent(context, parent)             DeferLoop(ui_push_parent((context), (parent)), ui_pop_parent((context)))
#define ui_parentf(context, flags, label, ...) ui_parent((context), ui_nodef((context), (flags), (label), ##__VA_ARGS__))
#define ui_parent_temp(context, parent)        DeferLoop(ui_push_parent_temp((context), (parent)), ui_pop_parent_temp((context)))
#define ui_labelf(ctx, fmt, ...)               ui_parentf((ctx), UIFlags_DrawText, (fmt), ##__VA_ARGS__)

UI_Sizing_Value 
ui_fixed(F32 value)
{
	UI_Sizing_Value result = {0};
	result.sizing = UISizing_Fixed;
	result.value = value;
	return(result);
}

UI_Sizing_Value 
ui_grow()
{
	UI_Sizing_Value result = {0};
	result.sizing = UISizing_Grow;
	return(result);
}

UI_Sizing_Value 
ui_shrink()
{
	UI_Sizing_Value result = {0};
	result.sizing = UISizing_Shrink;
	return(result);
}

UI_Sizing_Value 
ui_fit()
{
	UI_Sizing_Value result = {0};
	result.sizing = UISizing_Fit;
	return(result);
}

Gradient2
grad2(V4f color0, V4f color1, Orientation2 orient)
{
  Gradient2 result = {0};
  result.color0 = color0;
  result.color1 = color1;
  result.orientation = orient;
  return(result);
}

Gradient2
grad2_single(V4f color)
{
  Gradient2 result = grad2(color, color, Orientation2_NS);
  return(result);
}

Padding2
pad2(F32 left, F32 top, F32 right, F32 bottom)
{
  Padding2 result = {0};
  result.left = left;
  result.top = top;
  result.right = right;
  result.bottom = bottom;
  return(result);
}

Padding2
pad2_equal(F32 value)
{
  Padding2 result = {0};
  result.left_top = result.right_bottom = v2ff(value);
  return(result);
}

Padding2
pad2_xy(F32 x, F32 y)
{
  Padding2 result = {0};
  result.left = result.right = x;
  result.top = result.bottom = y;
  return(result);
}

UI_Radius
ui_rad(F32 value, UI_Radius_Kind kind)
{
  UI_Radius result = {0};
  result.value = value;
  result.kind = kind;
  return(result);
}

UI_Radius
ui_rad_abs(F32 value)
{
  UI_Radius result = ui_rad(value, UIRadiusKind_Absolute);
  return(result);
}

F32
padding_axis_sum(Padding2 padding, Axis2 axis)
{
  Assert(axis >= 0 && axis < ArraySize(padding.c));
  Assert((axis+2) >= 0 && (axis+2) < ArraySize(padding.c));
  F32 result = (padding.c[axis] + padding.c[axis+2]);
  return(result);
}

F32
node_total_child_gap(UI_Node *node)
{
  Assert(node);
  F32 result = Cap0(((I32)node->layout_children_count - 1))*node->child_gap;
  return(result);
}

Range_V2f
ui_node_rng_v2f(UI_Node *node)
{
  Assert(node);
  Range_V2f result = rng_v2f_point_add(node->pos, node->size);
  return(result);
}

Range_V2f
ui_node_rng_v2f_inner(UI_Node *node)
{
  Range_V2f result = ui_node_rng_v2f(node);
  result.min = v2f_add(result.min, node->padding.left_top);
  result.max = v2f_sub(result.max, node->padding.right_bottom);
  return(result);
}

// ///////////////////////////////////////////////
// NOTE(erb): set next and stack function

#define UIResetStack(ctx, label)               ((ctx)->label##_stack_count = 0)
#define UIGetStackValue(ctx, label)       (Assert((ctx)->label##_stack_count >= 1), (ctx)->label##_stack[ctx->label##_stack_count-1])
#define UIHasNextValue(ctx, label)        ((ctx)->has_next_##label)
#define UINextValue(ctx, label)           (Assert(UIHasNextValue(ctx, label)), (ctx)->next_##label)
#define UIPopNextValue(ctx, label)        (Assert(UIHasNextValue(ctx, label)), (ctx)->has_next_##label = false, (ctx)->next_##label)
#define UIGetNextOrStackValue(ctx, label) (UIHasNextValue((ctx), label) ? UIPopNextValue((ctx), label) : UIGetStackValue((ctx), label))

#define UI_SET_NEXT_FUNC(Value_Type, label) \
void ui_set_next_##label(UI_Ctx *ctx, Value_Type value) { \
Assert(!(ctx->has_next_##label)); \
ctx->has_next_##label = true; \
ctx->next_##label = value; \
}
#define UI_PUSH_POP_FUNC(Value_Type, label) \
void ui_push_##label(UI_Ctx *ctx, Value_Type value) { \
Assert(ctx->label##_stack_count + 1 < ArraySize(ctx->label##_stack)); \
ctx->label##_stack[ctx->label##_stack_count++] = value; \
} \
void ui_pop_##label(UI_Ctx *ctx) { \
Assert(ctx->label##_stack_count > 0); \
ctx->label##_stack_count--; \
} 
#define UI_SET_NEXT_AND_PUSH_POP_FUNC(Value_Type, label) \
UI_SET_NEXT_FUNC(Value_Type, label) \
UI_PUSH_POP_FUNC(Value_Type, label)

UI_SET_NEXT_FUNC(V2f, floating_pos)
UI_SET_NEXT_FUNC(String8, debug_str)
UI_SET_NEXT_FUNC(GFX_Cursor_Kind, hover_cursor)

// NOTE(erb): layout axis
#define ui_layout_axis(context, value) DeferLoop(ui_push_layout_axis(context, value), ui_pop_layout_axis(context))
UI_SET_NEXT_AND_PUSH_POP_FUNC(Axis2, layout_axis)

// NOTE(erb): font size
#define ui_font_size(context, value) DeferLoop(ui_push_font_size(context, value), ui_pop_font_size(context))
UI_SET_NEXT_AND_PUSH_POP_FUNC(F32, font_size)

// NOTE(erb): font word spacing
#define ui_font_word_spacing(context, value) DeferLoop(ui_push_font_blank_spacing(context, value), ui_pop_font_blank_spacing(context))
UI_SET_NEXT_AND_PUSH_POP_FUNC(F32, font_blank_spacing)

// NOTE(erb): char spacing
#define ui_font_char_spacing(context, value) DeferLoop(ui_push_font_char_spacing(context, value), ui_pop_font_char_spacing(context))
UI_SET_NEXT_AND_PUSH_POP_FUNC(F32, font_char_spacing)

// NOTE(erb): line spacing
#define ui_line_spacing(context, value) DeferLoop(ui_push_line_spacing(context, value), ui_pop_line_spacing(context))
UI_SET_NEXT_AND_PUSH_POP_FUNC(F32, line_spacing)

// NOTE(erb): child gap
#define ui_child_gap(context, value) DeferLoop(ui_push_child_gap(context, value), ui_pop_child_gap(context))
UI_SET_NEXT_AND_PUSH_POP_FUNC(F32, child_gap)

// NOTE(erb): font color
#define ui_font_color(context, value) DeferLoop(ui_push_font_color(context, value), ui_pop_font_color(context))
UI_SET_NEXT_AND_PUSH_POP_FUNC(V4f, font_color)

// NOTE(erb): background 
#define ui_background(context, value) DeferLoop(ui_push_background(context, value), ui_pop_background(context))
UI_SET_NEXT_AND_PUSH_POP_FUNC(Gradient2, background)

#define ui_background_color(context, value) ui_background(context, grad2_single((value)))

// NOTE(erb): border color
#define ui_border_color(context, value) DeferLoop(ui_push_border_color(context, value), ui_pop_border_color(context))
UI_SET_NEXT_AND_PUSH_POP_FUNC(V4f, border_color)

// NOTE(erb): padding
#define ui_padding(context, value) DeferLoop(ui_push_padding(context, value), ui_pop_padding(context))
UI_SET_NEXT_AND_PUSH_POP_FUNC(Padding2, padding)

// NOTE(erb): sizing
#define ui_sizing_x(context, value) DeferLoop(ui_push_sizing_x(context, value), ui_pop_sizing_x(context))
UI_SET_NEXT_AND_PUSH_POP_FUNC(UI_Sizing_Value, sizing_x)

#define ui_sizing_y(context, value) DeferLoop(ui_push_sizing_y(context, value), ui_pop_sizing_y(context))
UI_SET_NEXT_AND_PUSH_POP_FUNC(UI_Sizing_Value, sizing_y)

#define ui_sizing(context, x, y) ui_sizing_x((context), (x)) ui_sizing_y((context), (y))

// NOTE(erb): navigation
UI_PUSH_POP_FUNC(UI_Key, nav_interface_key)

UI_Key
ui_top_nav_interface(UI_Ctx *ui)
{
  UI_Key result = UIGetStackValue(ui, nav_interface_key);
  return(result);
}

B32
ui_is_top_nav_interface(UI_Ctx *ui, UI_Key key)
{
  B32 result = ui_key_equals(key, ui_top_nav_interface(ui));
  return(result);
}

// NOTE(erb): helpers

void
ui_set_next_sizing(UI_Ctx *ui, UI_Sizing_Value x, UI_Sizing_Value y)
{
  ui_set_next_sizing_x(ui, x);
  ui_set_next_sizing_y(ui, y);
}

void
ui_set_next_rect(UI_Ctx *ctx, Range_V2f rect)
{
	ui_set_next_floating_pos(ctx, rect.min);
  V2f size = rng_v2f_size(rect);
  ui_set_next_sizing(ctx, ui_fixed(size.x), ui_fixed(size.y));
}

// NOTE(erb): alignment
#define ui_align_x(context, value) DeferLoop(ui_push_align_x(context, value), ui_pop_align_x(context))
UI_SET_NEXT_AND_PUSH_POP_FUNC(UI_Align, align_x)

#define ui_align_y(context, value) DeferLoop(ui_push_align_y(context, value), ui_pop_align_y(context))
UI_SET_NEXT_AND_PUSH_POP_FUNC(UI_Align, align_y)

// NOTE(erb): border thickness
#define ui_border_thickness(context, value) DeferLoop(ui_push_border_thickness(context, value), ui_pop_border_thickness(context))
UI_SET_NEXT_AND_PUSH_POP_FUNC(F32, border_thickness)

// NOTE(erb): corner radius
#define ui_corner_radius(context, value) DeferLoop(ui_push_corner_radius(context, value), ui_pop_corner_radius(context))
UI_SET_NEXT_AND_PUSH_POP_FUNC(UI_Radius, corner_radius)

#define ui_corner_radius_abs(context, value) ui_corner_radius(ui, ui_rad_abs(value))
void
ui_set_next_corner_radius_abs(UI_Ctx *ui, F32 value)
{
  ui_set_next_corner_radius(ui, ui_rad_abs(value));
}

// NOTE(erb): specify all the border at once

#define ui_border(ui, color, thick, radius) \
ui_border_color(ui, (color)) \
ui_border_thickness(ui, (thick)) \
ui_corner_radius(ui, radius)

#define ui_border_abs(ui, color, thick, radius) ui_border(ui, color, thick, ui_rad_abs(radius))

void 
ui_set_next_border(UI_Ctx *ui, V4f color, F32 thick, UI_Radius radius)
{
  ui_set_next_border_color(ui, color);
  ui_set_next_border_thickness(ui, thick);
  if(radius.value != 0)
  {
    ui_set_next_corner_radius(ui, radius);
  }
}

#define ui_set_next_border_abs(ui, color, thick, radius) ui_set_next_border(ui, color, thick, ui_rad_abs(radius))

void
ui_set_next_background_color(UI_Ctx *ui, V4f color)
{
  ui_set_next_background(ui, grad2_single(color));
}

// NOTE(erb): seed key
#define ui_seed_key(context, value) DeferLoop(ui_push_seed_key(context, value), ui_pop_seed_key(context))
UI_SET_NEXT_AND_PUSH_POP_FUNC(UI_Key, seed_key)

// ///////////////////////////////////////////////
// NOTE(erb): internal stuff
// ///////////////////////////////////////////////

void
ui_build_node_cache(UI_Ctx *ctx, UI_Node *node)
{
  for(EachNode(UI_Node, child, node->first_child))
	{
		ui_build_node_cache(ctx, child);
	}
	U64 hash = ui_hash_from_key(node->key);
  if (hash != 0)
	{
		U64 idx = hash & (ArraySize(ctx->node_cache) - 1);
		UI_Node **free_slot = ctx->node_cache + idx;
		if (*free_slot) 
		{
			node->next_hash = (*free_slot);
		}
    *free_slot = node;
	}
}

file_local void
fit_size_node(UI_Node *node, Axis2 axis)
{
	Assert(node);
  
  F32 *node_size = &node->size.c[axis];
  F32 *node_min_size = &node->min_size.c[axis];
	
  // NOTE(erb): fixed size nodes don't have padding I guess?
	if (!(node->flags & UIFlags_DrawText) && node->sizing[axis] != UISizing_Fixed)
	{
    F32 padding = padding_axis_sum(node->padding, axis);
		*node_min_size += padding;
		*node_size += padding;
		
		if(node->layout_axis == axis)
		{
			*node_size += node_total_child_gap(node);
		}
	}
	
  if(!(node->flags & UIFlags_Floating))
  {
    UI_Node *parent = node->parent;
    if(parent)
    {
      F32 *parent_size = &parent->size.c[axis];
      F32 *parent_min_size = &parent->min_size.c[axis];
      
      if(parent->sizing[axis] != UISizing_Fixed)
      {
        if(parent->layout_axis == axis)
        {
          *parent_size += *node_size;
          *parent_min_size += *node_min_size;
        }
        else if(!(parent->flags & UIFlags_CrossOverlay))
        {
          *parent_size = Max(*parent_size, *node_size);
          *parent_min_size = Max(*parent_min_size, *parent_min_size);
        }
      }
    }
  }
}

file_local void 
fit_size_step(UI_Node *node, Axis2 axis)
{
  for(EachNode(UI_Node, child, node->first_child))
  {
		fit_size_step(child, axis);
  }
  fit_size_node(node, axis);
}

file_local void 
wrap_text_node(Arena *arena, UI_Node *node)
{
  SupressUnused(arena);
  SupressUnused(node);
  Assert(node);
  Assert(arena);
  
  // NOTE(erb): iterate and wrap when needed.
  if ((node->flags & UIFlags_DrawText) && node->sizing[Axis2_X] == UISizing_Shrink)
  {
    Assert(node->text != 0);
    Implement();
  }
}

file_local void 
wrap_text_step(Arena *arena, UI_Node *node)
{
  for(EachNode(UI_Node, child, node->first_child))
  {
    wrap_text_node(arena, child);
  }
}

file_local void 
grow_and_shrink_size(UI_Node *parent, Axis2 axis)
{
  Assert(parent);
  
  F32 padding = padding_axis_sum(parent->padding, axis);
  F32 extra_size = parent->size.c[axis] - padding;
  
	if (parent->layout_axis == axis)
	{
    typedef struct List_Item List_Item;
    struct List_Item
    {
      UI_Node *node;
      List_Item *prev;
      List_Item *next;
    };
    
    typedef struct Node_List Node_List;
    struct Node_List
    {
      List_Item *first;
      List_Item *last;
    };
    
    Scratch scratch = scratch_begin(0, 0);
    Node_List shrink_list = {0};
    Node_List grow_list = {0};
    
    // NOTE(erb): build grow and shrink list
    for(EachNode(UI_Node, child, parent->first_child))
    {
      if (!(child->flags & UIFlags_Floating))
      {
        UI_Sizing sizing = child->sizing[axis];
        
        if(sizing == UISizing_Grow || 
           sizing == UISizing_Shrink)
        {
          List_Item *item = PushStruct(scratch.arena, List_Item);
          item->node = child;
          if(sizing==UISizing_Grow)
          {
            DLLQueueAppend(grow_list.first, grow_list.last, item);
          }
          else // sizing==UISizing_Shrink
          {
            DLLQueueAppend(shrink_list.first, shrink_list.last, item);
          }
        }
        extra_size -= child->size.c[axis];
      }
    }
    extra_size -= node_total_child_gap(parent);
    
    if (grow_list.first != 0)
    {
      for(;extra_size > 0.0001;)
      {
        F32 smallest = grow_list.first->node->size.c[axis];
        F32 second_smallest = f32_max; 
        F32 size_to_add = extra_size;
        
        U32 growable_count = 0;
        for(EachNode(List_Item, item, grow_list.first))
        {
          UI_Node *child = item->node;
          
          F32 size = child->size.c[axis];
          F32 max_size = child->max_size.c[axis];
          
          Assert(child->sizing[axis] == UISizing_Grow);
          if (size < max_size)
          {
            growable_count++;
            if (size < smallest)
            {
              second_smallest = smallest;
              smallest = size;
            }
            if (size > smallest)
            {
              second_smallest= Min(second_smallest, size);
              size_to_add = second_smallest - smallest;
            }
          }
        }
        
        if(growable_count == 0) 
        {  
          break;
        }
        
        size_to_add = Min(size_to_add, extra_size / growable_count);
        
        if(size_to_add == 0) 
        { 
          break;
        }
        
        U32 grown_count = 0;
        
        for(EachNode(List_Item, item, grow_list.first))
        {
          UI_Node *child = item->node;
          
          F32 *size = &child->size.c[axis];
          Assert(child->sizing[axis] == UISizing_Grow);
          if(*size == smallest)
          {
            if(*size < child->max_size.c[axis])
            {
              *size += size_to_add;
              extra_size -= size_to_add;
              grown_count++;
            }
          }
        }	
        
        
        if(grown_count == 0) 
        {
          break;
        }
      }
    }
    
    if(shrink_list.first != 0)
    {
      while(extra_size < -0.0001)
      {
        F32 largets = shrink_list.first->node->size.c[axis];
        F32 second_largest = 0;
        // NOTE(erb): this value is negative
        F32 size_to_sub = extra_size;
        
        U32 shrinkable_count = 0;
        for(EachNode(List_Item, item, shrink_list.first))
        {
          UI_Node *child = item->node;
          
          F32 size = child->size.c[axis];
          F32 minsize = child->min_size.c[axis];
          UI_Sizing sizing = child->sizing[axis];
          
          Assert(sizing == UISizing_Shrink);
          if(size > minsize)
          {
            shrinkable_count++;
            if(size > largets)
            {
              second_largest = largets;
              largets = size;
            }
            if(size < largets)
            {
              second_largest = Max(second_largest, size);
              size_to_sub = second_largest - largets;
            }
          }
        }
        
        if(shrinkable_count == 0) 
        {  
          break;
        }
        
        size_to_sub = Max(size_to_sub, extra_size / shrinkable_count);
        
        U32 shrunk_count = 0;
        for(EachNode(List_Item, item, shrink_list.first))
        {
          UI_Node *child = item->node;
          
          F32 *size = &child->size.c[axis];
          UI_Sizing sizing = child->sizing[axis];
          
          Assert(sizing == UISizing_Shrink);
          if (*size == largets)
          {
            if (*size > child->min_size.c[axis])
            {
              // NOTE(erb): WidthToSub is negative
              *size += size_to_sub;
              extra_size -= size_to_sub;
              shrunk_count++;
            }
          }	
        }
        
        if (shrunk_count == 0)
        {
          break;
        }
      }
      
      scratch_end(&scratch);
    }
	}
	else
	{
    for(EachNode(UI_Node, child, parent->first_child))
		{
			UI_Sizing sizing = child->sizing[axis];
			F32 *size = &child->size.c[axis];
			
			if (sizing == UISizing_Shrink) 
			{
				*size = Max(extra_size, child->min_size.c[axis]);
			}
			else if (sizing == UISizing_Grow)
			{
				*size = Min(extra_size, child->max_size.c[axis]);
			}
		}
	}
}

file_local void 
grow_and_shrink_step(UI_Node *parent, Axis2 axis)
{
	if (parent)
	{
		grow_and_shrink_size(parent, axis);
		
		for(EachNode(UI_Node, child, parent->first_child))
		{
			grow_and_shrink_step(child, axis);
		}
	}
}

U64 
index_from_cursor_delta(F32 delta, UI_Text *text)
{
  Assert(text);
  
  F32 line_width = 0;
  F32 scale_factor = text->face.size / text->face.font->height_px;
  
  U64 idx = 0;
  UTF8_Decode_Iterator it = utf8_it(text->content);
  for(;utf8_it_next(&it);++idx)
  {
    F32 letter_width = q2d_codepoint_width_or_blank(it.codepoint, text->face.font, 
                                                    scale_factor, text->face.blank_spacing);
    
    if(it.cursor < it.base_str.size)
    {
      letter_width += text->face.char_spacing;
    }
    
    if (delta <= line_width + letter_width)
    {
      // NOTE(erb): snap to forward or back char
      if (delta >= line_width + letter_width*0.5f)
      {
        idx++;
      }
      break;
    }
    line_width += letter_width;
  }
  return(idx);
}

file_local UI_Draw_Cmd *
push_draw_cmd(Arena *arena, UI_Draw_Cmd_List *draw_cmds, UI_Draw_Kind kind)
{
  UI_Draw_Cmd *cmd = PushStruct(arena, UI_Draw_Cmd);
  SLLQueueAppend(draw_cmds->first, draw_cmds->last, cmd);
  cmd->kind = kind;
  return(cmd);
}

UI_Draw_Cmd *
push_draw_cmd_text(Arena *arena, UI_Draw_Cmd_List *draw_cmds, UI_Text *text)
{
  UI_Draw_Cmd *cmd = push_draw_cmd(arena, draw_cmds, UIDrawKind_Text);
  cmd->text.color = text->color;
  cmd->text.font = text->face.font;
  cmd->text.font_size = text->face.size;
  cmd->text.blank_spacing = text->face.blank_spacing;
  cmd->text.char_spacing = text->face.char_spacing;
  return(cmd);
}

F32
resolve_corner_radius(UI_Node *node)
{
  Assert(node != 0);
  Assert(node->flags & UIFlags_DrawRounded);
  F32 value = node->corner_radius.value;
  F32 result = 0;
  switch(node->corner_radius.kind)
  {
    case UIRadiusKind_Absolute:
    {
      result = value;
    } break;
    
    case UIRadiusKind_PercentOfX:
    {
      Assert(value >= 0.f && value <= 1.f);
      result = value * node->size.x;
    } break;
    
    case UIRadiusKind_PercentOfY:
    {
      Assert(value >= 0.f && value <= 1.f);
      result = value * node->size.y;
    } break;
    
    case UIRadiusKind_PercentOfMax:
    {
      Assert(value >= 0.f && value <= 1.f);
      result = value * Max(node->size.x, node->size.y);
    } break;
    
    case UIRadiusKind_PercentOfMin:
    {
      Assert(value >= 0.f && value <= 1.f);
      result = value * Min(node->size.x, node->size.y);
    } break;
    
    InvalidDefaultCase;
  }
  return(result);
}

file_local void 
draw_node_cmd(Arena *arena, UI_Draw_Cmd_List *draw_cmds, UI_Node *node)
{
  Assert(draw_cmds);
  Assert(node);
  
	if(node->flags & UIFlags_DrawBackGround)
	{
		UI_Draw_Cmd *cmd = push_draw_cmd(arena, draw_cmds, UIDrawKind_Rect);
		cmd->range = ui_node_rng_v2f(node);
    cmd->background = node->background;
		if(node->flags & UIFlags_DrawRounded)
		{
			cmd->rect.corner_radius = resolve_corner_radius(node);
		}
	}
  
	if(node->flags & UIFlags_DrawBorder)
	{
    UI_Draw_Cmd *cmd = push_draw_cmd(arena, draw_cmds, UIDrawKind_RectLines);
		cmd->range = ui_node_rng_v2f(node);
    cmd->background = grad2_single(node->border_color);
    cmd->rect.border_thickness = node->border_thickness;
		if(node->flags & UIFlags_DrawRounded)
		{
			cmd->rect.corner_radius = resolve_corner_radius(node);
		}
	}
	
	if(node->flags & UIFlags_DrawText)
	{
		UI_Text *text = node->text;
		Assert(text);
		
		if(text->line_count > 0)
		{
      F32 line_height = text->face.size + text->face.line_spacing;
			U32 line_idx = 0;
			for(UI_Line_Node *line = text->first_line;
          line && line_idx < text->line_count;
          line = line->next, line_idx++)
			{
        UI_Draw_Cmd *cmd = push_draw_cmd_text(arena, draw_cmds, text);
        cmd->text.content = line->content;
				cmd->range.min = node->pos;
				cmd->range.min.y += line_idx*line_height;
			}
		}
		else
		{
      UI_Draw_Cmd *cmd = push_draw_cmd_text(arena, draw_cmds, text);
			cmd->text.content = text->content;
      cmd->range = ui_node_rng_v2f(node);
    }
	}
  
#if DEBUG
  // NOTE(erb): draw text highlight
  if(global_ui_debug_ctx->draw_text_highlight)
  {
    if(node->flags & UIFlags_DrawText)
    {
      UI_Draw_Cmd *cmd = push_draw_cmd(arena, draw_cmds, UIDrawKind_Rect);
      cmd->background = grad2_single(with_alpha(color_yellow, 0.5f));
      cmd->range = ui_node_rng_v2f(node);
    }
  }
  
  // NOTE(erb): draw border (outer size)
  if(global_ui_debug_ctx->draw_outer_size_line)
  {
    if(!(node->flags & UIFlags_DrawText))
    {
      UI_Draw_Cmd *cmd = push_draw_cmd(arena, draw_cmds, UIDrawKind_RectLines);
      cmd->background = grad2_single(with_alpha(color_debug, 0.8f));
      cmd->range = ui_node_rng_v2f(node);
      cmd->rect.border_thickness = 2.0f;
    }
  }
  
  // NOTE(erb): draw padding line (inner size)
  if(global_ui_debug_ctx->draw_inner_size_line)
  {
    if(!(node->flags & UIFlags_DrawText))
    {
      UI_Draw_Cmd *cmd = push_draw_cmd(arena, draw_cmds, UIDrawKind_RectLines);
      cmd->background = grad2_single(color_green);
      cmd->range = ui_node_rng_v2f_inner(node);
      cmd->rect.border_thickness = 2.0f;
    }
  }
#endif
}

file_local void 
draw_step(Arena *arena, UI_Draw_Cmd_List *draw_cmds, UI_Node *node)
{
  Assert(node);
  
  draw_node_cmd(arena, draw_cmds, node);
  
  if(node->flags & UIFlags_ClipRectangle)
  {
    UI_Draw_Cmd *cmd = push_draw_cmd(arena, draw_cmds, UIDrawKind_ClipBegin);
    cmd->range = ui_node_rng_v2f_inner(node);
    
    // NOTE(erb): to avoid overlapping from scissor regions
    AssertImplies(node->parent != 0, 
                  rng_v2f_contains_inclusive_range(ui_node_rng_v2f(node->parent), 
                                                   cmd->range));
  }
  
	for(EachNode(UI_Node, child, node->first_child))
  {
		draw_step(arena, draw_cmds, child);
  }
  
  if(node->flags & UIFlags_ClipRectangle)
  {
    push_draw_cmd(arena, draw_cmds, UIDrawKind_ClipEnd);
  }
}


// ///////////////////////////////////////////////
// NOTE(erb): usage
// ///////////////////////////////////////////////

void
ui_set_clipboard_api(UI_Ctx *ctx, Clipboard_API api)
{
  Assert(ctx);
  ctx->clipboard_api = api;
}

void
ui_set_cursor_api(UI_Ctx *ctx, Cursor_API api)
{
  Assert(ctx);
  ctx->cursor_api = api;
}

UI_Node *ui_node_make(UI_Ctx *ctx, UI_Flags flags, UI_Key key, String8 str);
void ui_build_node_cache(UI_Ctx *ctx, UI_Node *node);
void ui_push_parent(UI_Ctx *ctx, UI_Node *node);

void
ui_init(UI_Ctx *ctx, Arena *arena)
{
	ctx->arena = push_sub_arena(arena, sizeof(UI_Node)*500);
	ctx->last_arena = push_sub_arena(arena, sizeof(UI_Node)*500);
  ctx->input_state_arena = push_sub_arena(arena, (sizeof(UI_Line_Input_State) + 512)*32);
}

void 
ui_update_input_state_from_events(UI_Event_State *state, GFX_Event_List *event_list)
{
  for(EachU32(idx, GFXKey_COUNT))
  {
    Button_State *key = (state->keys + idx);
    key->half_transition_count = 0;
  }
  for(EachU32(idx, GFXMouseCode_COUNT))
  {
    Button_State *button = (state->buttons + idx);
    button->half_transition_count = 0;
  }
  ZeroStruct(&state->scroll);
  ZeroStruct(&state->scroll_clicks);
  state->past_cursor_delta = state->cursor_delta;
  V2f old_cursor = state->cursor;
  
  for(EachNode(GFX_Event, event, event_list->first))
  {
    switch(event->kind)
    {
      default: {} break;
      
      case GFXEventKind_WindowLoseFocus:
      {
      } break;
      
      case GFXEventKind_KeyDown:
      case GFXEventKind_KeyUp:
      {
        Button_State *key = (state->keys + event->key.code);
        key->ended_down = (event->kind == GFXEventKind_KeyDown);
        key->half_transition_count++;
      } break;
      
      case GFXEventKind_MouseDown:
      case GFXEventKind_MouseUp:
      {
        Button_State *button = (state->buttons + event->key.code);
        button->ended_down = (event->kind == GFXEventKind_MouseDown);
        button->half_transition_count++;
      } break;
      
      case GFXEventKind_MouseWheel:
      {
        state->scroll = v2f_add(state->scroll, event->mouse.wheel);
        state->scroll_clicks = v2i_add(state->scroll_clicks, event->mouse.wheel_clicks);
      } break;
      
      case GFXEventKind_MouseMove:
      {
        state->cursor = event->mouse.position;
      } break;
    }
  }
  state->cursor_delta = v2f_sub(state->cursor, old_cursor);
}

void
ui_set_cursor(UI_Ctx *ctx, GFX_Cursor_Kind kind)
{
  if(ctx->cursor_api.set_func != 0)
  {
    ctx->cursor_api.set_func(kind, ctx->cursor_api.data);
  }
}

void
clipboard_api_set_str8(Clipboard_API *api, String8 str)
{
  if(api->set_str8_func != 0)
  {
    api->set_str8_func(str, api->data);
  }
}

String8
clipboard_api_push_str8(Clipboard_API *api, Arena *arena)
{
  String8 result = {0};
  if(api->push_str8_func != 0)
  {
    result = api->push_str8_func(arena, api->data);
  }
  return(result);
}

void
ui_begin_frame(UI_Ctx *ctx, UI_Font_Face face, GFX_Event_List *events, Range_V2f root_rect, F32 delta_time)
{
  ctx->frame_tick += 1;
  ctx->frame_delta_time = delta_time;
  ctx->frame_total_time += delta_time;
  
  ui_update_input_state_from_events(&ctx->input, events);
  
  ZeroStruct(&ctx->hot_node);
  ctx->event_list = *events;
  ctx->default_font_face = face;
  ui_set_cursor(ctx, GFXCursorKind_Arrow);
  
  UIResetStack(ctx, padding);
  UIResetStack(ctx, align_x);
  UIResetStack(ctx, align_y);
	UIResetStack(ctx, font_size);
	UIResetStack(ctx, font_blank_spacing);
	UIResetStack(ctx, font_char_spacing);
	UIResetStack(ctx, line_spacing);
	UIResetStack(ctx, child_gap);
	UIResetStack(ctx, font_color);
	UIResetStack(ctx, background);
	UIResetStack(ctx, border_color);
  UIResetStack(ctx, seed_key);
	UIResetStack(ctx, sizing_x);
	UIResetStack(ctx, sizing_y);
	UIResetStack(ctx, corner_radius);
	UIResetStack(ctx, border_thickness);
	UIResetStack(ctx, layout_axis);
  
  if(ctx->frame_tick == 1)
  {
    ui_push_nav_interface_key(ctx, ui_key_null());
  }
	
	ui_push_padding(ctx, pad2_equal(0));
  ui_push_align_x(ctx, UIAlign_Start);
  ui_push_align_y(ctx, UIAlign_Start);
	ui_push_font_size(ctx, face.size);
	ui_push_font_char_spacing(ctx, face.char_spacing);
	ui_push_font_blank_spacing(ctx, face.blank_spacing);
	ui_push_line_spacing(ctx, face.line_spacing);
	ui_push_child_gap(ctx, 3);
	ui_push_font_color(ctx, color_black);
	ui_push_background(ctx, grad2_single(color_debug));
	ui_push_border_color(ctx, color_debug);
  ui_push_seed_key(ctx, ui_key_null());
	ui_push_sizing_x(ctx, ui_fit());
	ui_push_sizing_y(ctx, ui_fit());
	ui_push_corner_radius(ctx, ui_rad_abs(2));
	ui_push_border_thickness(ctx, 1);
  ui_push_layout_axis(ctx, Axis2_X);
	
  ui_set_next_rect(ctx, root_rect);
  ui_set_next_debug_str(ctx, S8("root"));
  ui_push_parent(ctx, ui_nodef(ctx, UIFlags_Floating, S8("root_panel_%p"), ctx));
}

void
ui_end_frame(UI_Ctx *ctx)
{
  Assert(ctx->current = ctx->root);
  
	mem_zero((U8 *)ctx->node_cache, ArraySize(ctx->node_cache)*sizeof(UI_Node *));
	
	ui_build_node_cache(ctx, ctx->root);
	arena_release(&ctx->last_arena);
	Swap(Arena, ctx->last_arena, ctx->arena);
  
	ctx->node_count = 0;
	ctx->root = 0;
  ctx->popup = 0;
}

F32 
get_align_multiplier(UI_Align align)
{
  function_persist F32 align_multipliers[] = {
    [UIAlign_Start] = 0.0f,
    [UIAlign_Center] = 0.5f,
    [UIAlign_End] = 1.0f,
  };
  F32 result = align_multipliers[align];
  return(result);
}

file_local void 
position_align_step(UI_Node *node)
{
  Assert(node);
  
  Axis2 axis = node->layout_axis;
  Axis2 cross = (node->layout_axis+1)%2;
  
  F32 axis_align_multiplier = get_align_multiplier(node->align[axis]);
  F32 cross_align_multiplier = get_align_multiplier(node->align[cross]);
  
  Range_V2f inner = ui_node_rng_v2f_inner(node);
	V2f advance_pos = inner.start;
  
  V2f container_space = rng_v2f_size(inner);
  F32 axis_space_remaining = container_space.c[axis];
  
  for(EachNode(UI_Node, child, node->first_child))
  {
		if (!(child->flags & UIFlags_Floating))
		{
			child->pos = advance_pos;
      advance_pos.c[axis] += child->size.c[axis];
      if (child->next)
      {
        advance_pos.c[axis] += node->child_gap;
        axis_space_remaining -= node->child_gap;
      }
      axis_space_remaining -= child->size.c[axis];
		}
		position_align_step(child);
  }
  
  advance_pos = inner.start;
  advance_pos.c[axis] += axis_space_remaining*axis_align_multiplier;
  
  for(EachNode(UI_Node, child, node->first_child))
  {
		if (!(child->flags & UIFlags_Floating))
		{
			child->pos = advance_pos;
      child->pos.c[cross] += (container_space.c[cross] - child->size.c[cross])*cross_align_multiplier;
      
			advance_pos.c[axis] += child->size.c[axis];
      if (child->next)
      {
        advance_pos.c[axis] += node->child_gap;
      }
		}
		position_align_step(child);
  }
}

UI_Draw_Cmd_List
ui_layout_and_draw(UI_Ctx *ctx, Arena *arena)
{
	Assert(ctx->current == ctx->root);
  
  fit_size_step(ctx->root, Axis2_X);
  grow_and_shrink_step(ctx->root, Axis2_X);
  wrap_text_step(&ctx->arena, ctx->root);
	fit_size_step(ctx->root, Axis2_Y);
	grow_and_shrink_step(ctx->root, Axis2_Y);
  position_align_step(ctx->root);
	
	UI_Draw_Cmd_List draw_cmds = {0};
	draw_step(arena, &draw_cmds, ctx->root);
  
	return(draw_cmds);
}

UI_Node *
ui_node_from_key(UI_Ctx *ctx, UI_Key key)
{
  UI_Node *result = 0;
  
  U64 index = ui_hash_from_key(key) & (ArraySize(ctx->node_cache) - 1);
  for(UI_Node *test_node = ctx->node_cache[index];
      test_node != 0; 
      test_node = test_node->next_hash)
  {
    if(ui_key_equals(test_node->key, key))
    {
      result = test_node;
      break;
    }
  }
  
  return(result);
}

F32
measure_str8_width(Q2D_Font *font, String8 str, F32 height, F32 word_spacing, F32 char_spacing)
{
  Assert(height != 1.0f);
  Assert(font->height_px != 0.0f);
  F32 scale = (height / font->height_px);
  F32 width = 0;
  
  UTF8_Decode_Iterator it = utf8_it(str);
  for(;utf8_it_next(&it);)
  {
    width += q2d_codepoint_width_or_blank(it.codepoint, font, scale, word_spacing);
    if(it.cursor < it.base_str.size)
    {
      width += char_spacing;
    }
  }
  return(width);
}

F32
ui_font_face_vadvance(UI_Font_Face *face)
{
  Assert(face->size != 1.0f);
  Assert(face->font->height_px != 0.0f);
  F32 scale = (face->size/face->font->height_px);
  F32 height = scale * fnt_get_vadvance_px(&face->font->data);
  return(height);
}

V2f
ui_measure_str8_face(String8 str, UI_Font_Face *face)
{
  V2f result = {0};
  result.x = measure_str8_width(face->font, str, face->size, face->blank_spacing, face->char_spacing);
  result.y = ui_font_face_vadvance(face);
  return(result);
}

V2f 
ui_measure_text(UI_Text *text)
{
  V2f result = ui_measure_str8_face(text->content, &text->face);
  return(result);
}

UI_Font_Face
ui_font_face_from_ctx(UI_Ctx *ctx)
{
  UI_Font_Face face = {0};
  face.font = ctx->default_font_face.font;
  face.size = UIGetNextOrStackValue(ctx, font_size);
  face.char_spacing = UIGetNextOrStackValue(ctx, font_char_spacing);
  face.blank_spacing = UIGetNextOrStackValue(ctx, font_blank_spacing);
  face.line_spacing = UIGetNextOrStackValue(ctx, line_spacing);
  return(face);
}

UI_Node *
ui_node_make(UI_Ctx *ctx, UI_Flags flags, UI_Key key, String8 str)
{
  Assert(ctx);
	UI_Node *node = PushStruct(&ctx->arena, UI_Node);
  
	node->max_size = v2ff(f32_max);
  node->flags = flags;
  
  AssertImplies(flags & UIFlags_Comm, !ui_key_is_null(key));
  if(!(flags & UIFlags_DontSeedKey))
  {
    key = ui_key_seed_with(key, UIGetNextOrStackValue(ctx, seed_key));
  }
  node->key = key;
  
  AssertImplies(flags & UIFlags_HoverEffect, flags & UIFlags_Comm);
  
  node->padding = UIGetNextOrStackValue(ctx, padding);
	node->child_gap = UIGetNextOrStackValue(ctx, child_gap);
  
  node->align[Axis2_X] = UIGetNextOrStackValue(ctx, align_x);
  node->align[Axis2_Y] = UIGetNextOrStackValue(ctx, align_y);
  
  UI_Sizing_Value sizing_x = UIGetNextOrStackValue(ctx, sizing_x);
  node->sizing[Axis2_X] = sizing_x.sizing;
	node->size.c[Axis2_X] = sizing_x.value;
	
  UI_Sizing_Value sizing_y = UIGetNextOrStackValue(ctx, sizing_y);
  node->sizing[Axis2_Y] = sizing_y.sizing;
	node->size.c[Axis2_Y] = sizing_y.value;
  
  node->layout_axis = UIGetNextOrStackValue(ctx, layout_axis);
  
  AssertImplies((UIHasNextValue(ctx, border_thickness) || UIHasNextValue(ctx, border_color)), 
                (flags & UIFlags_DrawBorder));
	if(flags & UIFlags_DrawBorder)
	{
		node->border_thickness = UIGetNextOrStackValue(ctx, border_thickness);
    node->border_color = UIGetNextOrStackValue(ctx, border_color);
	}
  
  AssertImplies(UIHasNextValue(ctx, corner_radius), (flags & UIFlags_DrawRounded));
	if(flags & UIFlags_DrawRounded)
	{
		node->corner_radius = UIGetNextOrStackValue(ctx, corner_radius);
	}
  
  AssertImplies(UIHasNextValue(ctx, background), (flags & UIFlags_DrawBackGround));
	if(flags & UIFlags_DrawBackGround)
	{
		node->background = UIGetNextOrStackValue(ctx, background);
	}
	
  if(UIHasNextValue(ctx, hover_cursor))
  {
    Assert(flags & UIFlags_Comm);
    node->hover_cursor = UIPopNextValue(ctx, hover_cursor);
  }
  
	if(UIHasNextValue(ctx, floating_pos))
	{
    Assert(flags & UIFlags_Floating);
    node->pos = UIPopNextValue(ctx, floating_pos);
	}
	
	if (node->flags & UIFlags_DrawText)
	{
		UI_Text *text = PushStruct(&ctx->arena, UI_Text);
    text->face = ui_font_face_from_ctx(ctx);
		text->color = UIGetNextOrStackValue(ctx, font_color);
		text->content = push_str8_copy(&ctx->arena, str);
		text->first_line = 0;
		
		V2f text_size = ui_measure_text(text);
		V2f min_text_size = text_size;
		V2f max_text_size = text_size;
    
    if (node->sizing[Axis2_X] == UISizing_Shrink) 
    {
      // NOTE(erb): find largest word
      I32 word_count;
      String8 largest_word = {0};
      {
        Scratch scratch = scratch_begin(0, 0);
        String8_List words = str8_split_blank(scratch.arena, str);
        // TODO(erb): convert this to utf8 measure since this only measures bytes, should be codepoints
        for(EachNode(String8_Node, str_node, words.first))
        {
          if(str_node->str.size > largest_word.size)
          {
            largest_word = str_node->str;
          }
        }
        word_count = (I32)words.count;
        scratch_end(&scratch);
      }
      
      V2f largest_word_size = ui_measure_str8_face(largest_word, &text->face);
      
      max_text_size.y = word_count*text->face.size + Cap0(word_count - 1)*text->face.line_spacing;
      min_text_size.x = largest_word_size.x;
      node->sizing[Axis2_Y] = UISizing_Grow;
    }
    
		node->text = text;
		node->size = text_size;
		node->min_size = min_text_size;
		node->max_size = max_text_size;
	}
  
#if DEBUG
  if(UIHasNextValue(ctx, debug_str))
	{
    node->debug_str = UIPopNextValue(ctx, debug_str);
	}
  else
  {
    node->debug_str = push_str8_copy(&ctx->arena, str);
  }
#endif
  
	return(node);
}

UI_Key
ui_keyf_varg(const char *fmt, va_list args_src)
{
  va_list args;
  va_copy(args, args_src);
  
  Scratch scratch = scratch_begin(0, 0);
  String8 resolved_key = push_str8f_varg(scratch.arena, fmt, args_src);
  UI_Key key = ui_key_make(str8_hash(resolved_key));
  scratch_end(&scratch);
  va_end(args);
  return(key);
}

UI_Key
ui_keyf(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  UI_Key key = ui_keyf_varg(fmt, args);
  va_end(args);
  return(key);
}

typedef struct UI_String8_Key UI_String8_Key;
struct UI_String8_Key
{
  String8 str;
  UI_Key key;
};

UI_String8_Key
ui_parse_label_str_key_varg(Arena *arena, String8 label, va_list args_src)
{
  String8 key_start_markup = S8("##");
  
  Scratch scratch = scratch_begin(&arena, 1);
  
  va_list args;
  va_copy(args, args_src);
  String8 resolved = push_str8f_varg(arena, push_cstr(scratch.arena, label), args);
  va_end(args);
  
  String8 str_part = resolved;
  String8 key_part = resolved;
  Index_U64 markup = str8_find(resolved, key_start_markup, 0);
  if(markup.exists)
  {
    str_part = str8_prefix(resolved, markup.value);
    key_part = str8_postfix(resolved, markup.value + key_start_markup.size);
  }
  
  UI_String8_Key result = {0};
  result.str = str_part;
  result.key = ui_key_make(str8_hash(key_part));
  
  scratch_end(&scratch);
  return(result);
}

UI_Node *
ui_nodef_varg(UI_Ctx *ctx, UI_Flags flags, String8 label, va_list args_src)
{
  
  Scratch scratch = scratch_begin(0, 0);
  
  va_list args;
  va_copy(args, args_src);
  UI_String8_Key str_key = ui_parse_label_str_key_varg(scratch.arena, label, args);
  va_end(args);
  
  UI_Node *node = ui_node_make(ctx, flags, str_key.key, str_key.str);
  scratch_end(&scratch);
  return(node);
}

// NOTE(erb): label contains str and the key in the format
// ui_node(ui, 0, "button##%d", idx)
UI_Node *
ui_nodef(UI_Ctx *ctx, UI_Flags flags, String8 label, ...)
{
  va_list args;
  va_start(args, label);
  UI_Node *node = ui_nodef_varg(ctx, flags, label, args);
  va_end(args);
  return(node);
}

void
ui_push_parent(UI_Ctx *ctx, UI_Node *node)
{
  Assert(node);
	if(ctx->root == 0)
	{
		ctx->root = node;
		node->pos = ctx->root_pos;
	}
	else
	{
		UI_Node *parent = ctx->current;
    Assert(parent);
    SLLQueueAppend(parent->first_child, parent->last_child, node);
    if(!(node->flags & UIFlags_Floating))
    {
      parent->layout_children_count++;
    }
		node->parent = parent;
	}
	ctx->current = node;
  ctx->node_count++;
}

void
ui_pop_parent(UI_Ctx *ctx)
{
	Assert(ctx->current != 0);
	ctx->current = ctx->current->parent;
}

void
ui_push_parent_temp(UI_Ctx *ctx, UI_Node *node)
{
  node->parent = ctx->current;
  ctx->current = node;
}

UI_Node *
ui_pop_parent_temp(UI_Ctx *ctx)
{
  Assert(ctx->current != 0);
  UI_Node *temp = ctx->current;
  ctx->current = temp->parent;
  temp->parent = 0;
  return(temp);
}


B32
key_down(UI_Ctx *ctx, GFX_Key code)
{
  Button_State *key = (ctx->input.keys + code);
  B32 result = (key->ended_down);
  return(result);
}

B32
key_pressed(UI_Ctx *ctx, GFX_Key code)
{
  Button_State *key = (ctx->input.keys + code);
  B32 result = (key->ended_down && key->half_transition_count > 0);
  return(result);
}

B32
mouse_button_down(UI_Ctx *ctx, GFX_Mouse_Code code)
{
  B32 result = ctx->input.buttons[code].ended_down;
  return(result);
}

B32
mouse_button_pressed(UI_Ctx *ctx, GFX_Mouse_Code code)
{
  Button_State *button = (ctx->input.buttons + code);
  B32 result = (button->ended_down && button->half_transition_count > 0);
  return(result);
}

B32
mouse_button_released(UI_Ctx *ctx, GFX_Mouse_Code code)
{
  Button_State *button = (ctx->input.buttons + code);
  B32 result = (!button->ended_down && button->half_transition_count > 0);
  return(result);
}

UI_Comm 
ui_comm_from_node(UI_Ctx *ui, UI_Node *node)
{
	UI_Comm result = {0};
  Assert(node->flags & UIFlags_Comm);
  if(node->flags & UIFlags_Comm)
  {
    UI_Node *cached = ui_node_from_key(ui, node->key);
    result.node = cached;
    
    // NOTE(erb): the way this is written now, if you change the container with the comm information
    // like "grow it if it is being hovered", the hit testing will still happen with the "old" information
    // because you use the calculating immediatly and not the resulting calculation, something to keep in mind
    // Maybe it is best to have some kind of flag like (UIFlags_CommRectFromCached or UIFlags_CommImmediateIfPossible - first option might be best)
    Range_V2f rect = {0};
    B32 has_fixed_rect = ((node->flags & UIFlags_Floating) 
                          && node->sizing[Axis2_X] == UISizing_Fixed 
                          && node->sizing[Axis2_Y] == UISizing_Fixed);
    if(has_fixed_rect && !(node->flags & UIFlags_CommRectFromCached))
    {
      rect = ui_node_rng_v2f(node);
    }
    else if(cached)
    {
      rect = ui_node_rng_v2f(cached);
    }
    
    result.hovered = rng_v2f_contains_inclusive(rect, ui->input.cursor);
    if(result.hovered)
    {
      ui->hot_node = node->key;
    }
    
    B32 is_hot = ui_key_equals(ui->hot_node, node->key);
    if(is_hot)
    {
      result.scroll = ui->input.scroll;
    }
    
    B32 mouse_left_released = mouse_button_released(ui, GFXMouseCode_Left);
    B32 mouse_left_pressed = mouse_button_pressed(ui, GFXMouseCode_Left);
    B32 mouse_left_down = mouse_button_down(ui, GFXMouseCode_Left);
    
    B32 mouse_right_released = mouse_button_released(ui, GFXMouseCode_Right);
    B32 mouse_right_pressed = mouse_button_pressed(ui, GFXMouseCode_Right);
    B32 mouse_right_down = mouse_button_down(ui, GFXMouseCode_Right);
    
    B32 is_active = ui_key_equals(ui->active_node, node->key);
    if(is_active) 
    {
      result.left_down = mouse_left_down;
      result.right_down = mouse_right_down;
      if(mouse_left_released || mouse_right_released)
      {
        if(is_hot)
        {
          result.left_clicked = mouse_left_released;
          result.right_clicked = mouse_right_released;
        }
        ZeroStruct(&ui->active_node);
      }
    }
    else if(is_hot)
    {
      result.hovered = true;
      if(mouse_left_pressed || mouse_right_pressed) 
      {
        ui->active_node = node->key;
        result.left_pressed = mouse_left_pressed;
        result.right_pressed = mouse_right_pressed;
      }
      ZeroStruct(&ui->hot_node);
    }
    
    result.pressed_out = (!result.hovered && (mouse_left_pressed || mouse_right_pressed));
    
    if(result.hovered)
    {
      ui_set_cursor(ui, node->hover_cursor);
      if(node->flags & UIFlags_HoverEffect)
      {
        // TODO(erb): this is test code
        F32 a = (is_active ? 0.75f : 0.5f); 
        V4f accent = v4f_lerp(node->background.color0, color_white, a);
        node->background = grad2(node->background.color0,
                                 accent, Orientation2_NS);
      }
    }
    
    
    result.left_dragging = !result.left_pressed && result.left_down;
    
    if(result.left_clicked)
    {
      UIResetStack(ui, nav_interface_key);
      ui_push_nav_interface_key(ui, ui_key_null());
      ui_push_nav_interface_key(ui, node->key);
    }
    
    if(ui_key_equals(node->key, UIGetStackValue(ui, nav_interface_key)) &&
       key_pressed(ui, GFXKey_Enter))
    {
      result.left_clicked = true;
    }
  }
  return(result);
}

// ///////////////////////////////////////////////
// NOTE(erb): popup
// ///////////////////////////////////////////////

void
ui_push_popup(UI_Ctx *ui, UI_Node *node)
{
  ui_push_parent_temp(ui, node);
}

void
ui_pop_popup(UI_Ctx *ui)
{
  ui->popup = ui_pop_parent_temp(ui);
}

#define ui_popup(ui, node) DeferLoop(ui_push_popup(ui, node), ui_pop_popup(ui))

void
ui_push_center_popup(UI_Ctx *ui)
{
  Range_V2f root_rect = ui_node_rng_v2f(ui->root);
  V2f root_rect_size = rng_v2f_size(root_rect);
  V2f size = v2f_hada(root_rect_size, v2f(0.4f, 0.7f));
  size.x = CapBottom(size.x, 500);
  size.y = CapBottom(size.y, 500);
  Range_V2f rect = rng_v2f_center_size(rng_v2f_center(root_rect), size);
  
  ui_set_next_rect(ui, rect);
  ui_set_next_debug_str(ui, S8("ui_ctx_popup_rect"));
  UI_Node *node = ui_nodef(ui, UIFlags_Floating|UIFlags_ClipRectangle, S8Z());
  ui_push_seed_key(ui, ui_keyf("ui_ctx_popup"));
  
  ui_push_popup(ui, node);
}

void
ui_pop_center_popup(UI_Ctx *ui)
{
  ui_pop_seed_key(ui);
  ui_pop_popup(ui);
}

#define ui_center_popup(ui) DeferLoop(ui_push_center_popup(ui), ui_pop_popup(ui))

#include "ui_basic_widgets.c"