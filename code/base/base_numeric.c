#include <math.h>

// TODO(erb): own implementation

// NOTE(erb): f32
F32
floor_f32(F32 v)
{
  F32 result = floorf(v);
  return(result);
}

F32
round_f32(F32 v)
{
  F32 result = roundf(v);
  return(result);
}

F32
floor_up_f32(F32 v)
{
  F32 result = floor_f32(v+0.5f);
  return(result);
}

I32
floor_f32_to_i32(F32 v)
{
  I32 result = (I32)floor_f32(v);
  return(result);
}

I32
floor_up_f32_to_i32(F32 v)
{
  I32 result = floor_f32_to_i32(v+0.5f);
  return(result);
}

U32
round_f32_to_u32(F32 value)
{
  U32 result = (U32)round_f32(value);
  return(result);
}

I32
round_f32_to_i32(F32 value)
{
  I32 result = (I32)round_f32(value);
  return(result);
}

F32
mod_f32(F32 value, F32 divisor)
{
  F32 result = fmodf(value, divisor);
  return(result);
}

// NOTE(erb): f64
F64
round_f64(F64 value)
{
  F64 result = round(value);
  return(result);
}

F64
floor_f64(F64 value)
{
  F64 result = floor(value);
  return(result);
}

U64
floor_f64_to_u64(F64 value)
{
  U64 result = (U64)floor_f64(value);
  return(result);
}

I64
floor_f64_to_i64(F64 value)
{
  I64 result = (I64)floor_f64(value);
  return(result);
}

U64
round_f64_to_u64(F64 value)
{
  U64 result = (U64)round_f64(value);
  return(result);
}

F64
power_f64(F64 base, F64 exponent)
{
  F64 result = pow(base, exponent);
  return(result);
}

// NOTE(erb): ceil
F32
ceil_f32(F32 v)
{
  // TODO(erb): own implementation
  F32 result = ceilf(v);
  return(result);
}

I32
ceil_f32_to_i32(F32 v)
{
  I32 result = (I32)ceil_f32(v);
  return(result);
}

U32 
safe_truncate_U64_to_U32(U64 value)
{
  Assert(value <= u32_max);
  U32 result = (U32)value;
  return result;
}

// NOTE(erb): misc
F32
lerp(F32 min, F32 max, F32 t)
{
  F32 result = min + (max - min)*t;
  return result;
}

typedef struct F64_Parts F64_Parts;
struct F64_Parts
{
  F64 fraction;
  I64 integer;
  B32 was_truncated;
};

F64_Parts
split_parts_f64(F64 value)
{
  F64 integr = 0;
  F64 frac = modf(value, &integr);
  
  F64_Parts result = {0};
  result.integer = floor_f64_to_i64(integr);
  result.fraction = (F64)frac;
  result.was_truncated = ((F64)result.integer+1.0f < value);
  return(result);
}

F32
copy_sign_f32(F32 value, F32 sign)
{
  F32 result = copysignf(value, sign);
  return(result);
}

/////////////////////////////////////////////////////
// NOTE(erb): v2f

V2f
v2f(F32 x, F32 y)
{
	V2f result = {0};
	result.x = x;
	result.y = y;
	return result;
}

V2f
v2ff(F32 v)
{
	V2f result = {0};
	result.x = result.y = v;
	return result;
}

V2f
v2fi(V2i v)
{
  V2f result = v2f((F32)v.x, (F32)v.y);
  return(result);
}

V2f
v2f_add(V2f a, V2f b)
{
  V2f result = v2f(a.x+b.x, a.y+b.y);
	return(result);
}

V2f
v2f_sub(V2f a, V2f b)
{
	V2f result = v2f(a.x-b.x, a.y-b.y);
	return(result);
}

V2f
v2f_hada(V2f a, V2f b)
{
  V2f result = v2f(a.x*b.x, a.y*b.y);
	return(result);
}

V2f
v2f_hada_div(V2f a, V2f b)
{
	V2f result = v2f(a.x/b.x, a.y/b.y);
	return(result);
}

V2f
v2f_hada_inv(V2f v)
{
  V2f result = v2f(1.0f/v.x, 1.0f/v.y);
  return(result);
}

V2f
v2f_scale(V2f v, F32 k) 
{
	V2f result = v2f(v.x*k, v.y*k);
	return(result);
}

V2f 
v2f_abs(V2f v) 
{
  V2f result = v2f(Abs(v.x), Abs(v.y));
  return(result);
}

V2f 
v2f_lerp(V2f min, V2f max, F32 t) 
{
  V2f result = v2f(lerp(min.x, max.x, t),
                   lerp(min.y, max.y, t));
  return(result);
}

V2f 
v2f_min(V2f a, V2f b)
{
  V2f result = v2f(Min(a.x, b.x), Min(a.y, b.y));
  return(result);
}

V2f 
v2f_max(V2f a, V2f b)
{
  V2f result = v2f(Max(a.x, b.x), Max(a.y, b.y));
  return(result);
}

B32
v2f_greater_equal(V2f a, V2f test)
{
  B32 result = (a.x >= test.x && a.y >= test.y);
  return(result);
}

V2f 
v2f_reverse_y(V2f v, F32 y)
{
  V2f result = {0};
  result.x = v.x;
  result.y = y - v.y;
  return(result);
}

V2i v2i(I32 x, I32 y);

V2i
v2f_floor_to_v2i(V2f v)
{
  V2i result = v2i(floor_f32_to_i32(v.x), 
                   floor_f32_to_i32(v.y));
  return(result);
}

/////////////////////////////////////////////////////
// NOTE(erb): v2i

V2i 
v2i(I32 x, I32 y)
{
	V2i result = {0};
	result.x = x;
	result.y = y;
	return result;
}

V2i v2ii(I32 v)
{
	V2i result = {0};
	result.x = result.y = v;
	return result;
}

V2i 
v2i_add(V2i a, V2i b)
{
	V2i result = v2i(a.x+b.x, a.y+b.y);
	return(result);
}

V2i 
v2i_sub(V2i a, V2i b)
{
  V2i result = v2i(a.x-b.x, a.y-b.y);
	return(result);
}

B32 
v2i_equals(V2i a, V2i b)
{
  B32 result = (a.x == b.x && a.y == b.y);
  return result;
}

B32
v2i_greater_equal(V2i a, V2i test)
{
  B32 result = (a.x >= test.x && a.y >= test.y);
  return(result);
}

B32
v3i_greater_equal(V3i a, V3i test)
{
  B32 result = (a.x >= test.x && 
                a.y >= test.y &&
                a.z >= test.z);
  return(result);
}

/////////////////////////////////////////////////////
// NOTE(erb): v3f

V3f
v3f(F32 x, F32 y, F32 z)
{
	V3f result = {0};
  result.x = x;
  result.y = y;
  result.z = z;
	return(result);
}

V3f
v3ff(F32 v)
{
	V3f result = {0};
	result.x = result.y = result.z = v;
	return(result);
}

V3f
v3f_scale(V3f v, F32 k) 
{
	V3f result = v3f(v.x*k, v.y*k, v.z*k);
	return(result);
}

V3i v3i(I32 x, I32 y, I32 z);

V3i 
v3f_floor_to_v3i(V3f v)
{
  V3i result = v3i(floor_f32_to_i32(v.x),
                   floor_f32_to_i32(v.y),
                   floor_f32_to_i32(v.z));
  return(result);
}

V3f
v3fi(V3i v)
{
	V3f result = v3f((F32)v.x, (F32)v.y, (F32)v.z);
	return(result);
}

/////////////////////////////////////////////////////
// NOTE(erb): v3i

V3i
v3i(I32 x, I32 y, I32 z)
{
	V3i result = {0};
  result.x = x;
  result.y = y;
  result.z = z;
	return(result);
}

V3i
v3ii(I32 v)
{
	V3i result = v3i(v, v, v);
	return(result);
}


/////////////////////////////////////////////////////
// NOTE(erb): v4

V4f
v4f(F32 x, F32 y, F32 z, F32 w)
{
	V4f result = {0};
  result.x = x;
  result.y = y;
  result.z = z;
  result.w = w;
	return(result);
}

V4f
v4ff(F32 v)
{
	V4f result = v4f(v, v, v, v);
	return(result);
}

V4f
v4f_xyz(V3f xyz, F32 a)
{
  V4f result = {0};
  result.xyz = xyz;
  result.a = a;
  return(result);
}

V4f 
with_alpha(V4f c, F32 a)
{
  V4f result = v4f(c.r, c.g, c.b, a);
  return(result);
}

V4f 
v4f_lerp(V4f min, V4f max, F32 t)
{
  V4f result = v4f(lerp(min.x, max.x, t),
                   lerp(min.y, max.y, t),
                   lerp(min.z, max.z, t),
                   lerp(min.w, max.w, t));
  return(result);
}


/////////////////////////////////////////////////////
// NOTE(erb): ranges

Range_F32
rng_f32(F32 min, F32 max)
{
  Range_F32 result = {0};
  result.min = min;
  result.max = max;
  return(result);
}

Range_F32
rng_f32_min_max(F32 a, F32 b)
{
  Range_F32 result = rng_f32(Min(a, b), Max(a, b));
  return(result);
}

Range_U64
rng_u64(U64 min, U64 max)
{
  Assert(max >= min);
  Range_U64 result = {0};
  result.min = min;
  result.max = max;
  return(result);
}

Range_U32
rng_u32(U32 min, U32 max)
{
  Assert(max >= min);
  Range_U32 result = {0};
  result.min = min;
  result.max = max;
  return(result);
}

U32
rng_u32_size(Range_U32 rng)
{
  U32 result = (rng.end - rng.start);
  return(result);
}


Range_I32
rng_i32(I32 min, I32 max)
{
  Assert(max >= min);
  Range_I32 result = {0};
  result.min = min;
  result.max = max;
  return(result);
}


Range_U64
rng_u64_min_max(U64 a, U64 b)
{
  Range_U64 result = rng_u64(Min(a, b), Max(a, b));
  return(result);
}

Range_V2f
rng_v2f(V2f min, V2f max)
{
  Assert(v2f_greater_equal(max, min));
  Range_V2f result = {0};
  result.min = min;
  result.max = max;
  return(result);
}

Range_V2f
rng_v2fi(Range_V2i range)
{
  Range_V2f result = {0};
  result.min = v2fi(range.min);
  result.max = v2fi(range.max);
  return(result);
}

Range_V2i
rng_v2i(V2i min, V2i max)
{
  Assert(v2i_greater_equal(max, min));
  Range_V2i result = {0};
  result.min = min;
  result.max = max;
  return(result);
}

Range_V3i
rng_v3i(V3i min, V3i max)
{
  Assert(v3i_greater_equal(max, min));
  Range_V3i result = {0};
  result.min = min;
  result.max = max;
  return(result);
}

Range_V2i
rng_v2i_center_sub(Range_V2i range, V2i v)
{
  Range_V2i result = rng_v2i(v2i_add(range.min, v),
                             v2i_sub(range.max, v));
  return(result);
}

Range_V2i
rng_v2i_center_add(Range_V2i range, V2i v)
{
  Range_V2i result = rng_v2i(v2i_add(range.min, v),
                             v2i_sub(range.max, v));
  return(result);
}

Range_F32 
rng_f32_point_add(F32 point, F32 add)
{
  Range_F32 result = rng_f32(point, point+add);
  return(result);
}

Range_V2f
rng_f32_combine_xy(Range_F32 x, Range_F32 y)
{
  Range_V2f result = {0};
  result.min.x = x.min;
  result.min.y = y.min;
  result.max.x = x.max;
  result.max.y = y.max;
  return(result);
}

V2f 
rng_v2f_size(Range_V2f range)
{
  V2f result = v2f_sub(range.max, range.min);
  return(result);
}

Range_V2f 
rng_v2f_point_add(V2f point, V2f add)
{
  Range_V2f result = rng_v2f(point, v2f_add(point, add));
  return(result);
}

Range_V2f 
rng_v2f_center_size(V2f center, V2f size)
{
  V2f point = v2f_sub(center, v2f_scale(size, 0.5f));
  Range_V2f result = rng_v2f_point_add(point, size);
  return(result);
}

Range_F32
rng_v2f_axis(Range_V2f range, Axis2 axis)
{
  Assert(axis >= 0 && axis < ArraySize(range.c));
  Assert((axis+2) >= 0 && (axis+2) < ArraySize(range.c));
  Range_F32 result = rng_f32(range.c[axis], range.c[axis+2]);
  return(result);
}

B32
rng_f32_contains_exclusive(Range_F32 range, F32 value)
{
  B32 result = (value < range.max && value > range.min);
  return(result);
}

B32
rng_f32_contains_inclusive(Range_F32 range, F32 value)
{
  B32 result = (value <= range.max && value >= range.min);
  return(result);
}

F32
rng_f32_overflow(Range_F32 range, F32 value)
{
  F32 result = 0;
  if(value > range.max)
  {
    result = (range.max - value);
  }
  else if(value < range.min)
  {
    result = (range.min - value);
  }
  return(result);
}

F32 
rng_f32_size(Range_F32 range)
{
  F32 size = (range.max - range.min);
  return(size);
}

I32 
rng_i32_size(Range_I32 range)
{
  I32 size = (range.max - range.min);
  return(size);
}

Range_F32
rng_f32_center_add(Range_F32 range, F32 v)
{
  Range_F32 result = rng_f32(range.min - v, range.max + v);
  return(result);
}

V2f 
rng_v2f_center(Range_V2f range)
{
  V2f result = v2f_scale(v2f_add(range.min, range.max), 0.5f);
  return(result);
}

Range_V2f
rng_v2f_center_add(Range_V2f range, V2f v)
{
  Range_V2f result = rng_v2f(v2f_sub(range.min, v),
                             v2f_add(range.max, v));
  return(result);
}

Range_V2f
rng_v2f_center_sub(Range_V2f range, V2f v)
{
  Range_V2f result = rng_v2f(v2f_add(range.min, v),
                             v2f_sub(range.max, v));
  return(result);
}

Range_V2f
rng_v2f_center_scale(Range_V2f range, F32 scale)
{
  Assert(scale >= 0 && scale <= 1);
  V2f k = v2f_scale(rng_v2f_size(range), scale-1.0f);
  Range_V2f result = rng_v2f_center_add(range, k);
  return(result);
}

Range_V2i
rng_v2i_point_add(V2i point, V2i add)
{
  Range_V2i result = rng_v2i(point, v2i_add(point, add));
  return result;
}

V2i
rng_v2i_size(Range_V2i range)
{
  V2i result = v2i_sub(range.max, range.min);
  return result;
}

Range_V2i
rng_v2f_floor_inclusive(Range_V2f range)
{
  Range_V2i result = {0};
  result.min = v2f_floor_to_v2i(range.min);
  result.max.x = floor_up_f32_to_i32(range.max.x);
  result.max.y = floor_up_f32_to_i32(range.max.y);
  return(result);
}

B32
rng_v2f_contains_inclusive(Range_V2f range, V2f v)
{
  B32 result = (v.x <= range.max.x && v.y <= range.max.y &&
                v.x >= range.min.x && v.y >= range.min.y);
  return(result);
}

B32
rng_v2f_contains_inclusive_range(Range_V2f range, Range_V2f test)
{
  B32 result = (rng_v2f_contains_inclusive(range, test.min) && 
                rng_v2f_contains_inclusive(range, test.max));
  return(result);
}

U64 
rng_u64_size(Range_U64 range)
{
  U64 size = range.max - range.min;
  return size;
}

Range_F32
rng_f32_reflect_at(Range_F32 range, F32 point)
{
  Range_F32 result = rng_f32(point + (point - range.max),
                             point + (point - range.min));
  return(result);
}

B32 
rng_u64_inclusive(Range_U64 range, U64 value)
{
  B32 result = (range.min <= value && value <= range.max);
  return result;
}

B32 
rng_u64_inclusive_range(Range_U64 in, Range_U64 container)
{
  B32 result = (rng_u64_inclusive(container, in.min) &&
                rng_u64_inclusive(container, in.max));
  return result;
}

Range_V2i
rng_v2i_from_inclusive_rng_v2f(Range_V2f range)
{
  Range_V2i result = {0};
  result.min.x = ceil_f32_to_i32(range.min.x);
  result.min.y = ceil_f32_to_i32(range.min.y);
  
  result.max.x = floor_f32_to_i32(range.max.x);
  result.max.y = floor_f32_to_i32(range.max.y);
  return(result);
}

///////////////////////////////////////////////////////////
// NOTE(erb): color

V3f
hsv_from_rgb(V3f rgb)
{
  F32 value = Max(rgb.r, Max(rgb.g, rgb.b));
  F32 x_min = Min(rgb.r, Min(rgb.g, rgb.b));
  F32 chroma = (value - x_min);
  
  F32 hue = 0;
  if(chroma == 0)
  {
    hue = 0;
  }
  else if(value == rgb.r)
  {
    hue = 60 * mod_f32(((rgb.g - rgb.b) / chroma), 6);
  }
  else if(value == rgb.g)
  {
    hue = 60 * (((rgb.b - rgb.r) / chroma) + 2);
  }
  else if(value == rgb.b)
  {
    hue = 60 * (((rgb.r - rgb.g) / chroma) + 4);
  }
  else InvalidCodePath;
  
  F32 saturation = (value==0 ? 0 : chroma/value);
  
  V3f result = v3f(hue, saturation, value);
  return(result);
}

F32
rbg_from_hsv_transform(V3f hsv, I32 n)
{
  F32 k = mod_f32(n + hsv.h/60.0f, 6);
  F32 result = hsv.v - hsv.v*hsv.s*Cap0(Min(k, Min(4-k, 1)));
  return(result);
}

V3f
rgb_from_hsv(V3f hsv)
{
  F32 r = rbg_from_hsv_transform(hsv, 5);
  F32 g = rbg_from_hsv_transform(hsv, 3);
  F32 b = rbg_from_hsv_transform(hsv, 1);
  V3f result = v3f(r, g, b);
  return(result);
}

V3f
rgb_from_rgba(V4f rgba)
{
  V3f result = v3f_scale(rgba.rgb, rgba.a);
  return(result);
}

V3f
rgb_f32_from_rgb_255(V3i rgb)
{
  V3f result = v3f_scale(v3fi(rgb), 1/255.0f);
  return(result);
}

V3f
rgb_from_packed_u32(U32 v)
{
  V3i rgb255 = {0};
  rgb255.r = ((v >> 16) & 0xFF);
  rgb255.g = ((v >>  8) & 0xFF);
  rgb255.b = ((v >>  0) & 0xFF);
  V3f result = rgb_f32_from_rgb_255(rgb255);
  return(result);
}

V4f
rgba_from_packed_u32(U32 v)
{
  V4f result = {0};
  result.rgb = rgb_from_packed_u32(v);
  result.a = 1.0f;
  return(result);
}

U32
rgb_to_packed_u32(V3f rgb)
{
  U32 r = (U32)floor_f32_to_i32(rgb.r*255);
  U32 g = (U32)floor_f32_to_i32(rgb.g*255);
  U32 b = (U32)floor_f32_to_i32(rgb.b*255);
  U32 result = ((r << 16) |
                (g << 8) | 
                (b << 0));
  return(result);
}

V3i
rgb_255_from_rgb_f32(V3f rgb)
{
  V3f norm = v3f_scale(rgb, 255.f);
  V3i result = v3f_floor_to_v3i(norm);
  return(result);
}
