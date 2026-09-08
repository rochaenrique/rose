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
#define color_blank   (V4f){ .r=0, .g=0, .b=0, .a=0 }

#define V2Expand(v) v.x, v.y
#define V3Expand(v) v.x, v.y, v.z
#define V4Expand(v) v.x, v.y, v.z, v.w

typedef union V2f V2f;
union V2f
{
	struct { F32 x, y; };
	F32 c[2];
};
raddbg_type_view(V2f, rows($, x, y));

typedef union V2i V2i;
union V2i
{
	struct { I32 x, y; };
  I32 c[2];
};
raddbg_type_view(V2i, rows($, x, y));

typedef union V3f V3f;
union V3f
{
	struct { F32 x, y, z; };
  struct { V2f xy; F32 _pad_z; };
	struct { F32 r, g, b; };
	struct { F32 h, s, v; };
  struct { V2f _pad_x; F32 yz; };
	F32 c[3];
};
raddbg_type_view(V3f, rows($, x, y, z));

typedef union V3i V3i;
union V3i
{
	struct { I32 x, y, z; };
  struct { V2i xy; I32 _pad_z; };
	struct { I32 r, g, b; };
	struct { I32 h, s, v; };
  struct { V2i _pad_x; I32 yz; };
	I32 c[3];
};
raddbg_type_view(V3i, rows($, x, y, z));

typedef union V4f V4f;
union V4f
{
	struct { F32 x, y, z, w; };
  struct { V2f xy, zw; };
	struct { V3f xyz; F32 _pad_w; };
  
	struct { F32 r, g, b, a; };
  struct { V3f rgb; F32 _pad_a; };
	F32 c[4];
};
raddbg_type_view(V4f, color($));

typedef union Range_V2f Range_V2f;
union Range_V2f
{
	struct { V2f start, end; };
	struct { V2f min, max; };
  V2f v[2];
  F32 c[4];
};
raddbg_type_view(Range_V2f, rows($, rows(min, x, y), rows(max, x, y)));

typedef union Range_V2i Range_V2i;
union Range_V2i
{
	struct { V2i start, end; };
	struct { V2i min, max; };
  V2i c[2];
};
raddbg_type_view(Range_V2i, rows($, rows(min, x, y), rows(max, x, y)));

typedef union Range_V3i Range_V3i;
union Range_V3i
{
	struct { V3i start, end; };
	struct { V3i min, max; };
  V3i c[3];
};
raddbg_type_view(Range_V3i, rows($, rows(min, x, y), rows(max, x, y)));

typedef union Range_U64  Range_U64 ;
union Range_U64 
{
  struct { U64 start, end; };
  struct { U64 min, max; };
};
raddbg_type_view(Range_U64, rows($, min, max));

typedef union Range_U32  Range_U32;
union Range_U32 
{
  struct { U32 start, end; };
  struct { U32 min, max; };
};
raddbg_type_view(Range_U32, rows($, min, max));

typedef union Range_I32  Range_I32;
union Range_I32 
{
  struct { I32 start, end; };
  struct { I32 min, max; };
};
raddbg_type_view(Range_I32, rows($, min, max));

typedef union Range_F32 Range_F32;
union Range_F32
{
  struct { F32 min, max; };
  struct { F32 start, end; };
};
raddbg_type_view(Range_F32, rows($, min, max));

#endif //BASE_NUMERIC_H
