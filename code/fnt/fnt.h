#ifndef FNT_H
#define FNT_H

// NOTE(erb): backends
#define FNT_BACKEND_STBTRUETYPE 0x1
#define FNT_BACKEND_FREETYPE    0x2
#define FNT_BACKEND_DEFAULT FNT_BACKEND_STBTRUETYPE

//- fallback to stb true type
#ifndef FNT_BACKEND
#define FNT_BACKEND FNT_BACKEND_STBTRUETYPE
#endif

#if FNT_BACKEND == FNT_BACKEND_STBTRUETYPE
#define FNT_BACKEND_STRING "stb_truetype"
#elif FNT_BACKEND == FNT_BACKEND_FREETYPE
#define FNT_BACKEND_STRING "freetype"
#endif

typedef struct FNT_Glyph FNT_Glyph;
struct FNT_Glyph
{
  Range_V2i bitmap_range;
  V2i offset;
  F32 advance_x;
  I32 codepoint;
};

typedef struct FNT_Baked_Font FNT_Baked_Font;
struct FNT_Baked_Font
{
  F32 bitmap_to_pixel_scale;
  
  U32 ascent_em; // baseline-to-top of the largest glyph in the font (positive) in EM
  U32 descent_em; // baseline-to-bottom of the largest glyph in the font in EM (positive)
  U32 line_gap_em; // baseline-to-baseline in EM
  
  F32 scale_em_to_pixels; // NOTE(erb): use this to compute values in pixels
  
  // TODO(erb): turn this into a hashmap?
  FNT_Glyph *glyphs;
  U32 glyph_count;
};

typedef struct Atlas_Cursor Atlas_Cursor;
struct Atlas_Cursor
{
  V2i point;
  I32 bottom_y;
  V2i atlas_size;
};

typedef struct FNT_Codepoint_Range FNT_Codepoint_Range;
struct FNT_Codepoint_Range
{
  Range_U32 value;
  FNT_Codepoint_Range *next;
};

// NOTE(erb): functions
B32            fnt_init();
FNT_Baked_Font fnt_bake_font(Arena *arena, String8 font_file, F32 pixel_height, 
                             FNT_Codepoint_Range *codepoint_ranges, U8 *atlas, V2i atlas_size);

#endif //FNT_H
