char *
gl_error_code_cstr(GLenum code)
{
  switch (code)
  {
    //case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
    //case GL_CONTEXT_LOST      : return "GL_CONTEXT_LOST";
    case GL_INVALID_ENUM      : return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE     : return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION : return "GL_INVALID_OPERATION";
    case GL_STACK_OVERFLOW    : return "GL_STACK_OVERFLOW";
    case GL_STACK_UNDERFLOW   : return "GL_STACK_UNDERFLOW";
    case GL_OUT_OF_MEMORY     : return "GL_OUT_OF_MEMORY";
    default: case GL_NO_ERROR : return "GL_NO_ERROR";
  }
}

// NOTE(erb): from https://wikis.khronos.org/opengl/OpenGL_Error
char *
gl_error_code_description(GLenum code)
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
    //case GL_CONTEXT_LOST: return "Given if the OpenGL context has been lost, due to a graphics card reset.";
    default: case GL_NO_ERROR: return "No error";
  }
}


void 
gl_clear_error()
{
  GLint error;
  do
  {
    error = glGetError();
  }
  while (error != GL_NO_ERROR);
}

I32 
gl_poll_error()
{
  GLint error;
  while ((error = glGetError()))
  {
    return error;
  }
  return(0);
}

GLuint
gl_create_compile_shader(Shader_Param param)
{
  gl_clear_error();
  GLuint shader = glCreateShader(param.type);
  GLCheckError("createShader");
  
  const GLchar *const *src = (const GLchar *const *)&param.source.value;
  GL(glShaderSource(shader, 1, src, 0));
  GL(glCompileShader(shader));
  
  I32 success;
  GL(glGetShaderiv(shader, GL_COMPILE_STATUS, &success));
  if (!success)
  {
    char info_log[512];
    GL(glGetShaderInfoLog(shader, 512, 0, info_log));
    os_logf("ERROR: Shader: %s", info_log);
    os_logf("%s", param.source);
  }
  
  return shader;
}

GLuint
gl_make_program(Shader_Param *shader_params, U32 shader_count)
{
  gl_clear_error();
  GLuint shader_program = glCreateProgram();
  GLCheckError("glCreateProgram");
  Assert(shader_program != 0 && "glCreateProgram failed");
  
  // NOTE(erb): shaders setup
  Scratch scratch = scratch_begin(0, 0);
  GLuint *shaders = PushArray(scratch.arena, shader_count, GLuint);
  
  for(U32Each(idx, shader_count))
  {
    GLuint sh = gl_create_compile_shader(shader_params[idx]);
    Assert(sh != 0);
    GL(glAttachShader(shader_program, sh));
    shaders[idx] = sh;
  }
  
  GL(glLinkProgram(shader_program));
  {
    I32 success;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success)
    {
      char info_log[512];
      glGetProgramInfoLog(shader_program, 512, 0, info_log);
      os_logf("ERROR: Shader LINK (id %d): '%s'", shader_program, info_log);
    }
  }
  
  for (U32Each(idx, shader_count))
  {
    GL(glDeleteShader(shaders[idx]));
  }
  
  scratch_end(&scratch);
  return(shader_program);
}


#define push_struct_attrib_F32(attrib_ctx, Struct_Type, field_name, Field_Type) \
_push_struct_attrib_F32(attrib_ctx, sizeof(Struct_Type), sizeof(Field_Type), offsetof(Struct_Type, field_name)) 
void 
_push_struct_attrib_F32(Buffer_Attribs *attrib_ctx, I32 struct_size, I64 field_size, U64 field_offset) 
{
  Assert(field_size % sizeof(F32) == 0);
  I32 component_count = (I32)(field_size/sizeof(F32));
  glVertexAttribPointer(attrib_ctx->count, 
                        component_count, 
                        GL_FLOAT, 
                        GL_FALSE, 
                        struct_size, 
                        (void *)field_offset);
  glEnableVertexAttribArray(attrib_ctx->count);
  if (attrib_ctx->should_divide)
  {
    glVertexAttribDivisor(attrib_ctx->count, 1); 
  }
  attrib_ctx->count++;
  attrib_ctx->total_size += (U32)field_size;
}

String8_Hash_Set
build_gl_extension_table(Arena *arena, String8 haystack)
{
  String8_Hash_Set table = {0};
  
  //- create the hash table array
  // NOTE(erb): dumb heuristic on how many extension strings there are to create the hash table size
  U32 ext_length_estimate = (U32)S8("_GL_ARB_").size + 15;
  table.slot_count = (U32)(haystack.size / ext_length_estimate); 
  table.slots = PushArray(arena, table.slot_count, String8_Node);
  
  U64 last_delim = 0;
  while (last_delim < haystack.size)
  {
    //- get next space seperator
    U64 next_delim = 0;
    Index_U64 delim_idx = str8_index_start(haystack, ' ', last_delim, 0);
    if (delim_idx.exists)
    {
      next_delim = delim_idx.value;
    }
    else
    {
      break;
    }
    
    String8 ext = str8_slice(haystack, rng_u64(last_delim, next_delim));
    
    U64 idx = str8_hash(ext) % table.slot_count;
    String8_Node *slot = (table.slots + idx);
    
    //- test if extension is allready in table
    B32 exists = false;
    for(;slot; 
        slot = slot->next)
    {
      if(str8_equals(slot->str, ext)) 
      {
        exists = true;
        break;
      }
      if(!slot->next)
      {
        break;
      }
    }
    
    //- add extension if it isn't in the table
    if (!exists)
    {
      //- if the current slot is not empty add a next
      if (slot->str.size != 0)
      {
        slot = slot->next = PushStruct(arena, String8_Node);
      }
      slot->str = ext;
      table.total_count++;
    }
    
    last_delim = (next_delim + 1);
  }
  
  return table;
}

#if defined(_WIN32) || defined(_WIN64)
#include "win32/win32_gl.c"
#elif defined(OS_LINUX)
#include "linux/linux_gl.c"
#endif