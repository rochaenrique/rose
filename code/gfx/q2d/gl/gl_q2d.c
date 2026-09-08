global GL_Renderer gl_renderer;

#include <math.h>

GLuint
load_shaders_and_program(String8 shader_directory)
{
  GLuint program = 0;
  Shader_Param shaders[] = 
  {
    { .path = S8(GL_Q2D_VERTEX_SHADER_FILE_NAME)  , .type = GL_VERTEX_SHADER  , },
    { .path = S8(GL_Q2D_FRAGMENT_SHADER_FILE_NAME), .type = GL_FRAGMENT_SHADER, },
  };
  
  //- load files and set full path
  Scratch scratch = scratch_begin(0, 0);
  for(EachElement(Shader_Param, sh, shaders))
  {
    sh->path = os_concat_path(scratch.arena, shader_directory, sh->path);
    sh->source = os_debug_read_entire_file(sh->path);
    Assert(sh->source.size > 0);
  }
  
  program = gl_make_program(shaders, ArraySize(shaders));
  
  // TODO(erb): this should be dynamic according to GL_MAX_TEXTURE_IMAGE_UNITS
  GL(glUseProgram(program));
  GLint textures[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, };
  GLint uniform = glGetUniformLocation(program, "u_textures");
  Assert(uniform != -1);
  GL(glUniform1iv(uniform, ArraySize(textures), textures));
  
  //- free file source
  for (EachElement(Shader_Param, sh, shaders))
  {
    os_debug_free_file_memory((void *)sh->source.value);
  }
  scratch_end(&scratch);
  return(program);
}


B32
q2d_init(String8 shader_directory)
{
  B32 result = false;
  gfx_gl_attach_begin();
  {
    GL_Renderer gl = {0};
    gl.resolution_uniform = -1;
    
    //- shaders and program
    gl.program = load_shaders_and_program(shader_directory);
    
    if (gl.program != 0)
    {
      result = true;
      
      // NOTE(erb): buffers setup
      GL(glGenVertexArrays(1, &gl.vao));
      GL(glBindVertexArray(gl.vao));
      
      GL(glGenBuffers(1, &gl.vbo));
      GL(glBindBuffer(GL_ARRAY_BUFFER, gl.vbo));
      
      //- attributes
      {
        Buffer_Attribs attribs = {0};
        attribs.should_divide = true;
        
        push_struct_attrib_F32(&attribs, Q2D_Quad,   dest_bottom_left, V2f);
        push_struct_attrib_F32(&attribs, Q2D_Quad,     dest_top_right, V2f);
        push_struct_attrib_F32(&attribs, Q2D_Quad,    src_bottom_left, V2f);
        push_struct_attrib_F32(&attribs, Q2D_Quad,      src_top_right, V2f);
        
        push_struct_attrib_F32(&attribs, Q2D_Quad,  color_bottom_left, V4f);
        push_struct_attrib_F32(&attribs, Q2D_Quad, color_bottom_right, V4f);
        push_struct_attrib_F32(&attribs, Q2D_Quad,     color_top_left, V4f);
        push_struct_attrib_F32(&attribs, Q2D_Quad,    color_top_right, V4f);
        
        push_struct_attrib_F32(&attribs, Q2D_Quad,      corner_radius, F32);
        push_struct_attrib_F32(&attribs, Q2D_Quad,      edge_softness, F32);
        push_struct_attrib_F32(&attribs, Q2D_Quad,   border_thickness, F32);
        push_struct_attrib_F32(&attribs, Q2D_Quad,       texture_slot, F32);
        Assert(attribs.total_size == sizeof(Q2D_Quad));
      }
      
      gl_renderer = gl;
    }
    else
    {
      err_set_error(S8("failed to create gl program"));
    }
  }
  gfx_gl_attach_end();
  return(result);
}

void
q2d_begin(GFX_Window window)
{
  if (gfx_window_is_valid(window))
  {
    gfx_gl_render_begin(window);
    
    V2i viewport = gfx_window_get_client_size(window);
    
    GL(glViewport(0, 0, viewport.x, viewport.y));
    
    GL(glEnable(GL_BLEND));
    GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    
    GL(glClearColor(V4Expand(color_white)));
    GL(glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT));
    
    gl_renderer.viewport_size = viewport;
  }
}

void
q2d_clear(Q2D_State *state)
{
  state->quad_count = 0;
  state->batch_count = 0;
  mem_zero((U8 *)state->batches, sizeof(state->batches));
}

void
q2d_end(GFX_Window window, Q2D_State *state)
{
  q2d_clear(state);
  gl_renderer.viewport_size = v2ii(0);
  gfx_gl_render_end(window);
}

GLuint
gl_pixel_format_from_q2d(Q2D_Pixel_Format q2d_pixel_fmt)
{
  GLuint fmt = 0;
  switch(q2d_pixel_fmt)
  {
    case Q2DPixelFormat_R   : fmt = GL_RED  ; break;
    case Q2DPixelFormat_RG  : fmt = GL_RG   ; break;
    case Q2DPixelFormat_RGB : fmt = GL_RGB  ; break;
    case Q2DPixelFormat_RGBA: fmt = GL_RGBA ; break;
  }
  return(fmt);
}

Q2D_Image_Texture 
q2d_create_texture_from_bitmap(U8 *bitmap_memory, V2i bitmap_size, Q2D_Pixel_Format pixel_fmt, 
                               Q2D_Pixel_Format target_fmt, Q2D_Texture_Flags flags, Q2D_Swizzle swizzle)
{
  Assert(bitmap_memory);
  Assert(bitmap_size.x != 0 && bitmap_size.y != 0);
  
  GLuint gl_pixel_format = gl_pixel_format_from_q2d(pixel_fmt);
  GLuint gl_target_format = gl_pixel_format_from_q2d(target_fmt);
  
  GLuint texture_id;
  gfx_gl_attach_begin();
  {
    GL(glGenTextures(1, &texture_id));
    GL(glBindTexture(GL_TEXTURE_2D, texture_id));
    
    if(flags & Q2DTextureFlag_MinMagLinear)
    {
      GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
      GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    }
    if(flags & Q2DTextureFlag_WrapClampToEdge)
    {
      
      GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
      GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    }
    
    GL(glTexImage2D(GL_TEXTURE_2D, 
                    0,                //LOD
                    gl_target_format, //target gl format
                    bitmap_size.x, 
                    bitmap_size.y, 
                    0,                //border (value must be 0 - from the docs)
                    gl_pixel_format,  //pixel data format
                    GL_UNSIGNED_BYTE, 
                    bitmap_memory));
    
    GLenum components[] = 
    {
      GL_TEXTURE_SWIZZLE_R,
      GL_TEXTURE_SWIZZLE_G,
      GL_TEXTURE_SWIZZLE_B,
      GL_TEXTURE_SWIZZLE_A,
    };
    for(EachIdx(idx, components))
    {
      if(swizzle.v[idx] != Q2DSwizzle_None)
      {
        GLint param = 0;
        switch(swizzle.v[idx])
        {
          case Q2DSwizzle_None: break;
          case Q2DSwizzle_R   : param = GL_RED  ; break;
          case Q2DSwizzle_G   : param = GL_GREEN; break;
          case Q2DSwizzle_B   : param = GL_BLUE ; break;
          case Q2DSwizzle_A   : param = GL_ALPHA; break;
          case Q2DSwizzle_One : param = GL_ONE  ; break;
          case Q2DSwizzle_Zero: param = GL_ZERO ; break;
        }
        GL(glTexParameteri(GL_TEXTURE_2D, components[idx], param));
      }
    }
    
    // NOTE(erb): this has to be after 'glTexImage2D' 
    if(flags & Q2DTextureFlag_GenMipMap)
    {
      GL(glGenerateTextureMipmap(texture_id));
    }
  }
  gfx_gl_attach_end();
  
  Q2D_Image_Texture result = {0};
  result.handle = texture_id;
  result.size = bitmap_size;
  return(result);
}

GLint
get_uniform(GLint *cached, char *name)
{
  if(*cached == -1)
  {
    *cached = glGetUniformLocation(gl_renderer.program, name);
    Assert(*cached != -1);
  }
  return(*cached);
}

void 
q2d_render(Q2D_State *renderer)
{
  Assert(renderer->quad_count < renderer->quad_capacity);
  Assert(renderer->batch_count < ArraySize(renderer->batches));
  
  //NOTE(erb): bind program
  GL(glUseProgram(gl_renderer.program));
  
  // NOTE(erb): resolution
  // TODO(erb): change this to actual monitor resolution
  GL(glUniform2f(get_uniform(&gl_renderer.resolution_uniform, "u_resolution"), 
                 (F32)gl_renderer.viewport_size.x, (F32)gl_renderer.viewport_size.y));
  
  // NOTE(erb): batches
  for (U32Each(batch_idx, renderer->batch_count))
  {
    Q2D_Batch *batch = (renderer->batches + batch_idx);
    
    if(batch->rect_count > 0)
    {
      // NOTE(erb): texture slots
      for (U32Each(texture_idx, batch->texture_count))
      {
        GL(glActiveTexture(GL_TEXTURE0 + texture_idx));
        
        Q2D_Texture handle = batch->textures[texture_idx];
        U32 tex = safe_truncate_U64_to_U32(handle);
        GL(glBindTexture(GL_TEXTURE_2D, tex));
      }
      
      Assert(renderer->quad_count <= renderer->quad_capacity);
      Q2D_Quad *rects_begin = renderer->quads + batch->start_rect_index;
      Assert(batch->start_rect_index + batch->rect_count <= renderer->quad_count);
      
      // NOTE(erb): bind buffer
      GL(glBindBuffer(GL_ARRAY_BUFFER, gl_renderer.vbo));
      GL(glBufferData(GL_ARRAY_BUFFER, sizeof(Q2D_Quad)*batch->rect_count, rects_begin, GL_STREAM_DRAW));
      
      // NOTE(erb): clip begin
      if(batch->should_clip)
      {
        glEnable(GL_SCISSOR_TEST);
        V2i size = rng_v2i_size(batch->clip_range);
        I32 y = gl_renderer.viewport_size.y - batch->clip_range.min.y - size.y;
        glScissor(batch->clip_range.min.x, y, size.x, size.y);
      }
      
      // NOTE(erb): draw
      {
        GL(glBindVertexArray(gl_renderer.vao));
        GL(glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (U32)batch->rect_count));
      }
      
      // NOTE(erb): clip end
      if(batch->should_clip)
      {
        glDisable(GL_SCISSOR_TEST);
      }
      
      // NOTE(erb): unbind stuff
      {
        GL(glBindVertexArray(0));
        GL(glBindBuffer(GL_ARRAY_BUFFER, 0));
      }
      
      for (U32Each(idx, batch->texture_count))
      {
        GL(glActiveTexture(GL_TEXTURE0 + idx));
        GL(glBindTexture(GL_TEXTURE_2D, 0));
      }
    }
  }
}

String8_Array     
q2d_get_shader_file_names(Arena *arena)
{
  String8_Array array = {0};
  array.items = PushArray(arena, 2, String8);
  array.items[array.count++] = push_str8_copy(arena, S8(GL_Q2D_VERTEX_SHADER_FILE_NAME));
  array.items[array.count++] = push_str8_copy(arena, S8(GL_Q2D_FRAGMENT_SHADER_FILE_NAME));
  return(array);
}

void
q2d_update_shaders(String8 shader_directory)
{
  gfx_gl_attach_begin();
  GLuint new_program = load_shaders_and_program(shader_directory);
  if(new_program != 0)
  {
    GL(glDeleteProgram(gl_renderer.program));
    gl_renderer.program = new_program;
  }
  gfx_gl_attach_end();
}

void
q2d_scissor_begin(Q2D_State *state, Range_V2i range)
{
  Q2D_Batch *batch = (state->batches + state->batch_count++);
  ZeroStruct(batch);
  batch->start_rect_index = state->quad_count;
  batch->clip_range = range;
  batch->should_clip = true;
}

void
q2d_scissor_end(Q2D_State *state)
{
  Q2D_Batch *last_batch = (state->batches + state->batch_count-1);
  Assert(last_batch);
  Q2D_Batch *new_batch = (state->batches + state->batch_count++);
  ZeroStruct(new_batch);
}

