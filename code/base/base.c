global String8 global_error = {0};

String8
err_get_last_error()
{
  return(global_error);
}

void
err_set_error(String8 error)
{
  global_error = error;
}

Index_U32
idx_u32_true(U32 value)
{
  Index_U32 result = {0};
  result.exists = true;
  result.value = value;
  return(result);
}

Index_U64
idx_u64_true(U64 value)
{
  Index_U64 result = {0};
  result.exists = true;
  result.value = value;
  return(result);
}

Axis2
dir2_axis(Direction2 dir)
{
  Axis2 axis = (dir==Direction2_North || dir==Direction2_South) ? Axis2_Y : Axis2_X;
  return(axis);
}

// NOTE(erb): includes
#include "base_numeric.c"
#include "base_memory.c"
#include "base_string.c"
#include "base_time.c"
#include "base_os/os.c"
