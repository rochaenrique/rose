// https://github.com/gamedevtech/X11OpenGLWindow

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>

#include <stdio.h>
#undef Assert
#include "../base/base.h"
#include "../os/os.h"

#include "../base/base.c"
#include "../os/os.c"


typedef GLXContext glXCreateContextAttribsARB_f(Display *dpy, GLXFBConfig config,
                                                GLXContext share_context, Bool direct,
                                                const int *attrib_list);

int 
main() 
{
  
  B32 os_initialized = os_init();
  if (!os_initialized)
  {
    printf("failed to init os layer: %.*s\n", S8Expand(err_get_last_error()));
    return 1;
  }
  Thread_Context tctx = {0};
  thread_context_init(&tctx);
  os_thread_context_set(&tctx);
  
	//- open display
	Display *display = XOpenDisplay(0);
	if (display == 0) 
  {
    printf("Unable to get X11 display\n");
    return 1;
	}
  
  //- get screen
	Screen *screen = DefaultScreenOfDisplay(display);
  if (screen == 0) 
  {
    printf("Failed to get default screen of display\n");
    return 1;
  }
  I32 screen_id = DefaultScreen(display);
  
  printf("display    = %p\n", display);
  printf("screen     = %p\n", screen);
  printf("screen_id  = %d\n", screen_id);
  
  //- check opengl versions
  {
    GLint major_glx = 0;
    GLint minor_glx = 0;
    Bool got_version = glXQueryVersion(display, &major_glx, &minor_glx);
    if (!got_version)
    {
      printf("Failed to query GLX Version\n");
      return 1;
    }
    printf("GLX version          : %d.%d\n", major_glx, minor_glx);
    printf("GLX client version   : %s\n", glXGetClientString(display, GLX_VERSION));
    printf("GLX client vendor    : %s\n", glXGetClientString(display, GLX_VENDOR));
    printf("GLX client extensions: %s\n", glXGetClientString(display, GLX_EXTENSIONS));
    printf("GLX server version   : %s\n", glXQueryServerString(display, screen_id, GLX_VERSION));
    printf("GLX server vendor    : %s\n", glXQueryServerString(display, screen_id, GLX_VENDOR));
    printf("GLX server extensions: %s\n", glXQueryServerString(display, screen_id, GLX_EXTENSIONS));
  }
  
  XVisualInfo *gl_visual = 0;
  GLXFBConfig frame_buffer_config = {0};
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
    GLXFBConfig *frame_buffer_config_array = glXChooseFBConfig(display, screen_id, glx_attribs, &frame_buffer_count);
    if (frame_buffer_count == 0 || frame_buffer_config_array == 0)
    {
      printf("Failed to get frame buffer count\n");
      XCloseDisplay(display);
      return 1;
    }
    
    //- pick best frame buffer config
    I32 best_samples_count = -1;
    I32 best_frame_buffer_config_idx = -1;
    for (I32 fb_idx = 0; 
         fb_idx < frame_buffer_count;
         ++fb_idx) 
    {
      GLXFBConfig config = frame_buffer_config_array[fb_idx];
      XVisualInfo *visual = glXGetVisualFromFBConfig(display, config);
      if (visual != 0) 
      {
        
        I32 sample_buffers = 0;
        B32 got_sample_buffers = glXGetFBConfigAttrib(display, config, GLX_SAMPLE_BUFFERS, &sample_buffers) == Success;
        I32 samples = 0;
        B32 got_samples = glXGetFBConfigAttrib(display, config, GLX_SAMPLES, &sample_buffers) == Success;
        
        if (got_sample_buffers && got_samples) 
        {
          if (samples > best_samples_count) 
          {
            best_samples_count = samples;
            best_frame_buffer_config_idx = fb_idx;
          }
        }
      }
      XFree(visual);
    }
    if (best_frame_buffer_config_idx < 0) 
    {
      printf("Failed to get vaild frame buffer config");
      XCloseDisplay(display);
      return 1;
    }
    frame_buffer_config = frame_buffer_config_array[best_frame_buffer_config_idx];
    XFree(frame_buffer_config_array);
    
    gl_visual = glXGetVisualFromFBConfig(display, frame_buffer_config);
    if (gl_visual == 0) 
    {
      printf("Failed to choose glx visual\n");
      XCloseDisplay(display);
      return 1;
    }
    Assert(screen_id == gl_visual->screen);
  }
  
	//- open window
  XSetWindowAttributes window_attributes = {0};
  Window window = {0};
  {
    //- attributes
    window_attributes.border_pixel = BlackPixel(display, screen_id);
    window_attributes.background_pixel = WhitePixel(display, screen_id);
    window_attributes.override_redirect = True;
    window_attributes.colormap = XCreateColormap(display, RootWindow(display, screen_id), 
                                                 gl_visual->visual, AllocNone);
    window_attributes.event_mask = ExposureMask;
    U64 value_mask = CWBackPixel|CWColormap|CWBorderPixel|CWEventMask;
    window = XCreateWindow(display, RootWindow(display, screen_id), 
                           0, 0, 960, 540, 1,  //x, y, width, height, border_width
                           gl_visual->depth, InputOutput, gl_visual->visual,
                           value_mask, &window_attributes);
  }
  
  //- redirect window close event
  Atom atom_delete_window = {0};
  {
    Atom atom_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &atom_delete_window, 1);
  }
  
  //- create glx context
  GLXContext glx_context = 0;
  {
    //- load ARB function
    glXCreateContextAttribsARB_f *glXCreateContextAttribsARB = (glXCreateContextAttribsARB_f *)glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");
    if (glXCreateContextAttribsARB == 0) 
    {
      printf("Failed to get glXCreateContextAttribsARB() function");
      XCloseDisplay(display);
      return 1;
    }
    
    int context_attribs[] = 
    {
      GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
      GLX_CONTEXT_MINOR_VERSION_ARB, 2,
      GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
      None,
    };
    
    //- parse the extension table
    const char *glx_extension_string = glXQueryExtensionsString(display, screen_id);
    String8 extension_str = S8CStr((char *)glx_extension_string);
    if (str8_find(extension_str, S8("GLX_ARB_create_context"))) 
    {
      //- actually create the context
      glx_context = glXCreateContextAttribsARB(display, frame_buffer_config, 0, true, context_attribs);
    }
    else
    {
      printf("GLX_ARB_create_context extension not enabled\n");
      return 1;
    }
  }
  
  //- make context current
  B32 made_current = glXMakeCurrent(display, window, glx_context);
  if (!made_current)
  {
    printf("failed to make glx context current");
    return 1;
  }
  
  //- gl strings
  {
    printf("GL Vendor          : %s\n", glGetString(GL_VENDOR));
    printf("GL Renderer        : %s\n", glGetString(GL_RENDERER));
    printf("GL Version         : %s\n", glGetString(GL_VERSION));
    printf("GL Shading Language: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
  }
  
  //- show window
  XClearWindow(display, window);
  XMapRaised(display, window);
  
  //- main loop
  for (;;)
  {
    //- poll events
    B32 should_stop = false;
    while (XPending(display) > 0) 
    {
      XEvent event;
      XNextEvent(display, &event);
      switch (event.type) 
      {
        case Expose: 
        {
          // NOTE(erb): resize event
        } break;
        
        case DestroyNotify: 
        {
          should_stop = true;
        } break;
        
        case ClientMessage: 
        {
          if (event.xclient.data.l[0] == (long)atom_delete_window) 
          {
            should_stop = true;
          }
        } break;
      }
    }
    
    if (should_stop) 
    {
      break;
    }
    
    //- render
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    //- swap windows
    glXSwapBuffers(display, window);
  }
  
  //- cleanup glx related
  glXDestroyContext(display, glx_context);
  XFree(gl_visual);
  XFreeColormap(display, window_attributes.colormap);
  
  //- cleanup
  XDestroyWindow(display, window);
  XFree(screen);
  XCloseDisplay(display);
  return 0;
}