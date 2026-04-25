/* date = April 24th 2026 3:21 pm */

#ifndef BASE_NUMERIC_H
#define BASE_NUMERIC_H


#define color_white   (V4f){ .r=1, .g=1, .b=1, .a=1 }
#define color_black   (V4f){ .r=0, .g=0, .b=0, .a=1 }
#define color_red     (V4f){ .r=1, .g=0, .b=0, .a=1 }
#define color_green   (V4f){ .r=0, .g=1, .b=0, .a=1 }
#define color_yellow  (V4f){ .r=1, .g=1, .b=0, .a=1 }
#define color_cyan    (V4f){ .r=0, .g=1, .b=1, .a=1 }
#define color_blue    (V4f){ .r=0, .g=0, .b=1, .a=1 }
#define color_magenta (V4f){ .r=1, .g=0, .b=1, .a=1 }
#define color_debug   (V4f){ .r=1, .g=0, .b=1, .a=1 }

#define v2_expand(v) v.x, v.y
#define v3_expand(v) v.x, v.y, v.z
#define v4_expand(v) v.x, v.y, v.z, v.w


typedef union V2f
{
	struct { F32 x, y; };
	F32 c[2];
} V2f;

typedef union V2i
{
	struct { I32 x, y; };
  I32 c[2];
} V2i;

typedef union V3f
{
	struct { F32 x, y, z; };
  struct { V2f xy; F32 _pad_z; };
  struct { V2f _pad_x; F32 yz; };
	F32 c[3];
} V3f;

typedef union V4f
{
	struct { F32 x, y, z, w; };
  struct { V2f xy, zw; };
	struct { V3f xyz; F32 _pad_w; };
  
	struct { F32 r, g, b, a; };
  struct { V3f rgb; F32 _pad_a; };
	F32 c[4];
} V4f;

typedef union Screen_Rect_F32
{
	struct { V2f top_left, bottom_right; };
	struct { F32 left, top, right, bottom; };
  F32 c[4];
} Screen_Rect_F32;

typedef union Screen_Rect_I32
{
	struct { V2i top_left, bottom_right; };
	struct { I32 left, top, right, bottom; };
  I32 c[4];
} Screen_Rect_I32;

typedef union Range_V2f
{
	struct { V2f start, end; };
	struct { V2f min, max; };
  V2f c[2];
} Range_V2f;

#endif //BASE_NUMERIC_H
