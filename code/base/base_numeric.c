U32 safe_truncate_U64_to_U32(U64 value)
{
  assert(value <= 0xFFFFFFFF);
  U32 result = (U32)value;
  return result;
}

F32 lerp(F32 min, F32 max, F32 t)
{
  F32 result = min + (max - min)*t;
  return result;
}

V2f v2f(F32 x, F32 y)
{
	V2f result = {0};
	result.x = x;
	result.y = y;
	return result;
}

V2f v2ff(F32 v)
{
	V2f result = {0};
	result.x = result.y = v;
	return result;
}

V2f v2fi(V2i v)
{
	V2f result = {0};
  result.x = (F32)v.x;
	result.y = (F32)v.y;
	return result;
}

V2f v2f_add(V2f a, V2f b)
{
	V2f result = {0};
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	return result;
}

V2f v2f_sub(V2f a, V2f b)
{
	V2f result = {0};
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return result;
}

V2f v2f_hada(V2f a, V2f b)
{
	V2f result = {0};
	result.x = a.x * b.x;
	result.y = a.y * b.y;
	return result;
}

V2f v2f_hada_div(V2f a, V2f b)
{
	V2f result = {0};
	result.x = a.x / b.x;
	result.y = a.y / b.y;
	return result;
}

V2f v2f_scale(V2f vec, F32 scalar) 
{
	V2f result = {0};
	result.x = vec.x * scalar;
	result.y = vec.y * scalar;
	return result;
}

V2f v2f_abs(V2f v) 
{
  V2f result = {0};
	result.x = abs(v.x);
	result.y = abs(v.y);
  return result;
}

V2f v2f_lerp(V2f min, V2f max, F32 t) 
{
  V2f result = {0};
	result.x = lerp(min.x, max.x, t);
	result.y = lerp(min.y, max.y, t);
  return result;
}

V2f v2f_min(V2f a, V2f b)
{
  V2f result = {0};
	result.x = min(a.x, b.x);
	result.y = min(a.y, b.y);
  return result;
}

V2f v2f_reverse_y(V2f v, F32 y)
{
  V2f result = {0};
  result.x = v.x;
  result.y = y - v.y;
  return result;
}

// NOTE(erb): v2i

V2i v2i(I32 x, I32 y)
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

V2i v2i_add(V2i a, V2i b)
{
	V2i result = {0};
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	return result;
}

V2i v2i_sub(V2i a, V2i b)
{
	V2i result = {0};
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return result;
}

B32 v2i_equals(V2i a, V2i b)
{
  B32 result = (a.x == b.x && a.y == b.y);
  return result;
}

// NOTE(erb): v3f

V3f v3f(F32 x, F32 y, F32 z)
{
	V3f result = {0};
  result.x = x;
  result.y = y;
  result.z = z;
	return result;
}

V3f v3ff(F32 v)
{
	V3f result = {0};
	result.x = result.y = result.z = v;
	return result;
}

// NOTE(erb): v4

V4f v4f(F32 x, F32 y, F32 z, F32 w)
{
	V4f result = {0};
  result.x = x;
  result.y = y;
  result.z = z;
  result.w = w;
	return result;
}

V4f v4ff(F32 v)
{
	V4f result = {0};
  result.x = result.y = result.z = result.w = v;
	
	return result;
}

V4f with_alpha(V4f color, F32 a)
{
  V4f result = color;
  result.a = a;
  return result;
}

V4f v4f_lerp(V4f min, V4f max, F32 t)
{
  V4f result = {0};
  result.x = lerp(min.x, max.x, t);
  result.y = lerp(min.y, max.y, t);
  result.z = lerp(min.z, max.z, t);
  result.w = lerp(min.w, max.w, t);
  return result;
}

// NOTE(erb): rects and ranges
V2f rect_size(Screen_Rect_F32 size);

Range_V2f range_point_add(V2f point, V2f add)
{
  Range_V2f result = {0};
  result.min = point;
  result.max = v2f_add(point, add);
  return result;
}

Range_V2f to_range(Screen_Rect_F32 srect, F32 max_y)
{
  V2f size = rect_size(srect);
  V2f pos = v2f(srect.left, max_y - srect.top - size.y);
  Range_V2f result = range_point_add(pos, size);
  return result;
}

Range_V2f range_from_size(V2f size)
{
  Range_V2f result = {0};
  result.max = size;
  return result;
}

Screen_Rect_F32 rect_from_size(V2f size)
{
  Screen_Rect_F32 result = {0};
  result.bottom_right = size;
  return result;
}

Screen_Rect_F32 rect_top_left(V2f top_left, V2f size)
{
  Screen_Rect_F32 result = {0};
  result.top_left = top_left;
  result.bottom_right = v2f_add(top_left, size);
  return result;
}

Screen_Rect_F32 rect_center(V2f center, V2f size)
{
  V2f top_left = v2f_sub(center, v2f_scale(size, 0.5f));
  Screen_Rect_F32 result = rect_top_left(top_left, size);
  return result;
}

V2f rect_size(Screen_Rect_F32 size)
{
  V2f result = v2f(size.right - size.left, size.bottom - size.top);
  return result;
}
