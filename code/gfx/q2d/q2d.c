Q2D_Image_Texture 
q2d_create_texture_from_image(IMG_Loaded *image)
{
  Assert(image);
  Assert(image->data.value && image->data.size);
  
  Q2D_Pixel_Format pixel_fmt = 0;
  Q2D_Swizzle swizzle = {0};
  switch(image->channels)
  {
    case 1: 
    {
      pixel_fmt = Q2DPixelFormat_R;
      swizzle.r = Q2DSwizzle_One;
      swizzle.g = Q2DSwizzle_One;
      swizzle.b = Q2DSwizzle_One;
      swizzle.a = Q2DSwizzle_R;
    } break;
    case 2: Implement(); break;
    case 3: pixel_fmt = Q2DPixelFormat_RGB  ; break;
    case 4: pixel_fmt = Q2DPixelFormat_RGBA ; break;
    InvalidDefaultCase;
  }
  Q2D_Texture_Flags flags = (Q2DTextureFlag_GenMipMap|
                             Q2DTextureFlag_MinMagLinear|
                             Q2DTextureFlag_WrapClampToEdge);
  Q2D_Image_Texture result = q2d_create_texture_from_bitmap(image->data.value, 
                                                            image->size, pixel_fmt, 
                                                            Q2DPixelFormat_RGBA, flags, 
                                                            swizzle);
  return(result);
}

Q2D_State *
make_render_data(Arena *arena)
{
  Q2D_State *result = PushStruct(arena, Q2D_State);
  IMG_Loaded blank_image = img_generate_blank_image(arena, 800, 800);
  result->blank_image_texture = q2d_create_texture_from_image(&blank_image);
  // TODO(erb): this should be better done
  result->quad_capacity = 100000;
  result->quads = PushArray(arena, result->quad_capacity, Q2D_Quad);
  return(result);
}

Index_U32 
get_texture_idx(Q2D_Batch *batch, Q2D_Texture texture_id)
{
  Index_U32 result = {0};
  for (EachU32(texture_idx, batch->texture_count))
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

Q2D_Quad*
q2d_append_render_rect(Q2D_State *state, 
                       Range_V2f dest_rect, Range_V2f src_rect, 
                       F32 corner_radius, F32 edge_softness, F32 border_thickness,
                       Quad_Colors colors, Q2D_Texture texture_id)
{
  // NOTE(erb): push rect
  U64 rect_idx = state->quad_count;
  Assert(state->quad_count < state->quad_capacity);
  Q2D_Quad *quad = (state->quads + state->quad_count++);
  
  quad->dest_bottom_left = dest_rect.min;
  quad->dest_top_right   = dest_rect.max;
  quad->src_bottom_left  = src_rect.min;
  quad->src_top_right    = src_rect.max;
  
  quad->color_bottom_left  = colors.bottom_left;
  quad->color_bottom_right = colors.bottom_right;
  quad->color_top_left     = colors.top_left;
  quad->color_top_right    = colors.top_right;
  
  quad->corner_radius      = corner_radius;
  quad->edge_softness      = edge_softness;
  quad->border_thickness   = border_thickness;
  
  if (state->batch_count == 0) 
  {
    state->batch_count = 1;
  }
  Q2D_Batch *batch = (state->batches + state->batch_count - 1);
  if(batch->rect_count == 0)
  {
    batch->start_rect_index = rect_idx;
  }
  
  Index_U32 found_texture_slot = get_texture_idx(batch, texture_id);
  if (found_texture_slot.exists)
  {
    // TODO(erb): change this to be a U32
    quad->texture_slot = (F32)found_texture_slot.value;
  }
  else
  {
    // NOTE(erb): push new batch
    if (batch->texture_count >= ArraySize(batch->textures))
    {
      Assert(state->batch_count < ArraySize(state->batches));
      batch = (state->batches + state->batch_count++);
      ZeroStruct(batch);
      batch->start_rect_index = rect_idx;
    }
    // TODO(erb): change this to be a U32
    quad->texture_slot = (F32)batch->texture_count;
    batch->textures[batch->texture_count++] = texture_id;
  }
  batch->rect_count++;
  return(quad);
}

Range_V2f 
texture_rect_range(Q2D_Image_Texture texture)
{
  Range_V2f result = rng_v2f(v2ff(0),v2fi(texture.size));
  return(result);
}

Quad_Colors 
quad_colors_single(V4f color)
{
  Quad_Colors result = {0};
  result.top_left = color;
  result.top_right = color;
  result.bottom_left = color;
  result.bottom_right = color;
  return(result);
}

void
q2d_render_rect_color(Q2D_State *state, Range_V2f range, V4f color)
{
  q2d_append_render_rect(state, range,
                         texture_rect_range(state->blank_image_texture),
                         0, 0, 0, quad_colors_single(color), 
                         state->blank_image_texture.handle);
}

void
q2d_render_rect_color_ex(Q2D_State *state, Range_V2f range, Quad_Colors colors, F32 corner_radius, F32 edge_softness, F32 border_thickness)
{
  q2d_append_render_rect(state,range,
                         texture_rect_range(state->blank_image_texture),
                         corner_radius, edge_softness, border_thickness, colors,  
                         state->blank_image_texture.handle);
}

void
q2d_render_texture_src_dest(Q2D_State *state, Range_V2f src, Range_V2f dest, Q2D_Texture texture, V4f color)
{
  q2d_append_render_rect(state,
                         dest,
                         src,
                         0, 0, 0, 
                         quad_colors_single(color), 
                         texture);
}

void
q2d_debug_grid(Q2D_State *state, V2f delta, V2f max, V4f color)
{
  for (Axis2 axis = 0; 
       axis<2; 
       ++axis)
  {
    if(delta.c[axis] != 0.0f)
    {
      V2f size = {0};
      size.c[axis] = max.c[axis];
      size.c[(axis+1)%2] = 1;
      
      V2f point = {0};
      for(F32 disp = 0; 
          disp <= max.c[(axis+1)%2]; 
          disp += delta.c[axis])
      {
        point.c[(axis+1)%2] = disp;
        q2d_render_rect_color(state, rng_v2f_point_add(point, size), color);
      }
    }
  }
}


Range_V2f 
rng_v2f_reflect_y(Range_V2f rng, F32 y)
{
  Range_F32 range_y = rng_f32((F32)rng.min.y, (F32)rng.max.y);
  Range_F32 reflected = rng_f32_reflect_at(range_y, y);
  Range_V2f result = rng_v2f(v2f((F32)rng.min.x, reflected.min),
                             v2f((F32)rng.max.x, reflected.max));
  return(result);
}

Q2D_Font
q2d_load_font(Arena *arena, String8 font_file, F32 height_px)
{
  
  // NOTE(erb): default atlas bitmap size
#if 0
  V2i atlas_size = v2ii(1024);
#else
  V2i atlas_size = v2ii(1024*8); // TODO(erb): this is extreme debug code
#endif
  
  Scratch scratch1 = scratch_begin(&arena, 1);
  FNT_Codepoint_Range *codepoint_ranges = 0;
  {
    // NOTE(erb): latin suplement
    FNT_Codepoint_Range *rng = PushStruct(scratch1.arena, FNT_Codepoint_Range);
    rng->value = rng_u32(codepoint_latin_suplement_begin, codepoint_latin_suplement_end);
    SLLStackPush(codepoint_ranges, rng);
  }
  {
    // NOTE(erb): ascii
    FNT_Codepoint_Range *rng = PushStruct(scratch1.arena, FNT_Codepoint_Range);
    rng->value = rng_u32('!', '~');
    SLLStackPush(codepoint_ranges, rng);
  }
  
  U8 *bitmap = 0;
  {
    // TODO(erb): extreme debug code!!!!
    bitmap = os_memory_commit(atlas_size.x*atlas_size.y);
  }
  
  Q2D_Font result = {0};
  result.data = fnt_bake_font(arena, font_file, height_px, codepoint_ranges, bitmap, atlas_size);
  result.height_px = height_px;
  
  Q2D_Swizzle swizzle = {0};
  swizzle.r = Q2DSwizzle_One;
  swizzle.g = Q2DSwizzle_One;
  swizzle.b = Q2DSwizzle_One; 
  swizzle.a = Q2DSwizzle_R;
  result.atlas = q2d_create_texture_from_bitmap(bitmap, atlas_size,
                                                Q2DPixelFormat_R, Q2DPixelFormat_R,
                                                Q2DTextureFlag_MinMagLinear, 
                                                swizzle);
  
  os_memory_release(bitmap);
  scratch_end(&scratch1);
  return(result);
}

F32
q2d_render_glyph_scaled(Q2D_State *render_state, I32 codepoint, V2f baseline, Q2D_Font *font, F32 scale, V4f color, B32 should_draw)
{
  F32 advance_x = 0;
  FNT_Glyph *baked = fnt_get_glyph(&font->data, codepoint);
  
  V4f effective_color = color;
#if DEBUG
  if(!baked)
  {
    baked = fnt_get_glyph(&font->data, '?');
    effective_color = color_debug;
  }
#endif
  
  if(baked)
  {
    advance_x = baked->advance_x*scale;
    if(should_draw)
    {
      V2f bottom_up_offset = v2fi(v2i(baked->offset.x, -baked->offset.y));
      bottom_up_offset = v2f_scale(bottom_up_offset, scale);
      
      V2f glyph_top_left = v2f_add(baseline, bottom_up_offset);
      
      V2f size = v2fi(rng_v2i_size(baked->bitmap_range));
      size = v2f_scale(size, font->data.bitmap_to_pixel_scale);
      size = v2f_scale(size, scale);
      
      Range_V2f dest = rng_v2f_point_add(glyph_top_left, size);
      Range_V2f src = rng_v2fi(baked->bitmap_range);
      
#if 0
      q2d_render_rect_color(render_state, dest, with_alpha(color_yellow, 0.75f));
      
      Range_V2f line = {0};
      line.min.x = dest.min.x + size.x*0.5f - 0.5f;
      line.min.y = dest.min.y;
      line.max.x = dest.min.x + size.x*0.5f + 1.0f;
      line.max.y = dest.min.y + size.y;
      q2d_render_rect_color(render_state, line, color_red);
#endif
      
      q2d_render_texture_src_dest(render_state, src, dest, font->atlas.handle, color);
    }
  }
  return(advance_x);
}

F32
q2d_codepoint_width_or_blank(I32 codepoint, Q2D_Font *font, F32 scale, F32 blank_spacing)
{
  F32 width = 0;
  if(char_is_blank(codepoint))
  {
    width = blank_spacing;
  }
  else
  {
    FNT_Glyph *glyph = fnt_get_glyph(&font->data, codepoint);
    if(glyph)
    {
      width = glyph->advance_x*scale;
    }
  }
  return(width);
}

F32
q2d_render_string_baseline_px(Q2D_State *render_state, String8 str, V2f baseline, Q2D_Font *baked_font, F32 height_px, F32 blank_spacing, F32 char_spacing, V4f color, B32 should_draw)
{
  Assert(render_state);
  Assert(baked_font);
  
  F32 scale = (height_px / baked_font->height_px);
  V2f cursor = baseline;
  
  UTF8_Decode_Iterator it = utf8_it(str);
  for(;utf8_it_next(&it);)
  {
    if(char_is_blank(it.codepoint))
    {
      cursor.x += blank_spacing;
    }
    else
    {
      cursor.x += q2d_render_glyph_scaled(render_state, it.codepoint, cursor, baked_font, scale, color, should_draw);
    }
    
    if(it.cursor < it.base_str.size)
    {
      cursor.x += char_spacing;
    }
  }
  return(cursor.x - baseline.x);
}

F32
q2d_render_string_baseline(Q2D_State *render_state, String8 str, V2f baseline, Q2D_Font *baked_font, F32 blank_spacing, F32 char_spacing, V4f color, B32 should_draw)
{
  F32 height = baked_font->height_px;
  F32 advance_x = q2d_render_string_baseline_px(render_state, str, baseline, baked_font, height,
                                                blank_spacing, char_spacing, color, should_draw);
  return(advance_x);
}

#if GFX_BACKEND == GFX_BACKEND_OPENGL
#include "gl/gl_q2d.c"
#endif