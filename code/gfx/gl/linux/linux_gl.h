/* date = May 18th 2026 10:11 pm */

#ifndef LINUX_GL_H
#define LINUX_GL_H

#define GL_GLEXT_PROTOTYPES 0

///////////////////////////////////////////////////
// NOTE(erb): glx.h definitions
#define GLX_VENDOR     1
#define GLX_VERSION    2
#define GLX_EXTENSIONS 3

#define GLX_DOUBLEBUFFER        5
#define GLX_RED_SIZE            8
#define GLX_GREEN_SIZE          9
#define GLX_BLUE_SIZE           10
#define GLX_ALPHA_SIZE          11
#define GLX_DEPTH_SIZE          12
#define GLX_STENCIL_SIZE        13

#define GLX_X_VISUAL_TYPE               0x22
#define GLX_RGBA_BIT                    0x00000001
#define GLX_WINDOW_BIT                  0x00000001
#define GLX_TRUE_COLOR                  0x8002
#define GLX_DRAWABLE_TYPE               0x8010
#define GLX_RENDER_TYPE                 0x8011
#define GLX_X_RENDERABLE                0x8012

#define GLX_SAMPLES                     0x186a1 /*100001*/
#define GLX_SAMPLE_BUFFERS              0x186a0 /*100000*/

typedef struct __GLXcontextRec *GLXContext;
typedef struct __GLXFBConfigRec *GLXFBConfig;
typedef void (*__GLXextFuncPtr)(void);
typedef XID GLXDrawable;

// NOTE(erb): glx function types
#define X(NAME, RETURN, PARAMS) typedef RETURN LNX_##NAME PARAMS;
#include "linux_glx_functions_x_list.h"
#undef X

// NOTE(erb): glx function pointers
#define X(NAME, RETURN, PARAMS) global LNX_##NAME * lnx_##NAME = 0;
#include "linux_glx_functions_x_list.h"
#undef X

///////////////////////////////////////////////////

///////////////////////////////////////////////////
// NOTE(erb): glxext.h definitions
#define GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define GLX_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB     0x2092
#define GLX_CONTEXT_FLAGS_ARB             0x2094

///////////////////////////////////////////////////

// NOTE(erb): dump name just so there is no confusion with LNX_GLX_State and LNX_GFX_State
typedef struct LNX_GL_GLX_State LNX_GL_GLX_State;
struct LNX_GL_GLX_State
{
  GLXFBConfig frame_buffer_config;
  GLXContext glx_context;
  void *gl_module;
};

// NOTE(erb): glx extension function types
#define X(NAME, RETURN, PARAMS) typedef RETURN LNX_##NAME PARAMS;
#include "linux_glx_extensions_functions_x_list.h"
#undef X

// NOTE(erb): glx extension functions
#define X(NAME, RETURN, PARAMS) global LNX_##NAME * lnx_##NAME = 0;
#include "linux_glx_extensions_functions_x_list.h"
#undef X

#endif //LINUX_GL_H
