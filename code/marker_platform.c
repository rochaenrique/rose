#include "marker_platform.h"

High_Freq_Time high_freq_time_from_ns(U64 nanos)
{
  High_Freq_Time result = {0};
  result.ns = nanos;
  result.ms = nanos * 0.0000001;
  result.s  = nanos * 0.0000000001;
  return result;
}

Range_V2f texture_rect_range(Image_Texture texture)
{
  Range_V2f result = range_from_size(texture.size);
  return result;
}

char *debug_key_code_cstr(Key_Code code)
{
  char *result = "unknown";
  if (0 <= code && code < KeyCode_COUNT) 
  {
    result = key_code_cstrings[(I32)code];
  }
  return result;
}

char *debug_mouse_code_cstr(Mouse_Code code)
{
  char *result = "unknown";
  if (0 <= code && code < MouseCode_COUNT) 
  {
    result = mouse_code_cstrings[(I32)code];
  }
  return result;
}

String debug_modifiers_str(Arena *arena, Modifier_Flags flags)
{
  String result = {0};
  
  if (flags == ModifierFlags_None) 
  {
    result = S("None");
  }
  else
  {
    String_Builder builder = {0};
    
    if (flags & ModifierFlags_Shift)
    {
      str_build_str(arena, &builder, S("|Shift"));
    }
    if (flags & ModifierFlags_Control)
    {
      str_build_str(arena, &builder, S("Control"));
    }
    if (flags & ModifierFlags_Alt)
    {
      str_build_str(arena, &builder, S("|Alt"));
    }
    if (flags & ModifierFlags_Super)
    {
      str_build_str(arena, &builder, S("|Super"));
    }
    if (flags & ModifierFlags_Caps)
    {
      str_build_str(arena, &builder, S("|Caps"));
    }
    if (flags & ModifierFlags_NumLock)
    {
      str_build_str(arena, &builder, S("|NumLock"));
    }
    
    result = builder.buffer;
  }
  
  return result;
}

String debug_input_event_str(Arena *arena, Input_Event* event)
{
  assert(event);
  
  static char buffer[1024];
  
  switch (event->kind)
  {
    default:
    case InputEventKind_None:
    {
      snprintf(buffer, sizeof(buffer), "None");
    } break;
    
    case InputEventKind_Text:
    {
      snprintf(buffer, sizeof(buffer), "Text[codepoint=%d]", event->code_point);
    } break;
    
    case InputEventKind_KeyDown:
    case InputEventKind_KeyUp:
    {
      snprintf(buffer, sizeof(buffer), "Key_%s[code=%s, modifiers=%s]", 
               event->kind == InputEventKind_KeyUp ? "Up" : "Down", 
               debug_key_code_cstr(event->key.code),
               str_to_temp256_cstr(debug_modifiers_str(arena, event->key.modifiers)));
    } break;
    
    case InputEventKind_MouseUp:
    case InputEventKind_MouseDown:
    {
      snprintf(buffer, sizeof(buffer), "Mouse_%s[code=%s, modifiers=%s]", 
               event->kind == InputEventKind_MouseUp ? "Up" : "Down", 
               debug_mouse_code_cstr(event->mouse.code),
               str_to_temp256_cstr(debug_modifiers_str(arena, event->mouse.modifiers)));
    } break;
    
    case InputEventKind_CursorMove:
    {
      V2f position = event->cursor_position;
      snprintf(buffer, sizeof(buffer), "MouseMove[position=(%f, %f)]", position.x, position.y);
    } break;
    
    case InputEventKind_MouseWheel:
    {
      V2f delta = event->wheel_delta;
      snprintf(buffer, sizeof(buffer), "MouseWheel[delta=(%f, %f)]", delta.x, delta.y);
    } break;
  }
  
  String str = push_cstr_copy(arena, buffer);
  return str;
}

// NOTE(erb): keys

B32 key_down(Input_State *input, Key_Code code)
{
  B32 result = input->keys[code].ended_down;
  return result;
}

B32 key_up(Input_State *input, Key_Code code)
{
  B32 result = !input->keys[code].ended_down;
  return result;
}

B32 key_touched(Input_State *input, Key_Code code)
{
  B32 result = (input->keys[code].half_transition_count > 0);
  return result;
}

B32 key_pressed(Input_State *input, Key_Code code)
{
  B32 result = key_touched(input, code) && key_down(input, code);
  return result;
}

B32 key_pressed_modded(Input_State *input, Key_Code code, Modifier_Flags mods)
{
  B32 result = key_pressed(input, code) && (input->modifiers & mods);
  return result;
}

B32 key_released(Input_State *input, Key_Code code)
{
  B32 result = key_touched(input, code) && key_up(input, code);
  return result;
}

// NOTE(erb): mouse

B32 mouse_button_down(Input_State *input, Mouse_Code code)
{
  B32 result = input->mouse_buttons[code].ended_down;
  return result;
}

B32 mouse_button_up(Input_State *input, Mouse_Code code)
{
  B32 result = !input->mouse_buttons[code].ended_down;
  return result;
}

B32 mouse_button_touched(Input_State *input, Mouse_Code code)
{
  B32 result = input->mouse_buttons[code].half_transition_count > 0;
  return result;
}

B32 mouse_button_pressed(Input_State *input, Mouse_Code code)
{
  B32 result = mouse_button_touched(input, code) && mouse_button_down(input, code);
  return result;
}

B32 mouse_button_released(Input_State *input, Mouse_Code code)
{
  B32 result = mouse_button_touched(input, code) && mouse_button_up(input, code);
  return result;
}

// NOTE(erb): text measure

V2f measure_text_size_ignore_lines_and_tabs(Font_Data *font_data, String str, F32 font_height, F32 spacing)
{
  F32 pen_x = 0;
  
  F32 scale = font_height/font_data->height;
  
  for (U32 char_idx = 0;
       char_idx < str.length;
       char_idx++)
  {
    char ch = str.value[char_idx];
    if (ch != '\n' && ch != '\t') 
    {
      Glyph_Info info = font_data->ascii_glyph_table[(U32)ch];
      if (char_idx < str.length - 1) 
      {
        pen_x += spacing;
      }
      pen_x += info.advance_x * scale;
    }
  }
  
  V2f result = v2f(pen_x, font_height);
  return result;
}

Index_U32 get_texture_idx(Render_Batch *batch, U32 texture_id)
{
  Index_U32 result = {0};
  
  for (U32 texture_idx = 0;
       texture_idx < batch->texture_count;
       texture_idx++)
  {
    if (texture_id == batch->textures[texture_idx])
    {
      result.value = texture_idx;
      result.exists = true;
      break;
    }
  }
  
  return result;
}

Corner_Colors corner_colors_single(V4f color)
{
  Corner_Colors result = {0};
  result.top_left = color;
  result.top_right = color;
  result.bottom_left = color;
  result.bottom_right = color;
  return result;
}

Corner_Colors corner_colors_top_bottom(V4f color_top, V4f color_bottom)
{
  Corner_Colors result = {0};
  result.top_left = color_top;
  result.top_right = color_top;
  result.bottom_left = color_bottom;
  result.bottom_right = color_bottom;
  return result;
}

Corner_Colors corner_colors_left_right(V4f color_left, V4f color_right)
{
  Corner_Colors result = {0};
  result.top_left = color_left;
  result.top_right = color_right;
  result.bottom_left = color_left;
  result.bottom_right = color_right;
  return result;
}

Render_Rect *append_render_rect(Render_Data *renderer, 
                                Range_V2f dest_rect, Range_V2f src_rect, 
                                F32 corner_radius, F32 edge_softness, F32 border_thickness,
                                Corner_Colors colors, U32 texture_id)
{
  // NOTE(erb): push rect
  U64 rect_idx = renderer->rects.count;
  assert(renderer->rects.count <= renderer->rects.capacity);
  Render_Rect *rect = (renderer->rects.buffer + renderer->rects.count++);
  
  rect->dest_bottom_left = dest_rect.min;
  rect->dest_top_right = dest_rect.max;
  rect->src_bottom_left = src_rect.min;
  rect->src_top_right = src_rect.max;
  
  rect->corner_radius = corner_radius;
  rect->edge_softness = edge_softness;
  rect->border_thickness = border_thickness;
  rect->color_bottom_left = colors.bottom_left;
  rect->color_bottom_right = colors.bottom_right;
  rect->color_top_left = colors.top_left;
  rect->color_top_right = colors.top_right;
  
  Render_Batch *batch = 0;
  if (renderer->batch_count == 0) 
  {
    batch = (renderer->batches + renderer->batch_count++);
  }
  else
  {
    batch = (renderer->batches + renderer->batch_count - 1);
  }
  
  Index_U32 found_texture_slot = get_texture_idx(batch, texture_id);
  
  if (found_texture_slot.exists)
  {
    // TODO(erb): change this to be a U32
    rect->texture_slot = (F32)found_texture_slot.value;
  }
  else
  {
    // NOTE(erb): push new batch
    if (batch->texture_count >= array_size(batch->textures))
    {
      assert(renderer->batch_count < array_size(renderer->batches));
      batch = (renderer->batches + renderer->batch_count++);
      batch->texture_count = 0;
      batch->start_rect_index = rect_idx;
      mem_zero((U8 *)batch->textures, sizeof(batch->textures));
    }
    // TODO(erb): change this to be a U32
    rect->texture_slot = (F32)batch->texture_count;
    batch->textures[batch->texture_count++] = texture_id;
  }
  
  return rect;
}

Range_V2f rect_to_render_range(Render_Data *renderer, Screen_Rect_F32 rect)
{
  Range_V2f result = to_range(rect, (F32)renderer->screen_size.y);
  return result;
}

void render_rect(Render_Data *renderer, Screen_Rect_F32 rect, V4f color)
{
  append_render_rect(renderer, 
                     rect_to_render_range(renderer, rect), 
                     texture_rect_range(renderer->blank_image_texture),
                     0, 0, 0,
                     corner_colors_single(color), 
                     renderer->blank_image_texture.id);
}

void render_rect_blank_texture(Render_Data *renderer, 
                               Screen_Rect_F32 rect,
                               F32 corner_radius, 
                               F32 edge_softness, 
                               F32 border_thickness,
                               Corner_Colors colors)
{
  append_render_rect(renderer, 
                     rect_to_render_range(renderer, rect), 
                     texture_rect_range(renderer->blank_image_texture),
                     corner_radius, 
                     edge_softness, 
                     border_thickness,
                     colors, 
                     renderer->blank_image_texture.id);
}

void render_rect_gradient(Render_Data *renderer, Screen_Rect_F32 rect, Corner_Colors colors)
{
  append_render_rect(renderer, 
                     rect_to_render_range(renderer, rect), 
                     texture_rect_range(renderer->blank_image_texture),
                     0, 0, 0,
                     colors, renderer->blank_image_texture.id);
}

void render_rect_rounded(Render_Data *renderer, Screen_Rect_F32 rect, V4f color, F32 corner_radius)
{
  append_render_rect(renderer, 
                     rect_to_render_range(renderer, rect), 
                     texture_rect_range(renderer->blank_image_texture),
                     corner_radius, 
                     0.5f,
                     0,
                     corner_colors_single(color), 
                     renderer->blank_image_texture.id);
}

void render_rect_gradient_rounded(Render_Data *renderer, Screen_Rect_F32 rect, Corner_Colors colors, F32 corner_radius)
{
  append_render_rect(renderer, 
                     rect_to_render_range(renderer, rect), 
                     texture_rect_range(renderer->blank_image_texture),
                     corner_radius, 
                     0.5f, 
                     0,
                     colors, renderer->blank_image_texture.id);
}

void render_rect_lines(Render_Data *renderer, Screen_Rect_F32 rect, V4f color, F32 border_thickness)
{
  append_render_rect(renderer, 
                     rect_to_render_range(renderer, rect), 
                     range_point_add(v2ff(0), renderer->blank_image_texture.size),
                     0, 0,
                     border_thickness,
                     corner_colors_single(color), 
                     renderer->blank_image_texture.id);
}

void render_rect_lines_rounded(Render_Data *renderer, Screen_Rect_F32 rect, V4f color, F32 corner_radius, F32 border_thickness)
{
  append_render_rect(renderer, 
                     rect_to_render_range(renderer, rect), 
                     range_point_add(v2ff(0), renderer->blank_image_texture.size),
                     corner_radius, 
                     0.5f,
                     border_thickness,
                     corner_colors_single(color), 
                     renderer->blank_image_texture.id);
}

void render_texture(Render_Data *renderer, Screen_Rect_F32 rect, Image_Texture texture)
{
  append_render_rect(renderer, 
                     rect_to_render_range(renderer, rect), 
                     range_point_add(v2ff(0), texture.size),
                     0, 0, 0,
                     corner_colors_single(color_white), texture.id);
}

V2i rect_I32_size(Screen_Rect_I32 rect)
{
  V2i result = v2i(rect.left - rect.right, rect.bottom - rect.top);
  return result;
}

V2f render_text(Render_Data *renderer, Font_Data *font_data, 
                String str, 
                V2f top_left, 
                F32 font_height, F32 spacing, 
                V4f color)
{
  F32 scale = font_height/font_data->height;
  V2f pen = {0};
  pen.x = top_left.x;
  pen.y = top_left.y + font_height - font_data->below_base_line_offset*scale;
  
  for (U32 char_idx = 0;
       char_idx < str.length;
       char_idx++)
  {
    char ch = str.value[char_idx];
    Glyph_Info info = font_data->ascii_glyph_table[(U32)ch];
    
    V2f pos = v2f(pen.x + info.bearing.x * scale,
                  pen.y - info.bearing.y * scale);
    V2f size = v2f_scale(v2fi(info.size), scale);
    
    // NOTE(erb): push rect
    {
      Screen_Rect_F32 dest_rect = rect_top_left(pos, size);
      
      Range_V2f src_range = {0};
      src_range.min = v2f(0, (F32)info.size.y);
      src_range.max = v2f((F32)info.size.x, 0);
      
      Range_V2f dest_range = rect_to_render_range(renderer, dest_rect);
      
      append_render_rect(renderer, 
                         dest_range, 
                         src_range, 
                         0, 0, 0, 
                         corner_colors_single(color), 
                         info.texture_id);
    }
    
    if (char_idx < str.length - 1) 
    {
      pen.x += spacing;
    }
    
    pen.x += info.advance_x * scale;
  }
  
  V2f text_size = v2f(pen.x - top_left.x, font_height);
  return text_size;
}

V2f render_text_face(Render_Data *renderer, Font_Face font_face, 
                     String str, 
                     V2f top_left, 
                     V4f color)
{
  V2f result = render_text(renderer, 
                           font_face.data, 
                           str, 
                           top_left, 
                           font_face.height, 
                           font_face.spacing, 
                           color);
  return result;
}
