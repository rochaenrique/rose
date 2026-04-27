/* date = April 22nd 2026 11:56 am */

#ifndef GL_H
#define GL_H

typedef U8 GLubyte;
typedef U32 GLenum;
typedef F32 GLfloat;
typedef U32 GLbitfield;

#define GL_COLOR_BUFFER_BIT 0x00004000

#define GL_VENDOR     0x1F00
#define GL_RENDERER   0x1F01
#define GL_VERSION    0x1F02
#define GL_EXTENSIONS 0x1F03

#define GL_NO_ERROR                       0
#define GL_INVALID_ENUM                   0x0500
#define GL_INVALID_VALUE                  0x0501
#define GL_INVALID_OPERATION              0x0502
#define GL_STACK_OVERFLOW                 0x0503
#define GL_STACK_UNDERFLOW                0x0504
#define GL_OUT_OF_MEMORY                  0x0505

// NOTE(erb): opengl function types
#define X(NAME, RETURN, PARAMS) typedef RETURN GL_##NAME PARAMS;
#include "gl_functions_x_list.h"
#undef X

// NOTE(erb): opengl function pointers
#define X(NAME, RETURN, PARAMS) global GL_##NAME * NAME = 0;
#include "gl_functions_x_list.h"
#undef X

#endif //GL_H
