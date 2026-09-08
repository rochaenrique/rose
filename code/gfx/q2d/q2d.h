#ifndef Q2D_H
#define Q2D_H

typedef U64 Q2D_Texture;

typedef struct Q2D_Quad Q2D_Quad;
struct Q2D_Quad
{
  V2f dest_bottom_left;
  V2f dest_top_right;
  V2f src_bottom_left;
  V2f src_top_right;
  
  V4f color_bottom_left;
  V4f color_bottom_right;
  V4f color_top_left;
  V4f color_top_right;
  
  F32 corner_radius;
  F32 edge_softness;
  F32 border_thickness;
  F32 texture_slot;
};

typedef struct Q2D_Batch Q2D_Batch;
struct Q2D_Batch
{
  Q2D_Texture textures[8];
  U32 texture_count;
  U64 start_rect_index;
  U64 rect_count;
  
  Range_V2i clip_range;
  B32 should_clip;
};

typedef struct Q2D_Image_Texture Q2D_Image_Texture;
struct Q2D_Image_Texture
{
  Q2D_Texture handle;
  V2i size;
};

typedef struct Q2D_State Q2D_State;
struct Q2D_State
{
  U32 quad_count;
  U32 quad_capacity;
  Q2D_Quad *quads;
  
  U32 batch_count;
  Q2D_Batch batches[512];
  
  Q2D_Image_Texture blank_image_texture;
};
raddbg_type_view(Q2D_State, rows($, $.blank_image_texture, $.quad_capacity, $.quad_count, $.batch_count, array($.quads, $.quad_count), array($.batches, $.batch_count)));

typedef struct Quad_Colors Quad_Colors;
struct Quad_Colors
{
  V4f bottom_left;
  V4f bottom_right;
  V4f top_left;
  V4f top_right;
};

// NOTE(erb): avoid including "fnt/fnt.h" here
typedef struct Q2D_Font Q2D_Font;
struct Q2D_Font
{
  Q2D_Image_Texture atlas;
  FNT_Baked_Font data;
  F32 height_px;
};

typedef enum Q2D_Texture_Flags
{
  Q2DTextureFlag_GenMipMap       = (1 << 0),
  Q2DTextureFlag_MinMagLinear    = (1 << 1),
  Q2DTextureFlag_WrapClampToEdge = (1 << 2),
} Q2D_Texture_Flags;

typedef enum Q2D_Pixel_Format
{
  Q2DPixelFormat_RGBA,
  Q2DPixelFormat_RGB,
  Q2DPixelFormat_RG,
  Q2DPixelFormat_R,
} Q2D_Pixel_Format;

typedef enum Q2D_Swizzle_Value
{
  Q2DSwizzle_None = 0,
  Q2DSwizzle_R,
  Q2DSwizzle_G,
  Q2DSwizzle_B,
  Q2DSwizzle_A,
  Q2DSwizzle_One,
  Q2DSwizzle_Zero,
} Q2D_Swizzle_Value;

typedef union Q2D_Swizzle Q2D_Swizzle;
union Q2D_Swizzle
{
  Q2D_Swizzle_Value v[4];
  struct
  {
    Q2D_Swizzle_Value r;
    Q2D_Swizzle_Value g;
    Q2D_Swizzle_Value b;
    Q2D_Swizzle_Value a;
  };
};

//- functions
B32               q2d_init(String8 shader_directory);
void              q2d_begin(GFX_Window window);
void              q2d_end(GFX_Window window, Q2D_State *state);
void              q2d_render(Q2D_State *renderer);
void              q2d_clear(Q2D_State *renderer);
void              q2d_scissor_begin(Q2D_State *renderer, Range_V2i range);
void              q2d_scissor_end(Q2D_State *state);
Q2D_Image_Texture q2d_create_texture_from_bitmap(U8 *bitmap_memory, V2i bitmap_size, Q2D_Pixel_Format pixel_fmt, 
                                                 Q2D_Pixel_Format target_fmt, Q2D_Texture_Flags flags, Q2D_Swizzle swizzle);
String8_Array     q2d_get_shader_file_names(Arena *arena);
void              q2d_update_shaders(String8 shader_directory);

void q2d_render_rect_color(Q2D_State *state, Range_V2f range, V4f color);
void q2d_render_rect_color_ex(Q2D_State *state, Range_V2f range, Quad_Colors colors, 
                              F32 corner_radius, F32 edge_softness, F32 border_thickness);
void q2d_render_texture_src_dest(Q2D_State *state, Range_V2f src, Range_V2f dest, Q2D_Texture texture, V4f color);
F32  q2d_render_string_baseline(Q2D_State *render_state, String8 str, V2f baseline, Q2D_Font *baked_font,
                                F32 word_spacing, F32 char_spacing, V4f color, B32 should_draw);

#if GFX_BACKEND == GFX_BACKEND_OPENGL
#include "gl/gl_q2d.h"
#else
#error "Only gl backend for q2d is supported"
#endif

#endif //Q2D_H
