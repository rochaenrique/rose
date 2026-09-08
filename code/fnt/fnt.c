/////////////////////////////////////////////////////////////////
// NOTE(erb): atlas cursor helpers

F32
fnt_get_vadvance_px(FNT_Baked_Font *font)
{
  F32 result = (font->ascent_em+font->descent_em)*font->scale_em_to_pixels;
  return(result);
}

F32
fnt_get_height_px(FNT_Baked_Font *font)
{
  F32 result = (font->ascent_em+font->descent_em+font->line_gap_em)*font->scale_em_to_pixels;
  return(result);
}

FNT_Glyph *
fnt_get_glyph(FNT_Baked_Font *font, I32 codepoint)
{
  // TODO(erb): binary search here
  FNT_Glyph *glyph = 0;
  for(U32Each(glyph_idx, font->glyph_count))
  {
    FNT_Glyph *test = (font->glyphs + glyph_idx);
    if(test->codepoint == codepoint)
    {
      glyph = test;
      break;
    }
  }
  return(glyph);
}

Atlas_Cursor
atlas_cursor_make(V2i atlas_size)
{
  Atlas_Cursor result = {0};
  result.point = v2ii(1);
  result.bottom_y = 1;
  result.atlas_size = atlas_size;
  return(result);
}

B32
atlas_cursor_wrap_fit(Atlas_Cursor *cursor, V2i size)
{
  // NOTE(erb): advance to next row
  if (cursor->point.x+size.x+1 >= cursor->atlas_size.x)
  {
    cursor->point = v2i(1, cursor->bottom_y);
  }
  // NOTE(erb): check if it fits
  B32 result = (cursor->point.y+size.y+1 < cursor->atlas_size.y); 
  return(result);
}

U32
atlas_cursor_get_row_idx(Atlas_Cursor *cursor, I32 row)
{
  U32 idx = ((row + cursor->point.y)*cursor->atlas_size.x + cursor->point.x);
  return(idx);
}

B32
atlas_cursor_advance(Atlas_Cursor *cursor, V2i size)
{
  cursor->bottom_y = Max(cursor->point.y+size.y+1, cursor->bottom_y);
  cursor->point.x += size.x+1;
  
  B32 result = (cursor->point.x+1 < cursor->atlas_size.x);
  return(result);
}


#if FNT_BACKEND == FNT_BACKEND_STBTRUETYPE
#include "stb_truetype/stb_truetype_fnt.c"
#elif FNT_BACKEND == FNT_BACKEND_FREETYPE
#include "freetype/freetype_fnt.c"
#else 
#error "Only stb_truetype and freetype are supported backends"
#endif