/* date = April 22nd 2026 11:56 am */

#ifndef GL_H
#define GL_H

///////////////////////////////////////////
// NOTE(erb): gfx functions to implement

B32  gfx_gl_init();
B32  gfx_gl_equip(GFX_Window window);
void gfx_gl_render_begin(GFX_Window window);
void gfx_gl_render_end(GFX_Window window);
void gfx_gl_attach_begin(void);
void gfx_gl_attach_end(void);

///////////////////////////////////////////
// NOTE(erb): gl api

typedef int GLint;
typedef int GLsizei;
typedef U8 GLubyte;
typedef char GLchar;
typedef unsigned int GLuint;
typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef F32 GLfloat;
typedef U32 GLbitfield;
typedef U64 GLsizeiptr;
typedef U64 GLintptr;
typedef void GLvoid;

#define GL_FALSE              0
#define GL_TRUE               1
#define GL_POINTS             0x0000
#define GL_LINES              0x0001
#define GL_LINE_LOOP          0x0002
#define GL_LINE_STRIP         0x0003
#define GL_TRIANGLES          0x0004
#define GL_TRIANGLE_STRIP     0x0005
#define GL_TRIANGLE_FAN       0x0006

#define GL_TEXTURE_2D                     0x0DE1
#define GL_SCISSOR_TEST                   0x0C11

#define GL_NEAREST                        0x2600
#define GL_LINEAR                         0x2601

#define GL_NEAREST_MIPMAP_NEAREST         0x2700
#define GL_LINEAR_MIPMAP_NEAREST          0x2701
#define GL_NEAREST_MIPMAP_LINEAR          0x2702
#define GL_LINEAR_MIPMAP_LINEAR           0x2703

#define GL_TEXTURE_MAG_FILTER             0x2800
#define GL_TEXTURE_MIN_FILTER             0x2801
#define GL_TEXTURE_WRAP_S                 0x2802
#define GL_TEXTURE_WRAP_T                 0x2803

#define GL_TEXTURE0                       0x84C0
#define GL_TEXTURE1                       0x84C1
#define GL_TEXTURE2                       0x84C2
#define GL_TEXTURE3                       0x84C3
#define GL_TEXTURE4                       0x84C4
#define GL_TEXTURE5                       0x84C5
#define GL_TEXTURE6                       0x84C6
#define GL_TEXTURE7                       0x84C7
#define GL_TEXTURE8                       0x84C8
#define GL_TEXTURE9                       0x84C9
#define GL_TEXTURE10                      0x84CA
#define GL_TEXTURE11                      0x84CB
#define GL_TEXTURE12                      0x84CC
#define GL_TEXTURE13                      0x84CD
#define GL_TEXTURE14                      0x84CE
#define GL_TEXTURE15                      0x84CF
#define GL_TEXTURE16                      0x84D0
#define GL_TEXTURE17                      0x84D1
#define GL_TEXTURE18                      0x84D2
#define GL_TEXTURE19                      0x84D3
#define GL_TEXTURE20                      0x84D4
#define GL_TEXTURE21                      0x84D5
#define GL_TEXTURE22                      0x84D6
#define GL_TEXTURE23                      0x84D7
#define GL_TEXTURE24                      0x84D8
#define GL_TEXTURE25                      0x84D9
#define GL_TEXTURE26                      0x84DA
#define GL_TEXTURE27                      0x84DB
#define GL_TEXTURE28                      0x84DC
#define GL_TEXTURE29                      0x84DD
#define GL_TEXTURE30                      0x84DE
#define GL_TEXTURE31                      0x84DF


#define GL_REPEAT                         0x2901
#define GL_CLAMP_TO_EDGE                  0x812F

#define GL_DEPTH_BUFFER_BIT               0x00000100
#define GL_STENCIL_BUFFER_BIT             0x00000400
#define GL_COLOR_BUFFER_BIT               0x00004000

#define GL_BLEND                          0x0BE2

#define GL_VENDOR             0x1F00
#define GL_RENDERER           0x1F01
#define GL_VERSION            0x1F02
#define GL_EXTENSIONS         0x1F03

#define GL_SHADING_LANGUAGE_VERSION       0x8B8C

#define GL_NO_ERROR           0
#define GL_INVALID_ENUM       0x0500
#define GL_INVALID_VALUE      0x0501
#define GL_INVALID_OPERATION  0x0502
#define GL_STACK_OVERFLOW     0x0503
#define GL_STACK_UNDERFLOW    0x0504
#define GL_OUT_OF_MEMORY      0x0505

#define GL_COMPILE_STATUS     0x8B81
#define GL_LINK_STATUS        0x8B82

#define GL_FRAGMENT_SHADER    0x8B30
#define GL_VERTEX_SHADER      0x8B31

#define GL_ARRAY_BUFFER       0x8892

#define GL_BYTE               0x1400
#define GL_UNSIGNED_BYTE      0x1401
#define GL_SHORT              0x1402
#define GL_UNSIGNED_SHORT     0x1403
#define GL_INT                0x1404
#define GL_UNSIGNED_INT       0x1405
#define GL_FLOAT              0x1406
#define GL_FIXED              0x140C

#define GL_RG                 0x8227

#define GL_DEPTH_COMPONENT    0x1902
#define GL_RED                0x1903
#define GL_GREEN              0x1904
#define GL_BLUE               0x1905
#define GL_ALPHA              0x1906
#define GL_RGB                0x1907
#define GL_RGBA               0x1908

#define GL_TEXTURE_SWIZZLE_R              0x8E42
#define GL_TEXTURE_SWIZZLE_G              0x8E43
#define GL_TEXTURE_SWIZZLE_B              0x8E44
#define GL_TEXTURE_SWIZZLE_A              0x8E45

#define GL_STREAM_DRAW        0x88E0
#define GL_STATIC_DRAW        0x88E4
#define GL_DYNAMIC_DRAW       0x88E8

#define GL_ZERO                           0
#define GL_ONE                            1
#define GL_SRC_COLOR                      0x0300
#define GL_ONE_MINUS_SRC_COLOR            0x0301
#define GL_SRC_ALPHA                      0x0302
#define GL_ONE_MINUS_SRC_ALPHA            0x0303
#define GL_DST_ALPHA                      0x0304
#define GL_ONE_MINUS_DST_ALPHA            0x0305


// NOTE(erb): opengl function types
#define X(NAME, RETURN, PARAMS) typedef RETURN GL_##NAME PARAMS;
#include "gl_modern_functions_x_list.h"
#include "gl_core_functions_x_list.h"
#undef X

// NOTE(erb): opengl function pointers
#define X(NAME, RETURN, PARAMS) global GL_##NAME * NAME = 0;
#include "gl_modern_functions_x_list.h"
#include "gl_core_functions_x_list.h"
#undef X

///////////////////////////////////////////
// NOTE(erb): common gl utilities

typedef struct Buffer_Attribs Buffer_Attribs;
struct Buffer_Attribs
{
  U32 count;
  U32 total_size;
  B32 should_divide;
};

typedef struct Shader_Param Shader_Param;
struct Shader_Param
{
  String8 path;
  String8 source;
  GLenum type;
};

#define GLCheckError(call_str) \
do \
{ \
I32 error = gl_poll_error(); \
if (error) \
{ \
os_logf("%s:%d: [GL CALL ERROR] (0x%x) : \"%s\" : %s : %s", __FILE__, __LINE__, error, call_str, \
gl_error_code_cstr(error), gl_error_code_description(error)); \
Assert(false);\
} \
} while (0)

#define GL(call) \
do \
{ \
gl_clear_error(); \
call; \
GLCheckError(#call); \
} \
while (0)


#if defined(OS_WIN32)
#include "win32/win32_gl.h"
#elif defined(OS_LINUX)
#include "linux/linux_gl.h"
#endif

#endif //GL_H
