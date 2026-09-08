#include <dlfcn.h> //open()

global LNX_GL_GLX_State lnx_gl_glx_state;

#define lnx_gl_gfx_handle_from_slot(slot) (GFX_Window)(((slot) - lnx_gl_window_slots) + 1)

#define GLX_GET_PROC_ADDRESS(VALUE, TYPE, STR) VALUE = (TYPE *)lnx_glXGetProcAddressARB((const GLubyte *)(STR))

B32
gfx_gl_init()
{
  String8 error = {0};
  
  void *gl_module = 0;
  {
    gl_module = dlopen("libGL.so", RTLD_NOW);
    if (gl_module == 0)
    {
      error = S8("failed to load libGL.so");
    }
  }
  
  //- load glx functions
  if (error.value == 0)
  {
#define X(NAME, RETURN, PARAMS) lnx_##NAME = dlsym(gl_module, #NAME);
#include "linux_glx_functions_x_list.h"
#undef X
    
#define X(NAME, RETURN, PARAMS) if (lnx_##NAME == 0) { error = S8("missing glx function"); goto end_glx_functions; }
#include "linux_glx_functions_x_list.h"
#undef X
    end_glx_functions:;
  }
  
#if DEBUG
  GLint major_version = 0;
  GLint minor_version = 0;
  if (error.value == 0)
  {
    Bool got_version = lnx_glXQueryVersion(lnx_gfx_state.display, &major_version, &minor_version);
    if (got_version)
    {
      Display *d = lnx_gfx_state.display;
      I32 s = lnx_gfx_state.screen_id;
      
      os_logf("GLX version          : %d.%d", major_version, minor_version);
      os_logf("GLX client version   : %s", lnx_glXGetClientString(d, GLX_VERSION));
      os_logf("GLX client vendor    : %s", lnx_glXGetClientString(d, GLX_VENDOR));
      os_logf("GLX client extensions: %s", lnx_glXGetClientString(d, GLX_EXTENSIONS));
      os_logf("GLX server version   : %s", lnx_glXQueryServerString(d, s, GLX_VERSION));
      os_logf("GLX server vendor    : %s", lnx_glXQueryServerString(d, s, GLX_VENDOR));
      os_logf("GLX server extensions: %s", lnx_glXQueryServerString(d, s, GLX_EXTENSIONS));
    }
    else
    {
      error = S8("failed to query GLX version");
    }
  }
#endif
  
  GLXFBConfig frame_buffer_config = {0};
  if (error.value == 0)
  {
    GLint glx_attribs[] = 
    {
      GLX_X_RENDERABLE , True,
      GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
      GLX_RENDER_TYPE  , GLX_RGBA_BIT,
      GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
      GLX_RED_SIZE     , 8,
      GLX_GREEN_SIZE   , 8,
      GLX_BLUE_SIZE    , 8,
      GLX_ALPHA_SIZE   , 8,
      GLX_DEPTH_SIZE   , 24,
      GLX_STENCIL_SIZE , 8,
      GLX_DOUBLEBUFFER , True,
      None
    };
    
    //- frame buffer configs
    I32 frame_buffer_count = 0;
    GLXFBConfig *frame_buffer_config_array = lnx_glXChooseFBConfig(lnx_gfx_state.display, lnx_gfx_state.screen_id, 
                                                                   glx_attribs, &frame_buffer_count);
    if (frame_buffer_count == 0 || frame_buffer_config_array == 0)
    {
      error = S8("failed to get frame buffer configs");
    }
    
    //- pick best frame buffer config
    if (error.value == 0)
    {
      I32 best_samples_count = -1;
      I32 best_frame_buffer_config_idx = -1;
      for (I32 fb_idx = 0; 
           fb_idx < frame_buffer_count;
           ++fb_idx) 
      {
        GLXFBConfig config = frame_buffer_config_array[fb_idx];
        XVisualInfo *visual = lnx_glXGetVisualFromFBConfig(lnx_gfx_state.display, config);
        if (visual != 0) 
        {
          I32 sample_buffers = 0;
          B32 got_sample_buffers = lnx_glXGetFBConfigAttrib(lnx_gfx_state.display, config, 
                                                            GLX_SAMPLE_BUFFERS, &sample_buffers) == Success;
          I32 samples = 0;
          B32 got_samples = lnx_glXGetFBConfigAttrib(lnx_gfx_state.display, config, 
                                                     GLX_SAMPLES, &sample_buffers) == Success;
          
          if (got_sample_buffers && got_samples &&
              samples > best_samples_count) 
          {
            best_samples_count = samples;
            best_frame_buffer_config_idx = fb_idx;
          }
        }
        XFree(visual);
      }
      if (best_frame_buffer_config_idx < 0) 
      {
        error = S8("failed to get vaild frame buffer config");
      }
      else 
      {
        frame_buffer_config = frame_buffer_config_array[best_frame_buffer_config_idx];
      }
    }
    if (frame_buffer_config_array != 0) 
    {
      XFree(frame_buffer_config_array);
    }
  }
  
  XVisualInfo *visual = 0;
  if (error.value == 0)
  {
    visual = lnx_glXGetVisualFromFBConfig(lnx_gfx_state.display, frame_buffer_config);
    if (visual == 0) 
    {
      error = S8("failed to choose glx visual");
    }
    if (lnx_gfx_state.screen_id != visual->screen) 
    {
      error = S8("visual screen id does not match x11 screen id");
    }
  }
  
  
  //- get glx extensions
  if (error.value == 0)
  {
    //- load functions
#define X(NAME, RETURN, PARAMS) GLX_GET_PROC_ADDRESS(lnx_##NAME, LNX_##NAME, #NAME);
#include "linux_glx_extensions_functions_x_list.h"
#undef X
    
    //- parse the extension table
    const char *glx_extension_string = lnx_glXQueryExtensionsString(lnx_gfx_state.display, lnx_gfx_state.screen_id);
    String8 extension_str = S8CStr((char *)glx_extension_string);
    
    // TODO(erb): handle not having modern opengl create ARB function
    if (str8_find(extension_str, S8("GLX_ARB_create_context"))) 
    {
      if (lnx_glXCreateContextAttribsARB == 0) 
      {
        error = S8("failed to get glXCreateContextAttribsARB() function");
      }
    }
    else
    {
      error = S8("GLX_ARB_create_context extension not enabled\n");
    }
    
    //- apply swap control extension (if we have it)
    if (str8_find(extension_str, S8("GLX_EXT_swap_control")) && lnx_glXSwapIntervalEXT != 0)
    {
      lnx_glXSwapIntervalEXT(lnx_gfx_state.display, None, 1);
    }
  }
  
  //- create the global context
  GLXContext glx_context = {0};
  if (error.value == 0)
  {
    int context_attribs[] = 
    {
      GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
      GLX_CONTEXT_MINOR_VERSION_ARB, 2,
      GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
      None,
    };
    glx_context = lnx_glXCreateContextAttribsARB(lnx_gfx_state.display, 
                                                 frame_buffer_config, 
                                                 0, true, context_attribs);
    if (glx_context == 0) 
    {
      error = S8("failed to create glx context");
    }
  }
  
  //- set the context 
  if (error.value == 0)
  {
    B32 made_current = lnx_glXMakeCurrent(lnx_gfx_state.display, None, glx_context);
    if (!made_current)
    {
      error = S8("failed to make glx context current");
    }
  }
  
  //- load the modern regular gl and modern gl functions
  if (error.value == 0)
  {
#define X(NAME, RETURN, PARAMS) GLX_GET_PROC_ADDRESS(NAME, GL_##NAME, #NAME);
#include "gfx/gl/gl_modern_functions_x_list.h"
#include "gfx/gl/gl_core_functions_x_list.h"
#undef X
    
#define X(NAME, RETURN, PARAMS) if (NAME == 0) { error = S8("missing gl function " #NAME); goto end_gl_functions; }
#include "gfx/gl/gl_modern_functions_x_list.h"
#include "gfx/gl/gl_core_functions_x_list.h"
#undef X
    end_gl_functions:;
  }
  
#if DEBUG
  //- gl strings
  if (error.value == 0)
  {
    os_logf("GL Vendor          : %s", glGetString(GL_VENDOR));
    os_logf("GL Renderer        : %s", glGetString(GL_RENDERER));
    os_logf("GL Version         : %s", glGetString(GL_VERSION));
    os_logf("GL Shading Language: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
  }
#endif
  
  B32 result = false;
  if (error.value == 0) 
  {
    ZeroStruct(&lnx_gl_glx_state);
    lnx_gl_glx_state.frame_buffer_config = frame_buffer_config;
    lnx_gl_glx_state.glx_context = glx_context;
    lnx_gl_glx_state.gl_module = gl_module;
    result = true;
    
    // NOTE(erb): ISSUE: overwriting gfx layer from the equip layer is BAD because it allows only one equiper.
    // TODO(erb): have a cleaner way to set the overwrite the visual
    lnx_gfx_state.visual_info = *visual;
  }
  else
  {
    if(gl_module)
    {
      dlclose(gl_module);
    }
    if (visual) 
    {
      XFree(visual);
    }
    err_set_error(error);
  }
  return(result);
}

B32
gfx_gl_equip(GFX_Window handle)
{
  String8 error = {0};
  
  LNX_Window *window_slot = 0;
  Window x11_window = {0};
  if (gfx_window_is_valid(handle))
  {
    window_slot = lnx_slot_from_gfx_handle(handle);
    x11_window = window_slot->x11_window;
  }
  else
  {
    error = S8("windows window is not valid");
  }
  
  //- make context current
  if (error.value == 0) 
  {
    B32 made_current = lnx_glXMakeCurrent(lnx_gfx_state.display, x11_window, 
                                          lnx_gl_glx_state.glx_context);
    if (!made_current)
    {
      error = S8("failed to make glx context current");
    }
  }
  
  if (error.value != 0)
  {
    lnx_glXMakeCurrent(lnx_gfx_state.display, 0, 0);
    err_set_error(error);
  }
  B32 result = (error.value == 0);
  return(result);
}

void 
gfx_gl_attach_begin(void)
{
  B32 made_current = lnx_glXMakeCurrent(lnx_gfx_state.display, None, lnx_gl_glx_state.glx_context);
  Assert(made_current);
}

void 
gfx_gl_attach_end(void)
{
  B32 made_current = lnx_glXMakeCurrent(lnx_gfx_state.display, None, 0);
  Assert(made_current);
}

void
gfx_gl_render_begin(GFX_Window handle)
{
  if (gfx_window_is_valid(handle)) 
  {
    LNX_Window *slot = lnx_slot_from_gfx_handle(handle);
    lnx_glXMakeCurrent(lnx_gfx_state.display, slot->x11_window, 
                       lnx_gl_glx_state.glx_context);
  }
}

void
gfx_gl_render_end(GFX_Window handle)
{
  if (gfx_window_is_valid(handle)) 
  {
    LNX_Window *slot = lnx_slot_from_gfx_handle(handle);
    lnx_glXSwapBuffers(lnx_gfx_state.display, slot->x11_window);
  }
}

