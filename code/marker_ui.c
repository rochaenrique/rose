#include "marker_ui.h"
#include "marker_platform.c"

void ui_pop_parent(UI_Ctx *ctx);

// ///////////////////////////////////////////////
// NOTE(erb): helpers
// ///////////////////////////////////////////////

UI_Key ui_key_null()
{
	UI_Key result = {0};
	return result;
}

UI_Key ui_key_from_string(String str)
{
	UI_Key result = {0};
	result.content = hash_string(str);
	return result;
}

B32 ui_key_equals(UI_Key a, UI_Key b)
{
	B32 result = (a.content == b.content && a.iter == b.iter);
	return result;
}

U64 ui_hash_key(UI_Key key)
{
	U64 result = ((key.iter + 1) * key.content) ^ (key.parent * 69420);
	return result;
}

UI_Sizing_Value ui_fixed(F32 value)
{
	UI_Sizing_Value result = {0};
	result.sizing = UISizing_Fixed;
	result.value = value;
	return result;
}

UI_Sizing_Value ui_grow()
{
	UI_Sizing_Value result = {0};
	result.sizing = UISizing_Grow;
	return result;
}

UI_Sizing_Value ui_shrink()
{
	UI_Sizing_Value result = {0};
	result.sizing = UISizing_Shrink;
	return result;
}

UI_Sizing_Value ui_fit()
{
	UI_Sizing_Value result = {0};
	result.sizing = UISizing_Fit;
	return result;
}

void ui_set_next_floating_pos(UI_Ctx *ctx, V2f pos)
{
	ctx->has_next_floating_pos = true;
	ctx->next_floating_pos = pos;
}

void ui_set_next_fixed_size(UI_Ctx *ctx, V2f size)
{
	ctx->has_next_fixed_size = true;
	ctx->next_fixed_size = size;
}

void ui_set_next_rect(UI_Ctx *ctx, Screen_Rect_F32 rect)
{
	ui_set_next_floating_pos(ctx, rect.top_left);
	ui_set_next_fixed_size(ctx, rect_size(rect));
}

void ui_set_next_layout_axis(UI_Ctx *ctx, Axis2 axis)
{
	ctx->has_next_layout_axis = true;
	ctx->next_layout_axis = axis;
}

#define ui_reset_value_stack(label) ctx->label##_stack_count = 0

#define UI_PUSH_POP_FUNCS(label, Value_Type) \
void ui_push_##label(UI_Ctx *ctx, Value_Type value) \
{ \
assert(ctx->label##_stack_count + 1 < array_size(ctx->label##_stack)); \
ctx->label##_stack[ctx->label##_stack_count++] = value; \
} \
void ui_pop_##label(UI_Ctx *ctx) \
{ \
assert(ctx->label##_stack_count > 0); \
ctx->label##_stack_count--; \
}

#define ui_font_size(context, value) defer_loop(ui_push_font_size(context, value), ui_pop_font_size(context))
UI_PUSH_POP_FUNCS(font_size, F32)

#define ui_font_spacing(context, value) defer_loop(ui_push_font_spacing(context, value), ui_pop_font_spacing(context))
UI_PUSH_POP_FUNCS(font_spacing, F32)

#define ui_line_spacing(context, value) defer_loop(ui_push_line_spacing(context, value), ui_pop_line_spacing(context))
UI_PUSH_POP_FUNCS(line_spacing, F32)

#define ui_child_gap(context, value) defer_loop(ui_push_child_gap(context, value), ui_pop_child_gap(context))
UI_PUSH_POP_FUNCS(child_gap, F32)

#define ui_font_color(context, value) defer_loop(ui_push_font_color(context, value), ui_pop_font_color(context))
UI_PUSH_POP_FUNCS(font_color, V4f)

#define ui_background_color(context, value) defer_loop(ui_push_background_color(context, value), ui_pop_background_color(context))
UI_PUSH_POP_FUNCS(background_color, V4f)

#define ui_border_color(value) defer_loop(ui_push_border_color(value), ui_pop_border_color())
UI_PUSH_POP_FUNCS(border_color, V4f)

#define ui_padding(context, value) defer_loop(ui_push_padding(context, value), ui_pop_padding(context))
UI_PUSH_POP_FUNCS(padding, Screen_Rect_F32)

#define ui_sizing_x(context, value) defer_loop(ui_push_sizing_x(context, value), ui_pop_sizing_x(context))
UI_PUSH_POP_FUNCS(sizing_x, UI_Sizing_Value)

#define ui_align_x(context, value) defer_loop(ui_push_align_x(context, value), ui_pop_align_x(context))
UI_PUSH_POP_FUNCS(align_x, UI_Align)

#define ui_align_y(context, value) defer_loop(ui_push_align_y(context, value), ui_pop_align_y(context))
UI_PUSH_POP_FUNCS(align_y, UI_Align)

#define ui_border_thickness(context, value) defer_loop(ui_push_border_thickness(context, value), ui_pop_border_thickness(context))
UI_PUSH_POP_FUNCS(border_thickness, F32)

#define ui_corner_radius(context, value) defer_loop(ui_push_corner_radius(context, value), ui_pop_corner_radius(context))
UI_PUSH_POP_FUNCS(corner_radius, F32)

#define ui_seed_key(context, value) defer_loop(ui_push_seed_key(context, value), ui_pop_seed_key(context))
UI_PUSH_POP_FUNCS(seed_key, U64)

// ///////////////////////////////////////////////
// NOTE(erb): internal stuff
// ///////////////////////////////////////////////


void ui_build_element_cache(UI_Ctx *ctx, UI_Element *element)
{
	for (UI_Element *child = element->first_child;
       child;
       child = child->next)
	{
		ui_build_element_cache(ctx, child);
	}
	
	U64 hash = ui_hash_key(element->key);
	
  if (hash != 0)
	{
		U64 idx = hash & (array_size(ctx->element_cache) - 1);
		UI_Element **free_slot = ctx->element_cache + idx;
		
		if (*free_slot) 
		{
			element->next_hash = (*free_slot);
		}
    *free_slot = element;
	}
}

file_local
void fit_size_element(UI_Element *element, Axis2 axis)
{
	assert(element);
  
  F32 *element_size = &element->size.c[axis];
  F32 *element_min_size = &element->min_size.c[axis];
	
	if (!(element->flags & UIFlags_DrawText) && element->sizing[axis] != UISizing_Fixed)
	{
		F32 padding = element->padding.c[axis] + element->padding.c[axis+2];
		*element_min_size += padding;
		*element_size += padding;
		
		if (element->layout_axis == axis)
		{
			*element_size += (element->children_count - 1) * element->child_gap;
		}
	}
	
	UI_Element *parent = element->parent;
	if (parent)
	{
		F32 *parent_size = &parent->size.c[axis];
		F32 *parent_min_size = &parent->min_size.c[axis];
		
		if (parent->sizing[axis] != UISizing_Fixed)
		{
			if (parent->layout_axis == axis)
			{
				*parent_size += *element_size;
				*parent_min_size += *element_min_size;
			}
			else if (!(parent->flags & UIFlags_CrossOverlay))
			{
				*parent_size = max(*parent_size, *element_size);
				*parent_min_size = max(*parent_min_size, *parent_min_size);
			}
		}
	}
}

file_local
void fit_size_step(UI_Element *element, Axis2 axis)
{
  for (UI_Element *child = element->first_child;
       child;
       child = child->next)
  {
		fit_size_step(child, axis);
  }
  
  fit_size_element(element, axis);
}

file_local
void wrap_text_element(Arena *arena, UI_Element *element)
{
  supress_unused(arena);
  supress_unused(element);
#if 0
  assert(element);
  assert(arena);
  
  // NOTE(erb): iterate and wrap when needed.
  if ((element->flags & UIFlags_DrawText) && element->sizing[Axis2_X] == UISizing_Shrink)
  {
    assert(element->text);
    UI_Text *text = element->text;
    
    // NOTE(erb): push the first line in
    UI_Line_Node *first_line = push_struct(arena, UI_Line_Node);
    first_line->content = text->content;
    text->first_line = text->last_line = first_line;
    text->line_count++;
    
    U32 last_line_begin = 0;
    U32 longest_line_bytes = 0;
    F32 longest_line_width = element->size.x;
    
    F32 line_width = 0;
    U32 line_bytes = 0;
    
    F32 scale_factor = text->font_size / text->font.height;
    
    // TODO(erb): handle utf8
    // TODO(erb): handle new line wrapping
    for (U32 byte_idx = 0; 
         byte_idx < text->content.length;)
    {
      I32 codepoint_byte_count = 0;
      I32 letter = GetCodepointNext(text->content.value + byte_idx, &codepoint_byte_count);
      byte_idx += codepoint_byte_count;
      line_bytes++;
      
      line_width += scale_factor * get_char_width(text->raylib_font, letter);
      
      if (byte_idx < text->content.length - 1)
      {
        line_width += text->spacing;
      }
      
      if (line_width > element->size.x)
      {
        U32 wrap_word_width = 0;
        U32 wrap_idx = byte_idx;
        for (; 
             wrap_idx > 0;
             wrap_idx--)
        {
          wrap_word_width += scale_factor * get_char_width(text->raylib_font, wrap_idx);
          if (wrap_idx < line_bytes - 1)
          {
            wrap_word_width += text->spacing;
          }
          
          F32 new_width = (line_width - wrap_word_width);
          if (new_width < element->size.x || new_width < element->min_size.x)
          {
            break;
          }
        }
        
        // NOTE(erb): split the current line
        UI_Line_Node *old_line = text->last_line;
        old_line->content = str_slice(text->content, last_line_begin, wrap_idx);
        
        // NOTE(erb): Append new line
        UI_Line_Node *line = push_struct(arena, UI_Line_Node);
        line->content = str_from(text->content, wrap_idx);
        text->last_line = old_line->next = line;
        text->line_count++;
        last_line_begin = wrap_idx;
        
        longest_line_width = max(longest_line_width, line_width - wrap_word_width);
        line_width = 0;
        longest_line_bytes= max(longest_line_bytes, line_bytes - old_line->content.length);
        line_bytes = 0;
        byte_idx = wrap_idx+1;
      }
    }
    
    element->size.x = longest_line_width;
    element->size.y = (text->line_count * text->font_size) + ((text->line_count - 1) * text->line_spacing);
  }
#endif
}

file_local
void wrap_text_step(Arena *arena, UI_Element *element)
{
  
  for (UI_Element *child = element->first_child;
       child;
       child = child->next)
  {
    wrap_text_element(arena, child);
  }
}

file_local
void grow_and_shrink_size(UI_Element *parent, Axis2 axis)
{
  assert(parent);
  
	F32 padding = parent->padding.c[axis] + parent->padding.c[axis+2];
  F32 extra_size = parent->size.c[axis] - padding;
  
	if (parent->layout_axis == axis)
	{
		UI_Element *first_shrinkable = 0;
		UI_Element *first_growable = 0;
		for (UI_Element *child = parent->first_child;
         child;
         child = child->next)
		{
			UI_Sizing sizing = child->sizing[axis];
			if (!first_growable && sizing == UISizing_Grow)
			{
				first_growable = child;
			}
			
			if (!first_shrinkable && (sizing == UISizing_Shrink || sizing == UISizing_Fit))
			{
				first_shrinkable = child;
			}
			
			extra_size -= child->size.c[axis];
		}
		
		extra_size-= (parent->children_count - 1) * parent->child_gap;
		
		if (first_growable)
		{
			while (extra_size > 0.0001)
			{
				F32 smallest = first_growable->size.c[axis];
				F32 second_smallest = MAX_F32;
				F32 size_to_add = extra_size;
				
				U32 growable_count = 0;
				for (UI_Element *child = first_growable;
             child;
             child = child->next)
				{
					F32 size = child->size.c[axis];
					F32 max_size = child->max_size.c[axis];
					
					if (child->sizing[axis] == UISizing_Grow && size < max_size)
					{
						growable_count++;
						if (size < smallest)
						{
							second_smallest = smallest;
							smallest = size;
						}
						if (size > smallest)
						{
							second_smallest= min(second_smallest, size);
							size_to_add = second_smallest - smallest;
						}
					}
				}
				
				if (growable_count == 0) 
				{  
					break;
				}
				
				size_to_add = min(size_to_add, extra_size / growable_count);
        
        if (size_to_add == 0) 
        { 
          break;
        }
        
				U32 grown_count = 0;
				
				for (UI_Element *child = first_growable;
             child;
             child = child->next)
				{
					F32 *size = &child->size.c[axis];
					if (child->sizing[axis] == UISizing_Grow && *size == smallest)
					{
						if (*size < child->max_size.c[axis])
						{
							*size += size_to_add;
							extra_size -= size_to_add;
							grown_count++;
						}
					}
				}	
				
				
				if (grown_count == 0) 
				{
					break;
				}
			}
		}
		
		if (first_shrinkable)
		{
			while (extra_size < -0.0001)
			{
				F32 largets = first_shrinkable->size.c[axis];
				F32 second_largest = 0;
				// NOTE(erb): this value is negative
				F32 size_to_sub = extra_size;
				
				U32 shrinkable_count = 0;
				for (UI_Element *child = first_shrinkable;
             child;
             child = child->next)
				{
					F32 size = child->size.c[axis];
					F32 minsize = child->min_size.c[axis];
					UI_Sizing sizing = child->sizing[axis];
					
					if ((sizing == UISizing_Shrink || sizing == UISizing_Fit) && size > minsize)
					{
						shrinkable_count++;
						if (size > largets)
						{
							second_largest = largets;
							largets = size;
						}
						if (size < largets)
						{
							second_largest = max(second_largest, size);
							size_to_sub = second_largest - largets;
						}
					}
				}
				
				if (shrinkable_count == 0) 
				{  
					break;
				}
				
				size_to_sub = max(size_to_sub, extra_size / shrinkable_count);
				
				U32 shrunk_count = 0;
				for (UI_Element *child = first_shrinkable;
             child;
             child = child->next)
				{
					F32 *size = &child->size.c[axis];
					UI_Sizing sizing = child->sizing[axis];
					
					if ((sizing == UISizing_Shrink || sizing == UISizing_Fit) && *size == largets)
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
			
		}
		
	}
	else
	{
		for (UI_Element *child = parent->first_child;
         child;
         child = child->next)
		{
			UI_Sizing sizing = child->sizing[axis];
			F32 *size = &child->size.c[axis];
			
			if (sizing == UISizing_Shrink) 
			{
				*size = max(extra_size, child->min_size.c[axis]);
			}
			else if (sizing == UISizing_Grow)
			{
				*size = min(extra_size, child->max_size.c[axis]);
			}
		}
	}
}

file_local
void grow_and_shrink_step(UI_Element *parent, Axis2 axis)
{
	if (parent)
	{
		grow_and_shrink_size(parent, axis);
		
		for (UI_Element *child = parent->first_child;
         child;
         child = child->next)
		{
			grow_and_shrink_step(child, axis);
		}
	}
}

Index_U32 line_index_from_cursor_delta(F32 delta, UI_Text *text)
{
  assert(text);
  
  F32 line_width = 0;
  F32 scale_factor = text->font_size / (F32)text->font->height;
  
  Index_U32 index = {0};
  for (;
       index.value < text->content.length;
       index.value++)
  {
    char ch = text->content.value[index.value];
    Glyph_Info info = text->font->ascii_glyph_table[(U32)ch];
    F32 letter_width = info.advance_x*scale_factor;
    if (index.value < text->content.length - 1)
    {
      letter_width += text->spacing;
    }
    
    if (delta <= line_width + letter_width)
    {
      // NOTE(erb): snap to forward or back char
      if (delta >= line_width + letter_width/2)
      {
        index.value++;
      }
      index.exists = true;
      break;
    }
    
    line_width += letter_width;
  }
  
  return index;
}

Screen_Rect_F32 element_rect(UI_Element *element)
{
  Screen_Rect_F32 result = rect_top_left(element->pos, element->size);
  return result;
}

file_local
void draw_element_cmd(Arena *arena, UI_Draw_Cmd_List *draw_cmds, UI_Element *element)
{
  assert(draw_cmds);
  assert(element);
	
	if (element->flags & UIFlags_DrawBackGround)
	{
		UI_Draw_Cmd *cmd = sll_push_allocate(arena, draw_cmds, UI_Draw_Cmd);
		cmd->kind = UIDrawKind_Rect;
		cmd->rect = element_rect(element);
		cmd->color = element->background_color;
		
		if (element->flags & UIFlags_DrawRounded)
		{
			cmd->corner_radius = element->corner_radius;
		}
	}
	
	if (element->flags & UIFlags_DrawBorder)
	{
		UI_Draw_Cmd *cmd = sll_push_allocate(arena, draw_cmds, UI_Draw_Cmd);
		cmd->kind = UIDrawKind_RectLines;
    cmd->rect = element_rect(element);
		cmd->color = element->border_color;
    cmd->border_thickness = element->border_thickness;
		
		if (element->flags & UIFlags_DrawRounded)
		{
			cmd->corner_radius = element->corner_radius;
		}
	}
	
	if (element->flags & UIFlags_DrawText)
	{
		UI_Text *text = element->text;
		assert(text);
		
		if (text->line_count > 0)
		{
      F32 line_height = text->font_size + text->line_spacing;
			U32 line_idx = 0;
			for (UI_Line_Node *line = text->first_line;
           line && line_idx < text->line_count;
           line = line->next, line_idx++)
			{
				UI_Draw_Cmd *cmd = sll_push_allocate(arena, draw_cmds, UI_Draw_Cmd);
				
				cmd->rect.top_left = element->pos;
				cmd->rect.top_left.y += line_idx*line_height;
				
				cmd->kind = UIDrawKind_Text;
				cmd->color = text->color;
				cmd->font = text->font;
				cmd->content = line->content;
				cmd->font_size = text->font_size;
				cmd->spacing = text->spacing;
			}
		}
		else
		{
			UI_Draw_Cmd *cmd = sll_push_allocate(arena, draw_cmds, UI_Draw_Cmd);
			cmd->kind = UIDrawKind_Text;
      cmd->rect = element_rect(element);
			cmd->color = text->color;
			cmd->font = text->font;
			cmd->content = text->content;
			cmd->font_size = text->font_size;
			cmd->spacing = text->spacing;
		}
	}
}

file_local
void draw_step(Arena *arena, UI_Draw_Cmd_List *draw_cmds, UI_Element *element)
{
  assert(element);
  draw_element_cmd(arena, draw_cmds, element);
	for (UI_Element *child = element->first_child;
       child;
       child = child->next)
  {
		draw_step(arena, draw_cmds, child);
  }
}


// ///////////////////////////////////////////////
// NOTE(erb): usage
// ///////////////////////////////////////////////

UI_Element *ui_make_element(UI_Ctx *ctx, UI_Key Key, String str, UI_Flags flags);
void ui_build_element_cache(UI_Ctx *ctx, UI_Element *element);
void ui_push_parent(UI_Ctx *ctx, UI_Element *element);

void ui_init(UI_Ctx *ctx, Arena *arena)
{
	ctx->arena = push_sub_arena(arena, kb(64));
	ctx->last_arena = push_sub_arena(arena, kb(64));
  mem_scratch_init(arena, &ctx->scratch_mem);
  
}

void ui_begin_frame(UI_Ctx *ctx, Font_Face font, Input_State *input, Screen_Rect_F32 root_rect)
{
  ctx->input = input;
  ctx->default_font_face = font;
  
  ui_reset_value_stack(padding);
  ui_reset_value_stack(align_x);
  ui_reset_value_stack(align_y);
	ui_reset_value_stack(font_size);
	ui_reset_value_stack(font_spacing);
	ui_reset_value_stack(line_spacing);
	ui_reset_value_stack(child_gap);
	ui_reset_value_stack(font_color);
	ui_reset_value_stack(background_color);
	ui_reset_value_stack(border_color);
  ui_reset_value_stack(seed_key);
	ui_reset_value_stack(sizing_x);
	ui_reset_value_stack(corner_radius);
	ui_reset_value_stack(border_thickness);
  
  Screen_Rect_F32 padding = {0};
  padding.top = padding.left = padding.right = padding.bottom = 10;
	ui_push_padding(ctx, padding);
  ui_push_align_x(ctx, UIAlign_Start);
  ui_push_align_y(ctx, UIAlign_Start);
	ui_push_font_size(ctx, font.height);
	ui_push_font_spacing(ctx, font.spacing);
	ui_push_line_spacing(ctx, font.line_spacing);
	ui_push_child_gap(ctx, 10);
	ui_push_font_color(ctx, v4ff(1));
	ui_push_background_color(ctx, color_debug);
	ui_push_border_color(ctx, v4f(0, 0, 0, 1));
  ui_push_seed_key(ctx, ((U64)(~0)));
	ui_push_sizing_x(ctx, ui_fit());
	ui_push_corner_radius(ctx, 5.0f);
	ui_push_border_thickness(ctx, 2);
  
  ui_set_next_rect(ctx, root_rect);
	ui_push_parent(ctx, ui_make_element(ctx, ui_key_null(), str_null(), UIFlags_None));
}

void ui_end_frame(UI_Ctx *ctx)
{
	mem_zero((U8 *)ctx->element_cache, array_size(ctx->element_cache)*sizeof(UI_Element *));
	
	ui_build_element_cache(ctx, ctx->root);
	arena_release(&ctx->last_arena);
	swap(ctx->last_arena, ctx->arena, Arena);
  
	ctx->element_count = 0;
	ctx->root = 0;
  ctx->defered_draw.first = 0;
}

F32 get_align_multiplier(UI_Align align)
{
  function_persist F32 align_multipliers[] = {
    [UIAlign_Start] = 0.0f,
    [UIAlign_Center] = 0.5f,
    [UIAlign_End] = 1.0f,
  };
  F32 result = align_multipliers[align];
  return result;
}

file_local
void position_align_step(UI_Element *element)
{
  assert(element);
  
  Axis2 axis = element->layout_axis;
  Axis2 cross = (element->layout_axis + 1) % 2;
  
  F32 axis_align_multiplier = get_align_multiplier(element->align[axis]);
  F32 cross_align_multiplier = get_align_multiplier(element->align[cross]);
  
  V2f starting_pos = v2f_add(element->pos, element->padding.top_left);
	V2f advance_pos = starting_pos;
  
  V2f padding_space = v2f_add(element->padding.top_left, element->padding.bottom_right);
  V2f container_space = v2f_sub(element->size, padding_space);
  
  F32 axis_space_remaining = container_space.c[axis];
  
  for (UI_Element *child = element->first_child;
       child;
       child = child->next)
  {
		if (!(child->flags & UIFlags_Floating))
		{
			child->pos = advance_pos;
      advance_pos.c[axis] += child->size.c[axis];
      if (child->next)
      {
        advance_pos.c[axis] += element->child_gap;
        axis_space_remaining -= element->child_gap;
      }
      axis_space_remaining -= child->size.c[axis];
		}
		position_align_step(child);
  }
  
  advance_pos = starting_pos;
  advance_pos.c[axis] += axis_space_remaining*axis_align_multiplier;
  
  for (UI_Element *child = element->first_child;
       child;
       child = child->next)
  {
		if (!(child->flags & UIFlags_Floating))
		{
			child->pos = advance_pos;
      child->pos.c[cross] += (container_space.c[cross] - child->size.c[cross])*cross_align_multiplier;
      
			advance_pos.c[axis] += child->size.c[axis];
      if (child->next)
      {
        advance_pos.c[axis] += element->child_gap;
      }
		}
		position_align_step(child);
  }
}

UI_Draw_Cmd_List ui_calculate_and_draw(UI_Ctx *ctx, Arena *arena)
{
	assert(ctx->current == ctx->root);
  
  fit_size_step(ctx->root,        Axis2_X);
  grow_and_shrink_step(ctx->root, Axis2_X);
  wrap_text_step(&ctx->arena,     ctx->root);
	fit_size_step(ctx->root,        Axis2_Y);
	grow_and_shrink_step(ctx->root, Axis2_Y);
  position_align_step(ctx->root);
	
	UI_Draw_Cmd_List draw_cmds = {0};
	draw_step(arena, &draw_cmds, ctx->root);
  
  draw_cmds.last->next = ctx->defered_draw.first;
  
	return draw_cmds;
}

UI_Element *ui_get_cached_element(UI_Ctx *ctx, UI_Key key)
{
  UI_Element *result = 0;
  
  U64 index = ui_hash_key(key) & (array_size(ctx->element_cache) - 1);
  UI_Element *test_element = ctx->element_cache[index];
  for (;
       test_element != 0; 
       test_element = test_element->next_hash)
  {
    if (ui_key_equals(test_element->key, key))
    {
      result = test_element;
      break;
    }
  }
  
  return result;
}

#define get_ui_stack_value(context, label) ((context)->label##_stack[context->label##_stack_count-1])

UI_Element *ui_make_element(UI_Ctx *ctx, UI_Key key, String str, UI_Flags flags)
{
	UI_Element *element = push_struct(&ctx->arena, UI_Element);
	
	element->max_size = v2ff(MAX_F32);
	element->key = key;
  element->flags = flags;
  
  element->key.parent = get_ui_stack_value(ctx, seed_key);
  
  element->padding = get_ui_stack_value(ctx, padding);
	element->child_gap = get_ui_stack_value(ctx, child_gap);
  
  element->align[Axis2_X] = get_ui_stack_value(ctx, align_x);
  element->align[Axis2_Y] = get_ui_stack_value(ctx, align_y);
  
  element->sizing[Axis2_X] = get_ui_stack_value(ctx, sizing_x).sizing;
	element->size.c[Axis2_X] = get_ui_stack_value(ctx, sizing_x).value;
	
	if (flags & UIFlags_DrawBorder)
	{
		element->border_thickness = get_ui_stack_value(ctx, border_thickness);
    element->border_color = get_ui_stack_value(ctx, border_color);
	}
  
	if (flags & UIFlags_DrawRounded)
	{
		element->corner_radius = get_ui_stack_value(ctx, corner_radius);
	}
  
	if (element->flags & UIFlags_DrawBackGround)
	{
		element->background_color = get_ui_stack_value(ctx, background_color);
	}
	
	if (element->flags & UIFlags_DrawText)
	{
		UI_Text *text = push_struct(&ctx->arena, UI_Text);
		text->font = ctx->default_font_face.data;
		text->font_size = get_ui_stack_value(ctx, font_size);
		text->spacing = get_ui_stack_value(ctx, font_spacing);
		text->line_spacing = get_ui_stack_value(ctx, line_spacing);
		text->color = get_ui_stack_value(ctx, font_color);
		text->content = str_copy(&ctx->arena, str);
		text->first_line = 0;
		
		V2f text_size = measure_text_size_ignore_lines_and_tabs(text->font, text->content, text->font_size, text->spacing);
    
		V2f min_text_size = text_size;
		V2f max_text_size = text_size;
    
    if (element->sizing[Axis2_X] == UISizing_Shrink) 
    {
      // NOTE(erb): find largest word
      U32 word_count = 0;
      String largest_word = {0};
      for (U64 ch_idx = 0; 
           ch_idx < str.length;
           word_count++) 
      {
        String word = str_next_word(str_from(str, ch_idx));
        if (word.length > largest_word.length)
        {
          largest_word = word;
        }
        ch_idx += word.length;
      }
      //assert(largest_word.length > 0);
      
      V2f largest_word_size = measure_text_size_ignore_lines_and_tabs(text->font, largest_word, text->font_size, text->spacing);
      
      max_text_size.y = word_count*text->font_size + (word_count - 1)*text->line_spacing;
      min_text_size.x = largest_word_size.x;
      element->sizing[Axis2_Y] = UISizing_Grow;
    }
    
		element->text = text;
		element->size = text_size;
		element->min_size = min_text_size;
		element->max_size = max_text_size;
	}
	
	if (ctx->has_next_floating_pos)
	{
		element->flags |= UIFlags_Floating;
		element->pos = ctx->next_floating_pos;
		ctx->has_next_floating_pos = false;
	}
	
	if (ctx->has_next_fixed_size)
	{
		element->sizing[Axis2_X] = UISizing_Fixed;
		element->sizing[Axis2_Y] = UISizing_Fixed;
		element->size = ctx->next_fixed_size;
		ctx->has_next_fixed_size = false;
	}
	
	if (ctx->has_next_layout_axis)
	{
		element->layout_axis = ctx->next_layout_axis;
		ctx->has_next_layout_axis = false;
	}
  
	return element;
}

void ui_push_parent(UI_Ctx *ctx, UI_Element *element)
{
	if (ctx->root == 0)
	{
		ctx->root = element;
		element->pos = ctx->root_pos;
	}
	else
	{
		UI_Element *parent = ctx->current;
		if (parent->last_child)
		{
			parent->last_child = parent->last_child->next = element;
		}
		else
		{
			parent->last_child = parent->first_child = element;
		}
		
		parent->children_count++;
		element->parent = parent;
	}
	
	ctx->current = element;
}

void ui_pop_parent(UI_Ctx *ctx)
{
	assert(ctx->current != 0);
	ctx->element_count++;
	ctx->current = ctx->current->parent;
}

#define ui_parent(context, parent)     defer_loop(ui_push_parent(context, parent), ui_pop_parent(context))
#define ui_null_parent(context, flags) ui_make_element(context, ui_key_null(), str_null(), flags)
#define ui_parent_flagged(context, flags) defer_loop(ui_push_parent(context, ui_null_parent(context, flags)), ui_pop_parent(context))
#define ui_parent_zero(context)  ui_parent_flagged(context, UIFlags_None)

UI_Element_Out ui_element_out(UI_Ctx *ctx, UI_Element *element)
{
	UI_Element_Out result = {0};
	UI_Key key = element->key;
  
	if (ui_key_equals(ctx->active_element, key)) 
	{
    result.dragging = true;
    if (mouse_button_up(ctx->input, MouseCode_Left))
		{
			if (ui_key_equals(ctx->hot_element, key))
			{
				result.clicked = true;
			}
      ctx->active_element = ui_key_null();
      result.dragging = false;
		}
	}
	else if (ui_key_equals(ctx->hot_element, key))
	{
		result.hovered = true;
		if (mouse_button_down(ctx->input, MouseCode_Left)) 
		{
			ctx->active_element = key;
			result.pressed = true;
		}
		ctx->hot_element = ui_key_null();
	}
  else if (mouse_button_pressed(ctx->input, MouseCode_Left))
  {
    result.pressed_outside = true;
  }
  
  UI_Element *cached = ui_get_cached_element(ctx, key);
  if (cached)
  {
    if (element->flags & UIFlags_Clickable)
    {
      result.element = cached;
      
      V2f cursor = ctx->input->cursor_reverse_y.end;;
      
      if (cached->pos.x <= cursor.x && 
          cursor.x <= cached->pos.x + cached->size.x &&
          cached->pos.y <= cursor.y && 
          cursor.y <= cached->pos.y + cached->size.y)
      {
        ctx->hot_element = key;
      }
    }
  }
  
  return result;
}

void ui_text(UI_Ctx *ctx, String str)
{
	ui_parent(ctx, ui_make_element(ctx, ui_key_from_string(str), str, UIFlags_DrawText));
}

UI_Element_Out ui_button(UI_Ctx *ctx, String text)
{
	UI_Key key = ui_key_from_string(text);
	UI_Element *element = ui_make_element(ctx, key, str_null(), 
                                        UIFlags_Clickable|
                                        UIFlags_DrawBackGround|
                                        UIFlags_DrawRounded|
                                        UIFlags_DrawBorder);
  ui_seed_key(ctx, ui_hash_key(key)) ui_parent(ctx, element)
	{
		ui_text(ctx, text);
	}
	
	UI_Element_Out result = ui_element_out(ctx, element);
  
  element->background_color.a = 0.5f;
  if (result.hovered) 
  {
    element->background_color.a = 0.75f;
  }
  if (result.dragging) 
  {
    element->background_color = color_red;
  }
  
	return result;
}


UI_Element_Out ui_toggle(UI_Ctx *ctx, String text)
{
	UI_Key key = ui_key_from_string(text);
  UI_Element *element = ui_make_element(ctx, key, str_null(), 
                                        UIFlags_Clickable|
                                        UIFlags_DrawBackGround|
                                        UIFlags_DrawRounded|
                                        UIFlags_DrawBorder);
  
  ui_seed_key(ctx, ui_hash_key(key)) ui_parent(ctx, element)
  {
    ui_text(ctx, text);
  }
  
	UI_Element_Out out = ui_element_out(ctx, element);
  
  if (out.element)
  {
    element->toggled = out.element->toggled;
  }
  if (out.clicked)
  {
    element->toggled = !element->toggled;
  }
  if (element->toggled)
  {
    element->background_color = v4f(1, 1, 1, 1);
  }
  
	return out;
}

B32 ui_try_get_element_top_left(UI_Ctx *ctx, UI_Element *element, V2f *result)
{
  assert(result != 0);
  
  B32 found = false;
  UI_Element *cached = ui_get_cached_element(ctx, element->key);
  if (cached) 
  {
    *result = cached->pos;
    found = true;
  }
  
  return found;
}

void ui_push_draw_cursor_and_highlight(UI_Ctx *ctx,
                                       UI_Draw_Cmd_List *draw_cmds, 
                                       UI_Text text, 
                                       V2f pos,
                                       Text_Edit_State *text_state)
{
  UI_Draw_Cmd *cursor = sll_push_allocate(&ctx->arena, &ctx->defered_draw, UI_Draw_Cmd);
  UI_Text cursor_sub = text;
  cursor_sub.content = str_clip(cursor_sub.content, text_state->cursor);
  
  V2f cursor_sub_size = measure_text_size_ignore_lines_and_tabs(cursor_sub.font, 
                                                                cursor_sub.content, 
                                                                cursor_sub.font_size, 
                                                                cursor_sub.spacing);
  
  V2f cursor_pos = {0};
  cursor_pos.x = pos.x + cursor_sub_size.x;
  cursor_pos.y = pos.y;
  V2f size = v2f(1.5f, cursor_sub.font_size);
  cursor->rect = rect_top_left(cursor_pos, size);
  
  cursor->kind = UIDrawKind_Rect;
  cursor->color = color_white;
  
  
  cursor->color.a = 1.0f;
  
  if (text_state->mark != text_state->cursor)
  {
    UI_Text mark_sub = text;
    mark_sub.content = str_slice(mark_sub.content, 
                                 min(text_state->mark, text_state->cursor),
                                 max(text_state->mark, text_state->cursor));
    F32 highlight_width = measure_text_size_ignore_lines_and_tabs(mark_sub.font, 
                                                                  mark_sub.content, 
                                                                  mark_sub.font_size, 
                                                                  mark_sub.spacing).x;
    
    
    V2f highlight_pos = v2f(cursor->rect.top_left.x, pos.y);
    if (text_state->mark < text_state->cursor) 
    {
      highlight_pos.x -= highlight_width;
    }
    
    V2f hightlight_size = v2f(highlight_width, mark_sub.font_size);
    
    UI_Draw_Cmd *highlight = sll_push_allocate(&ctx->arena, draw_cmds, UI_Draw_Cmd);
    highlight->kind = UIDrawKind_Rect;
    highlight->rect = rect_top_left(highlight_pos, hightlight_size);
    highlight->color = with_alpha(color_yellow, 0.5f);
  }
}

#define bind(key, modifiers, flags, delta) \
[(key)][(modifiers & ModifierFlags_Control) != 0][((modifiers) & ModifierFlags_Shift) != 0] = {(flags),(delta),0}
Text_Action text_action_from_event_table[][2][2] = 
{
  bind(KeyCode_Left, ModifierFlags_None, TextActionFlags_DeltaPickSelectSide, -1),
  bind(KeyCode_Left, ModifierFlags_Shift, TextActionFlags_KeepMark, -1),
  bind(KeyCode_Left, ModifierFlags_Control, TextActionFlags_WordScan, -1),
  bind(KeyCode_Left, ModifierFlags_Shift|ModifierFlags_Control, TextActionFlags_KeepMark|TextActionFlags_WordScan, -1),
  
  bind(KeyCode_Right, ModifierFlags_None, TextActionFlags_DeltaPickSelectSide, 1),
  bind(KeyCode_Right, ModifierFlags_Shift, TextActionFlags_KeepMark, 1),
  bind(KeyCode_Right, ModifierFlags_Control, TextActionFlags_WordScan, 1),
  bind(KeyCode_Right, ModifierFlags_Shift|ModifierFlags_Control, TextActionFlags_KeepMark|TextActionFlags_WordScan, 1),
  
  bind(KeyCode_Delete, ModifierFlags_None, TextActionFlags_Delete|TextActionFlags_ZeroDeltaWithSelect, 1),
  bind(KeyCode_Delete, ModifierFlags_Control, TextActionFlags_Delete|TextActionFlags_WordScan|TextActionFlags_ZeroDeltaWithSelect, 1),
  
  bind(KeyCode_Backspace, ModifierFlags_None, TextActionFlags_Delete|TextActionFlags_ZeroDeltaWithSelect, -1),
  bind(KeyCode_Backspace, ModifierFlags_Control, TextActionFlags_Delete|TextActionFlags_WordScan|TextActionFlags_ZeroDeltaWithSelect, -1),
  
  bind(KeyCode_C, ModifierFlags_Control, TextActionFlags_Copy,  0),
  bind(KeyCode_X, ModifierFlags_Control, TextActionFlags_Cut,   0),
  bind(KeyCode_V, ModifierFlags_Control, TextActionFlags_Paste, 0),
  
  // emacs style
  bind(KeyCode_F, ModifierFlags_Control, TextActionFlags_DeltaPickSelectSide, 1),
  //bind(KeyCode_F, ModifierFlags_Alt, TextActionFlags_WordScan, 1),
  
  bind(KeyCode_B, ModifierFlags_Control, TextActionFlags_DeltaPickSelectSide, -1),
  //bind(KeyCode_B, ModifierFlags_Alt, TextActionFlags_WordScan, -1),
};
#undef bind

Text_Action text_action_from_event(Input_Event *event) 
{
  Text_Action action = {0};
  
  if (event->kind == InputEventKind_KeyDown)
  {
    if (event->key.code < array_size(text_action_from_event_table))
    {
      B32 ctrl = (event->key.modifiers & ModifierFlags_Control) != 0;
      B32 shift = (event->key.modifiers & ModifierFlags_Shift) != 0;
      action = text_action_from_event_table[event->key.code][ctrl][shift];
    }
  }
  else if (event->kind == InputEventKind_Text)
  {
    action.code_point = event->code_point;
    action.delta = 1;
  }
  
  return action;
}

Range_U64 range_U64_min_max(U64 a, U64 b)
{
  Range_U64 range = {0};
  range.min = min(a, b);
  range.max = max(a, b);
  return range;
}

Text_Cmd text_cmd_from_state_and_action(Window_ID window, Arena *arena, String str, Text_Edit_State *state, Text_Action *action)
{
  Text_Cmd cmd = {0};
  cmd.new_cursor = state->cursor;
  cmd.new_mark = state->mark;
  
  cmd.replace_str = S("");
  cmd.copy_str = S("");
  
  // TODO(erb): navigation 
  
  I64 char_delta = 0;
  {
    if (action->flags & TextActionFlags_WordScan) 
    {
      Index_U64 index = str_index_word_end_delta(str, (U32)state->cursor, (U32)action->delta);
      char_delta = (index.value - state->cursor);
    }
    else 
    {
      char_delta = action->delta;
    }
  }
  
  if (state->cursor != state->mark &&
      action->flags & TextActionFlags_ZeroDeltaWithSelect)
  {
    char_delta = 0;
  }
  
  if (state->cursor != state->mark &&
      action->flags & TextActionFlags_DeltaPickSelectSide)
  {
    char_delta = 0;
    if (action->delta > 0) 
    {
      cmd.new_cursor = max(state->cursor, state->mark);
    }
    else if (action->delta < 0)
    {
      cmd.new_cursor = min(state->cursor, state->mark);
    }
  }
  
  // NOTE(erb): apply delta
  cmd.new_cursor = (state->cursor + char_delta) % str.length; 
  
  // NOTE(erb): post navigation
  
  // NOTE(erb): copy
  if (action->flags & TextActionFlags_Copy || 
      action->flags & TextActionFlags_Cut) 
  {
    cmd.copy_str = str_slice(str, state->cursor, state->mark);
  }
  
  // NOTE(erb): delete
  if (action->flags & TextActionFlags_Delete || 
      action->flags & TextActionFlags_Cut)
  {
    cmd.replace_range = range_U64_min_max(cmd.new_cursor, cmd.new_mark);
    cmd.new_cursor = cmd.new_mark = cmd.replace_range.min;
  }
  
  // NOTE(erb): insert
  if (action->code_point != 0)
  {
    // NOTE(erb): only ascii chars
    cmd.single_char = (char)action->code_point;
    cmd.replace_str.value = &cmd.single_char;
    cmd.replace_str.length = 1;
    cmd.replace_range.min = state->cursor;
    cmd.replace_range.max = state->mark;
  }
  // NOTE(erb): paste
  else if (action->flags & TextActionFlags_Paste)
  {
    // TODO(erb): clipboard text
    cmd.replace_str = g_platform.get_clipboard_str(window, arena);
    cmd.replace_range.min = state->cursor;
    cmd.replace_range.max = state->mark;
  }
  
  // NOTE(erb): keep mark
  if (!(action->flags & TextActionFlags_KeepMark))
  {
    cmd.new_mark = cmd.new_cursor;
  }
  
  return cmd;
}

B32 ui_apply_text_action(UI_Ctx *ctx, Text_Action action, Text_Edit_State *state, Arena *input_arena, String_Builder *input_builder)
{
  B32 result = false;
  
  if (action.delta != 0 || 
      action.code_point != 0||
      action.flags != TextActionFlags_None)
  {
    Scratch scratch = mem_scratch_begin(&ctx->scratch_mem);
#if 0
    Text_Cmd text_cmd = text_cmd_from_state_and_action(ctx->window, scratch.arena, input_builder->buffer, state, &action);
#else
    Text_Cmd text_cmd = {0};
#endif
    
    if (state->cursor != text_cmd.new_cursor &&
        state->mark != text_cmd.new_mark)
    {
      result = true;
    }
    
    state->cursor = text_cmd.new_cursor;
    state->mark = text_cmd.new_mark;
    
    str_build_replace(input_arena, 
                      input_builder, 
                      text_cmd.replace_str, 
                      text_cmd.replace_range);
    
    if (text_cmd.copy_str.length > 0) 
    {
#if 0
      g_platform.set_clipboard_str(ctx->window, text_cmd.copy_str);
#endif
    }
    
    mem_scratch_end(&ctx->scratch_mem, &scratch);
  }
  
  return result;
}

void ui_input(UI_Ctx *ctx, Arena *builder_arena, String_Builder *str_builder, Input_State *input_state)
{
  UI_Key key = ui_key_from_string(str_builder->buffer);
  UI_Element *input = 0; 
  UI_Element *container = 0;
  
  ui_sizing_x(ctx, ui_grow()) 
    ui_parent(ctx, ui_make_element(ctx, key, str_null(), 
                                   UIFlags_DrawBorder|
                                   UIFlags_DrawRounded|
                                   UIFlags_DrawBackGround|
                                   UIFlags_Clickable)) 
    ui_seed_key(ctx, ui_hash_key(key))
  {
    input = ui_make_element(ctx, key, 
                            str_builder->buffer, 
                            UIFlags_DrawText);
    ui_parent(ctx, input);
    container = input->parent;
  }
  
  UI_Element_Out out = ui_element_out(ctx, container);
  
  if (out.element)
  {
    B32 key_action_happened = false;
    for (U32 event_idx = 0;
         event_idx < input_state->event_stream_count;
         event_idx++) 
    {
      Input_Event *event = (input_state->event_stream + event_idx);
      Text_Action action = text_action_from_event(event);
      key_action_happened = (key_action_happened ||
                             ui_apply_text_action(ctx, 
                                                  action, 
                                                  &ctx->text_state, 
                                                  builder_arena, 
                                                  str_builder));
    }
    
    if (!key_action_happened && (out.pressed || out.dragging))
    {
      V2f mouse_pos = input_state->cursor_reverse_y.end;
      V2f input_pos = {0};
      if (ui_try_get_element_top_left(ctx, input, &input_pos))
      {
        Index_U32 index = line_index_from_cursor_delta(mouse_pos.x - input_pos.x, input->text);
        if (out.dragging)
        {
          ctx->text_state.cursor = index.value;
        }
        else if (out.pressed) 
        {
          ctx->text_state.cursor = index.value;
          ctx->text_state.mark = index.value;
        }
      }
    }
    
    
    V2f pos = v2f_add(out.element->pos, out.element->padding.top_left);
    ui_push_draw_cursor_and_highlight(ctx, &ctx->defered_draw, *input->text, pos, &ctx->text_state);
  }
}