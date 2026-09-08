global Arena *img_current_arena;

#define STBI_NO_STDIO
#define STBI_ASSERT(condition) Assert(condition)

// TODO(erb): define memory with heap allocator
#if 0
#define STBI_MALLOC(size)
#define STBI_REALLOC_SIZED(ptr, old_size, new_size) 
#define STBI_FREE(p)
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"

IMG_Loaded
img_load_from_memory(Arena *arena, String8 memory)
{
  I32 buffer_length = safe_truncate_U64_to_U32(memory.size);
  I32 x, y, channels;
  U8 *result_data = stbi_load_from_memory((stbi_uc const *)memory.value, buffer_length, 
                                          &x, &y, &channels, 0); //0 = desired_channels
  IMG_Loaded result = {0};
  if (result_data)
  {
    // TODO(erb): remove this extra copy
    result.data = push_str8_copy(arena, S8Struct(result_data, x*y*channels));
    result.channels = channels;
    result.size = v2i(x, y);
    
    stbi_image_free(result_data);
  }
  else
  {
    err_set_error(S8CStr((char *)stbi_failure_reason()));
  }
  return(result);
}

IMG_Loaded 
img_generate_blank_image(Arena *arena, U32 width, U32 height)
{
  IMG_Loaded result = {0};
  result.size = v2i(width, height);
  result.channels = 4;
  
  String8 data = {0};
  data.size = result.channels * width * height;
  data.value = PushArray(arena, data.size, U8);
  
  mem_set(data.value, data.size, (1 << 8) - 1);
  
  result.data = data;
  return(result);
}
