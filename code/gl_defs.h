/* date = April 22nd 2026 11:56 am */

#ifndef GL_DEFS_H
#define GL_DEFS_H

typedef F32 GLfloat;
typedef U32 GLbitfield;
#define GL_COLOR_BUFFER_BIT               0x00004000

// NOTE(erb): opengl function types
#define X(NAME, RETURN, PARAMS) typedef RETURN GL_##NAME PARAMS;
#include "gl_funcs.h"
#undef X

// NOTE(erb): opengl function pointers
#define X(NAME, RETURN, PARAMS) global GL_##NAME * gl_##NAME = 0;
#include "gl_funcs.h"
#undef X

#endif //GL_DEFS_H
