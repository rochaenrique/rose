#include <math.h>

// TODO(erb): define memory with heap allocator
#define STBTT_assert(x) Assert(x)

#define STB_TRUETYPE_IMPLEMENTATION
#include "external/stb_truetype.h"

B32
fnt_init()
{
  // NOTE(erb): stub
  return(true);
}

FNT_Baked_Font
fnt_bake_font(Arena *arena, String8 font_file, F32 pixel_height, 
              FNT_Codepoint_Range *codepoint_ranges, U8 *atlas, V2i atlas_size)
{
  // NOTE(erb): init font
  I32 font_index = stbtt_GetFontOffsetForIndex(font_file.value, 0);
  Assert(font_index != -1);
  
  stbtt_fontinfo font_info;
  I32 initted_font = stbtt_InitFont(&font_info, font_file.value, font_index);
  Assert(initted_font != 0);
  
  F32 scale = stbtt_ScaleForPixelHeight(&font_info, pixel_height);
  
  U32 codepoint_count = 0;
  
  // NOTE(erb): calculate codepoints in font
  for(EachNode(FNT_Codepoint_Range, range, codepoint_ranges))
  {
    for(EachInRangeU32(codepoint, range->value))
    {
      I32 glyph_idx = stbtt_FindGlyphIndex(&font_info, codepoint);
      if(glyph_idx != 0)
      {
        codepoint_count++;
      }
    }
  }
  
  FNT_Glyph *baked_chars = PushArray(arena, codepoint_count, FNT_Glyph);
  
  // NOTE(erb): get metrics
  U32 glyph_count = 0;
  Atlas_Cursor cursor = atlas_cursor_make(atlas_size);
  for(EachNode(FNT_Codepoint_Range, range, codepoint_ranges))
  {
    for(EachInRangeU32(codepoint, range->value))
    {
      I32 glyph_idx = stbtt_FindGlyphIndex(&font_info, codepoint);
      if(glyph_idx != 0)
      {
        I32 advance = 0;
        I32 left_bearing = 0;
        stbtt_GetGlyphHMetrics(&font_info, glyph_idx, &advance, &left_bearing);
        SupressUnused(left_bearing);
        
        V2i p0 = {0};
        V2i p1 = {0};
        stbtt_GetGlyphBitmapBox(&font_info, glyph_idx, scale, scale, &p0.x, &p0.y, &p1.x, &p1.y);
        V2i size = v2i_sub(p1, p0);
        B32 fits = atlas_cursor_wrap_fit(&cursor, size);
        Assert(fits);
        U32 row_idx = atlas_cursor_get_row_idx(&cursor, 0);
        U8 *output = atlas + row_idx;
        stbtt_MakeGlyphBitmap(&font_info, output, size.x, size.y, atlas_size.x, scale, scale, glyph_idx);
        
        {
          FNT_Glyph *glyph = (baked_chars + glyph_count++);
          glyph->bitmap_range = rng_v2i_point_add(cursor.point, size);
          glyph->offset = v2i(p0.x, -p0.y); // NOTE(erb): truetype returns negative offset
          glyph->advance_x = scale*advance;
          glyph->codepoint = codepoint;
        }
        
        B32 advanced = atlas_cursor_advance(&cursor, size);
        SupressUnused(advanced);
      }
      else
      {
        os_logf("Invalid Codepoint %#x", codepoint);
        Assert(false);
      }
      //Assert(advanced);
    }
  }
  
  Assert(glyph_count == codepoint_count);
  
  I32 ascent, descent, line_gap;
  stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap);
  float scale_em = stbtt_ScaleForMappingEmToPixels(&font_info, pixel_height);
  
  FNT_Baked_Font result = {0};
  result.ascent_em = ascent;
  result.descent_em = Abs(descent);
  result.line_gap_em = line_gap;
  result.scale_em_to_pixels = scale_em;
  
  result.glyph_count = glyph_count;
  result.glyphs = baked_chars;
  result.bitmap_to_pixel_scale = 1.0f;
  return(result);
}

