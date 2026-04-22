#include "marker.c"
#include "marker_platform.h"
#include <stdio.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/wglext.h>

typedef struct GL_API
{
  PFNGLGETTEXTUREIMAGEPROC glGetTextureImage;
  PFNGLDELETEPROGRAMPROC glDeleteProgram;
  PFNGLBINDTEXTUREUNITPROC glBindTextureUnit;
  PFNGLACTIVETEXTUREPROC glActiveTexture;
  PFNGLBUFFERDATAPROC glBufferData;
  PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
  PFNGLUSEPROGRAMPROC glUseProgram;
  PFNGLUNIFORM2FPROC glUniform2f;
  PFNGLUNIFORM1IVPROC glUniform1iv;
  PFNGLDRAWARRAYSINSTANCEDPROC glDrawArraysInstanced;
  PFNGLBINDBUFFERPROC glBindBuffer;
  PFNGLGENBUFFERSPROC glGenBuffers;
  PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
  PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
  PFNGLATTACHSHADERPROC glAttachShader;
  PFNGLCOMPILESHADERPROC glCompileShader;
  PFNGLCREATEPROGRAMPROC glCreateProgram;
  PFNGLCREATESHADERPROC glCreateShader;
  PFNGLDELETESHADERPROC glDeleteShader;
  PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
  PFNGLGETPROGRAMIVPROC glGetProgramiv;
  PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
  PFNGLGETSHADERIVPROC glGetShaderiv;
  PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
  PFNGLLINKPROGRAMPROC glLinkProgram;
  PFNGLSHADERSOURCEPROC glShaderSource;
  PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
  PFNGLGENERATEMIPMAPPROC glGenerateMipmap;
  PFNGLVERTEXATTRIBDIVISORPROC glVertexAttribDivisor;
} GL_API;
static GL_API gl_api;

void gl_clear_error()
{
  while (glGetError() != GL_NO_ERROR);
}

I32 gl_poll_error()
{
  GLint error;
  while ((error = glGetError()))
  {
    return error;
  }
  return 0;
}

#define GL_TABLE_TOO_LARGE1 0x8031

char *debug_gl_error_code_cstr(I32 code)
{
  switch (code)
  {
    case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
    case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
    case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
    case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
    //case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_CONTEXT_LOST: return "GL_CONTEXT_LOST";
    case GL_TABLE_TOO_LARGE1: return "GL_TABLE_TOO_LARGE1";
    default: case GL_NO_ERROR: return "GL_NO_ERROR";
  }
}

// NOTE(erb): from https://wikis.khronos.org/opengl/OpenGL_Error
char *debug_gl_error_code_description(I32 code)
{
  switch (code)
  {
    case GL_INVALID_ENUM: return "Given when an enumeration parameter is not a legal enumeration for that function. This is given only for local problems; if the spec allows the enumeration in certain circumstances, where other parameters or state dictate those circumstances, then GL_INVALID_OPERATION is the result instead.";
    case GL_INVALID_VALUE: return "Given when a value parameter is not a legal value for that function. This is only given for local problems; if the spec allows the value in certain circumstances, where other parameters or state dictate those circumstances, then GL_INVALID_OPERATION is the result instead.";
    case GL_INVALID_OPERATION: return "Given when the set of state for a command is not legal for the parameters given to that command. It is also given for commands where combinations of parameters define what the legal parameters are.";
    case GL_STACK_OVERFLOW: return "Given when a stack pushing operation cannot be done because it would overflow the limit of that stack's size.";
    case GL_STACK_UNDERFLOW: return "Given when a stack popping operation cannot be done because the stack is already at its lowest point.";
    case GL_OUT_OF_MEMORY: return "Given when performing an operation that can allocate memory, and the memory cannot be allocated. The results of OpenGL functions that return this error are undefined; it is allowable for partial execution of an operation to happen in this circumstance.";
    //case GL_INVALID_FRAMEBUFFER_OPERATION: return "Given when doing anything that would attempt to read from or write/render to a framebuffer that is not complete.";
    case GL_CONTEXT_LOST: return "Given if the OpenGL context has been lost, due to a graphics card reset.";
    case GL_TABLE_TOO_LARGE1: return "Part of the ARB_imaging extension.";
    default: case GL_NO_ERROR: return "No error";
  }
}

#define gl_check_error(call_str) \
do \
{ \
I32 error = gl_poll_error(); \
if (error) \
{ \
printf("%s:%d: [GL CALL ERROR] (0x%x) : \"%s\" : %s : %s\n", __FILE__, __LINE__, error, call_str, \
debug_gl_error_code_cstr(error), debug_gl_error_code_description(error)); \
} \
} while (0)

#define gl(call) \
do \
{ \
gl_clear_error(); \
call; \
gl_check_error(#call); \
} \
while (0)

U32 gl_create_compile_shader(I32 shader_type, const char *src)
{
  gl_clear_error();
  U32 shader = gl_api.glCreateShader(shader_type);
  gl_check_error("createShader");
  
  gl(gl_api.glShaderSource(shader, 1, &src, 0));
  gl(gl_api.glCompileShader(shader));
  
  I32 success;
  gl(gl_api.glGetShaderiv(shader, GL_COMPILE_STATUS, &success));
  if (!success)
  {
    char info_log[512];
    gl(gl_api.glGetShaderInfoLog(shader, 512, 0, info_log));
    printf("ERROR: Shader: %s\n", info_log);
    printf("%s\n", src);
  }
  
  return shader;
}


U32 gl_make_program(const char *vertex_src, const char *fragment_src, B32 should_print_log)
{
  gl_clear_error();
  U32 shader_program = gl_api.glCreateProgram();
  gl_check_error("glCreateProgram");
  assert(shader_program != 0 && "glCreateProgram failed");
  
  // NOTE(erb): shaders setup
  U32 vertex_shader = gl_create_compile_shader(GL_VERTEX_SHADER, vertex_src);
  U32 fragment_shader = gl_create_compile_shader(GL_FRAGMENT_SHADER, fragment_src);
  
  gl(gl_api.glAttachShader(shader_program, vertex_shader));
  gl(gl_api.glAttachShader(shader_program, fragment_shader));
  gl(gl_api.glLinkProgram(shader_program));
  {
    I32 success;
    gl_api.glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success)
    {
      char info_log[512];
      gl_api.glGetProgramInfoLog(shader_program, 512, 0, info_log);
      printf("ERROR: Shader LINK (id %d): '%s'\n", shader_program, info_log);
    }
  }
  
  if (should_print_log)
  {
    printf("VERTEX ================\n%s\n", vertex_src);
    printf("FRAGME ================\n%s\n", fragment_src);
  }
  
  gl(gl_api.glDeleteShader(vertex_shader));
  gl(gl_api.glDeleteShader(fragment_shader));
  
  return shader_program;
}

Image_Texture gl_make_texture_from_image(Loaded_Image *image)
{
  assert(image);
  assert(image->data);
  
  U32 texture_id;
  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  
  GLuint gl_image_format = 0;
  if (image->channels == 3) 
  {
    gl_image_format = GL_RGB;
  }
  else if (image->channels == 4) 
  {
    gl_image_format = GL_RGBA;
  }
  
  
#if 0  
  glTexImage2D(GL_TEXTURE_2D, 0, 
               gl_image_format, 
               image->size.x, image->size.y, 
               0, 
               gl_image_format, 
               GL_UNSIGNED_BYTE, 
               image->data);
#else
  glTexImage2D(GL_TEXTURE_2D, 
               0, 
               GL_RGBA, 
               image->size.x, image->size.y, 
               0, 
               GL_RGBA, 
               GL_UNSIGNED_BYTE, 
               image->data);
#endif
  
  
  //gl_api.glGenerateMipmap(GL_TEXTURE_2D);
  
  Image_Texture texture = {0};
  texture.id = texture_id;
  texture.size = v2fi(image->size);
  
  return texture;
}

void gl_debug_texture(Arena *arena, U32 id, U32 width, U32 height, U32 n_channels)
{
  Scratch scratch = scratch_begin(arena);
  
  U32 size = (U32)(width * height * n_channels);
  void *pixels = push_array(scratch.arena, size, U8);
  gl_api.glGetTextureImage(id,
                           0,
                           GL_RGBA,
                           GL_UNSIGNED_BYTE,
                           size,
                           pixels);
  
  scratch_end(&scratch);
}

Loaded_Image generate_blank_image(Arena *arena, U32 width, U32 height)
{
  Loaded_Image image = {0};
  image.size = v2i(width, height);
  image.channels = 4;
  U64 size = image.channels * width * height;
  image.data = push_array(arena, size, unsigned char);
  for (U64 idx = 0;
       idx < size;
       idx++)
  {
    *(image.data + idx) = (1 << 8) - 1;
  }
  
  return image;
}

Loaded_Image generate_debug_image(Arena *arena, U32 width, U32 height, U32 pixel_value)
{
  Loaded_Image image = {0};
  image.size = v2i(width, height);
  image.channels = 4;
  U64 size = image.channels * width * height;
  image.data = push_array(arena, size, unsigned char);
  
  B32 spot = true;
  
  for (U32 y = 0;
       y < height;
       y++)
  {
    for (U32 x = 0;
         x < width;
         x++)
    {
      U64 idx = (y * width) + x;
      U32 *pixel = (U32 *)image.data + idx;
      
      if (spot) 
      {
        *pixel = pixel_value;
      }
      else
      {
        *pixel = 0xFFFFFFFF;
      }
      
      if (((x % 10) == 0) || 
          ((y % 10) == 0)) 
      {
        spot = !spot;
      }
    }
  }
  
  return image;
}


#define push_struct_attrib_F32(attrib_ctx, Struct_Type, field_name, Field_Type) \
_push_struct_attrib_F32(attrib_ctx, sizeof(Struct_Type), sizeof(Field_Type), offsetof(Struct_Type, field_name)) 
void _push_struct_attrib_F32(Buffer_Attribs *attrib_ctx, I32 struct_size, I64 field_size, U64 field_offset) 
{
  assert(field_size % sizeof(F32) == 0);
  gl_api.glEnableVertexAttribArray(attrib_ctx->count);
  
  I32 component_count = (I32)(field_size/sizeof(F32));
  gl_api.glVertexAttribPointer(attrib_ctx->count, 
                               component_count, 
                               GL_FLOAT, 
                               GL_FALSE, 
                               struct_size, 
                               (void *)field_offset);
  gl_api.glVertexAttribDivisor(attrib_ctx->count, 1);
  attrib_ctx->count++;
  attrib_ctx->total_size += (U32)field_size;
}

#define push_struct_attrib_U32(attrib_ctx, Struct_Type, field_name) \
_push_struct_attrib_U32(attrib_ctx, sizeof(Struct_Type), offsetof(Struct_Type, field_name)) 
void _push_struct_attrib_U32(Buffer_Attribs *attrib_ctx, I32 struct_size, U64 field_offset) 
{
  gl_api.glEnableVertexAttribArray(attrib_ctx->count);
  gl_api.glVertexAttribPointer(attrib_ctx->count, 
                               1, 
                               GL_UNSIGNED_INT, 
                               GL_TRUE, 
                               struct_size, 
                               (void *)field_offset);
  gl_api.glVertexAttribDivisor(attrib_ctx->count, 1);
  attrib_ctx->count++;
  attrib_ctx->total_size += (U32)sizeof(U32);
}

Render_Data gl_make_renderer(Arena *arena, char *vertex_shader_src, char *fragment_shader_src)
{
  Render_Data renderer = {0};
  
  Loaded_Image blank_image = generate_blank_image(arena, 800, 800);
  renderer.blank_image_texture = gl_make_texture_from_image(&blank_image);
  
  renderer.rects.capacity = 100000;
  renderer.rects.buffer = push_array(arena, renderer.rects.capacity, Render_Rect);
  
  // NOTE(erb): shaders setup
  renderer.program = gl_make_program(vertex_shader_src, fragment_shader_src, false);
  
  // NOTE(erb): buffers setup
  gl(gl_api.glGenVertexArrays(1, &renderer.vao));
  gl(gl_api.glBindVertexArray(renderer.vao));
  
  gl(gl_api.glGenBuffers(1, &renderer.vbo));
  gl(gl_api.glBindBuffer(GL_ARRAY_BUFFER, renderer.vbo));
  {
    Buffer_Attribs attribs = {0};
    push_struct_attrib_F32(&attribs, Render_Rect, dest_bottom_left, V2f);
    push_struct_attrib_F32(&attribs, Render_Rect, dest_top_right, V2f);
    push_struct_attrib_F32(&attribs, Render_Rect, src_bottom_left, V2f);
    push_struct_attrib_F32(&attribs, Render_Rect, src_top_right, V2f);
    
    push_struct_attrib_F32(&attribs, Render_Rect, color_bottom_left, V4f);
    push_struct_attrib_F32(&attribs, Render_Rect, color_bottom_right, V4f);
    push_struct_attrib_F32(&attribs, Render_Rect, color_top_left, V4f);
    push_struct_attrib_F32(&attribs, Render_Rect, color_top_right, V4f);
    
    push_struct_attrib_F32(&attribs, Render_Rect, corner_radius, F32);
    push_struct_attrib_F32(&attribs, Render_Rect, edge_softness, F32);
    push_struct_attrib_F32(&attribs, Render_Rect, border_thickness, F32);
    //push_struct_attrib_U32(&attribs, Render_Rect, texture_slot);
    push_struct_attrib_F32(&attribs, Render_Rect, texture_slot, F32);
    assert(attribs.total_size == sizeof(Render_Rect));
  }
  return renderer;
}

void gl_update_shaders(Render_Data *renderer, char *vertex_shader_src, char *fragment_shader_src)
{
  gl(gl_api.glDeleteProgram(renderer->program));
  renderer->program = gl_make_program(vertex_shader_src, fragment_shader_src, false);
}

U32 gl_load_glyph_texture(Arena *arena, U8 *bitmap, U32 width, U32 height)
{
  
  U32 texture_id;
  gl(glGenTextures(1, &texture_id));
  gl(glBindTexture(GL_TEXTURE_2D, texture_id));
  gl(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
  gl(glTexImage2D(GL_TEXTURE_2D, 
                  0, 
                  GL_R8, 
                  width,
                  height,
                  0,
                  GL_RED,
                  GL_UNSIGNED_BYTE, 
                  bitmap));
  
  
  gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ONE));
  gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ONE));
  gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ONE));
  gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED));
  
  gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
  gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  
  gl_debug_texture(arena, texture_id, width, height, 1);
  
  return texture_id;
}

void gl_render(Render_Data *renderer, V2i viewport)
{
  supress_unused(viewport);
  gl(glViewport(0, 0, viewport.x, viewport.y));
  
  // NOTE(erb): 
  gl(glEnable(GL_BLEND));
  gl(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
  
  gl(glClearColor(v4_expand(color_blue)));
  gl(glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT));
  
  // NOTE(erb): resolution
  // TODO(erb): change this to actual monitor resolution
  gl(gl_api.glUseProgram(renderer->program));
  gl(gl_api.glUniform2f(gl_api.glGetUniformLocation(renderer->program, "u_resolution"), 
                        (F32)viewport.x, (F32)viewport.y));
  
  // NOTE(erb): batches
  for (U32 batch_idx = 0;
       batch_idx < renderer->batch_count;
       batch_idx++)
  {
    Render_Batch *batch = (renderer->batches + batch_idx);
    U64 end_rect_idx = renderer->rects.count;
    if (batch_idx < renderer->batch_count - 1)
    {
      end_rect_idx = renderer->batches[batch_idx+1].start_rect_index;
    }
    
    U64 rect_count = end_rect_idx - batch->start_rect_index;
    assert(rect_count <= renderer->rects.capacity);
    Render_Rect *rects_begin = renderer->rects.buffer + batch->start_rect_index;
    
    // NOTE(erb): bind buffer
    gl(gl_api.glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo));
    gl(gl_api.glBufferData(GL_ARRAY_BUFFER, sizeof(Render_Rect)*rect_count, rects_begin, GL_STREAM_DRAW));
    
    // NOTE(erb): texture slots
    for (U32 texture_idx = 0;
         texture_idx < batch->texture_count;
         texture_idx++) 
    {
      U32 tex = batch->textures[texture_idx];
      U32 unit = GL_TEXTURE0 + texture_idx;
      gl(gl_api.glActiveTexture(unit));
      gl(glBindTexture(GL_TEXTURE_2D, tex));
      
    }
    
    U32 textures[16] = { 
      0, 1, 2, 3, 
      4, 5, 6, 7,
      8, 9, 10, 11, 
      12, 13, 14, 15,
    };
    gl(gl_api.glUseProgram(renderer->program));
    gl(gl_api.glUniform1iv(gl_api.glGetUniformLocation(renderer->program, "u_textures"), batch->texture_count, (GLint *)textures));
    
    // NOTE(erb): draw
    {
      gl(gl_api.glBindVertexArray(renderer->vao));
      gl(gl_api.glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (U32)rect_count));
    }
    
    // NOTE(erb): unbind stuff
    {
      gl(gl_api.glBindVertexArray(0));
      gl(gl_api.glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo));
      gl(gl_api.glBufferData(GL_ARRAY_BUFFER, 0, 0, GL_STREAM_DRAW));
    }
  }
  
}
void gl_load_functions()
{
  gl_api.glGetTextureImage = (PFNGLGETTEXTUREIMAGEPROC)wglGetProcAddress("glGetTextureImage");
  gl_api.glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
  gl_api.glBindTextureUnit = (PFNGLBINDTEXTUREUNITPROC)wglGetProcAddress("glBindTextureUnit");
  gl_api.glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
  gl_api.glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
  gl_api.glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
  gl_api.glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
  gl_api.glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
  gl_api.glUniform2f = (PFNGLUNIFORM2FPROC)wglGetProcAddress("glUniform2f");
  gl_api.glUniform1iv = (PFNGLUNIFORM1IVPROC)wglGetProcAddress("glUniform1iv");
  gl_api.glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)wglGetProcAddress("glDrawArraysInstanced");
  gl_api.glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
  gl_api.glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
  gl_api.glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
  gl_api.glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
  gl_api.glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
  gl_api.glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
  gl_api.glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
  gl_api.glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
  gl_api.glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
  gl_api.glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
  gl_api.glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
  gl_api.glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
  gl_api.glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
  gl_api.glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
  gl_api.glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
  gl_api.glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
  gl_api.glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
  gl_api.glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)wglGetProcAddress("glGenerateMipmap");
  gl_api.glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISORPROC)wglGetProcAddress("glVertexAttribDivisor");
  
  char msg[128];
  {
    sprintf_s(msg, sizeof(msg), "LOG: OpenGL version: %s\n", glGetString(GL_VERSION));
    OutputDebugStringA(msg);
  }
  
  {
    sprintf_s(msg, sizeof(msg), "LOG: OpenGL Vendor: %s\n", glGetString(GL_VENDOR));
    OutputDebugStringA(msg);
  }
  
  {
    sprintf_s(msg, sizeof(msg), "LOG: OpenGL Renderer: %s\n", glGetString(GL_RENDERER));
    OutputDebugStringA(msg);
  }
  
  {
    sprintf_s(msg, sizeof(msg), "LOG: OpenGL Version: %s\n", glGetString(GL_VERSION));
    OutputDebugStringA(msg);
  }
  
  {
    sprintf_s(msg, sizeof(msg), "LOG: OpenGL GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    OutputDebugStringA(msg);
  }
  
  {
    I32 max_vertex_attribs;
    gl(glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attribs));
    sprintf_s(msg, sizeof(msg), "LOG: OpenGL max vertex_shader Attributes Allowed: %d\n", max_vertex_attribs);
    OutputDebugStringA(msg);
  }
}