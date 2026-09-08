/* date = May 28th 2026 9:54 pm */

#ifndef GL_Q2D_H
#define GL_Q2D_H

typedef struct GL_Renderer GL_Renderer;
struct GL_Renderer
{
  GLuint vao;
  GLuint vbo;
  GLuint program;
  
  GLint resolution_uniform;
  
  V2i viewport_size;
};

#define GL_Q2D_VERTEX_SHADER_FILE_NAME "q2d_vertex_shader.glsl"
#define GL_Q2D_FRAGMENT_SHADER_FILE_NAME "q2d_fragment_shader.glsl"

#endif //GL_Q2D_H
