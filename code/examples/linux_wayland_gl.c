#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <signal.h>

#include <linux/input.h>

#include <wayland-client.h>
#include <wayland-egl.h>
#include <wayland-cursor.h>

#include <GLES2/gl2.h>
#include <EGL/egl.h>

#undef Assert
#include "../base/base.h"
#include "../os/os.h"

#include "../base/base.c"
#include "../os/os.c"

typedef struct WL_GL_Data
{
  struct wl_compositor *compositor;
  struct wl_shell *shell;
  struct wl_shell_surface *shell_surface;
  struct wl_seat *seat;
  struct wl_shm *shm;
  struct wl_surface *surface;
  
  struct wl_cursor *default_cursor;
  struct wl_cursor_theme *cursor_theme;
  struct wl_surface *cursor_surface;
  struct wl_pointer *pointer;
	struct wl_keyboard *keyboard;
  
  struct wl_callback *callback;
  int configured;
  
  int width;
  int height;
  int running;
  
  GLuint fragment_shader;
  GLuint vertex_shader;
  GLuint program;
  int pos;
  int col;
  int rotation_uniform;
  
  EGLDisplay egl_display;
  EGLSurface egl_surface;
} WL_GL_Data;

const char *vert_shader_text =
"uniform mat4 rotation;\n"
"attribute vec4 pos;\n"
"attribute vec4 color;\n"
"varying vec4 v_color;\n"
"void main() {\n"
"  gl_Position = rotation * pos;\n"
"  v_color = color;\n"
"}\n";

const char *frag_shader_text =
"precision mediump float;\n"
"varying vec4 v_color;\n"
"void main() {\n"
"  gl_FragColor = v_color;\n"
"}\n";

void
handle_ping(void *data, struct wl_shell_surface *shell_surface, 
            uint32_t serial)
{
  SupressUnused(data);
  wl_shell_surface_pong(shell_surface, serial);
}

void
handle_configure(void *data, struct wl_shell_surface *shell_surface,
                 uint32_t edges, int32_t width, int32_t height)
{
  SupressUnused(shell_surface);
  SupressUnused(edges);
  
  WL_GL_Data *user = (WL_GL_Data *)data;
  user->width = width;
  user->height = height;
}

void
handle_popup_done(void *data, struct wl_shell_surface *shell_surface)
{
  SupressUnused(data);
  SupressUnused(shell_surface);
}

struct wl_shell_surface_listener shell_surface_listener = 
{
	.ping = handle_ping,
	.configure = handle_configure,
	.popup_done = handle_popup_done,
};

void
keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard,
                       uint32_t format, int fd, uint32_t size)
{
  SupressUnused(data);
  SupressUnused(keyboard);
  SupressUnused(format);
  SupressUnused(fd);
  SupressUnused(size);
}

void
keyboard_handle_enter(void *data, struct wl_keyboard *keyboard,
                      uint32_t serial, struct wl_surface *surface,
                      struct wl_array *keys)
{
  SupressUnused(data);
  SupressUnused(keyboard);
  SupressUnused(serial);
  SupressUnused(surface);
  SupressUnused(keys);
}

void
keyboard_handle_leave(void *data, struct wl_keyboard *keyboard,
                      uint32_t serial, struct wl_surface *surface)
{
  SupressUnused(data);
  SupressUnused(keyboard);
  SupressUnused(serial);
  SupressUnused(surface);
}

void
keyboard_handle_key(void *data, struct wl_keyboard *keyboard,
                    uint32_t serial, uint32_t time, uint32_t key,
                    uint32_t state)
{
  SupressUnused(keyboard);
  SupressUnused(serial);
  SupressUnused(time);
  
  WL_GL_Data *user = (WL_GL_Data *)data;
  if (key == KEY_ESC && state)
  {
    user->running = 0;
  }
}

void
keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard,
                          uint32_t serial, uint32_t mods_depressed,
                          uint32_t mods_latched, uint32_t mods_locked,
                          uint32_t group)
{
  SupressUnused(data);
  SupressUnused(keyboard);
  SupressUnused(serial);
  SupressUnused(mods_depressed);
  SupressUnused(mods_latched);
  SupressUnused(mods_locked);
  SupressUnused(group);
}

void keyboard_handle_repeat(void* data, struct wl_keyboard* wl_keyboard, 
                            int rate, int delay)
{
  SupressUnused(data);
  SupressUnused(wl_keyboard);
  SupressUnused(rate);
  SupressUnused(delay);
}

struct wl_keyboard_listener keyboard_listener = 
{
	.keymap = keyboard_handle_keymap,
	.enter = keyboard_handle_enter,
	.leave = keyboard_handle_leave,
	.key = keyboard_handle_key,
	.modifiers = keyboard_handle_modifiers,
  .repeat_info = keyboard_handle_repeat,
};


void
pointer_handle_enter(void *data, struct wl_pointer *pointer,
                     uint32_t serial, struct wl_surface *surface,
                     wl_fixed_t sx, wl_fixed_t sy)
{
  SupressUnused(surface);
  SupressUnused(sx);
  SupressUnused(sy);
  
  WL_GL_Data *user = (WL_GL_Data *)data;
  if (user->default_cursor) 
  {
    struct wl_cursor_image *image = user->default_cursor->images[0];
    struct wl_buffer *buffer = wl_cursor_image_get_buffer(image);
    
    wl_pointer_set_cursor(pointer, serial,
                          user->cursor_surface,
                          image->hotspot_x,
                          image->hotspot_y);
    
    wl_surface_attach(user->cursor_surface, buffer, 0, 0);
    wl_surface_damage(user->cursor_surface, 0, 0, image->width, image->height);
    wl_surface_commit(user->cursor_surface);
  }
}

void
pointer_handle_leave(void *data, struct wl_pointer *pointer,
                     uint32_t serial, struct wl_surface *surface)
{
  SupressUnused(data);
  SupressUnused(pointer);
  SupressUnused(pointer);
  SupressUnused(serial);
  SupressUnused(surface);
}

void
pointer_handle_motion(void *data, struct wl_pointer *pointer,
                      uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
  SupressUnused(data);
  SupressUnused(pointer);
  SupressUnused(time);
  SupressUnused(sx);
  SupressUnused(sy);
}

void
pointer_handle_button(void *data, struct wl_pointer *pointer,
                      uint32_t serial, uint32_t time, uint32_t button,
                      uint32_t state)
{
  SupressUnused(data);
  SupressUnused(pointer);
  SupressUnused(time);
  
  WL_GL_Data *user = (WL_GL_Data *)data;
  if (button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_PRESSED)
  {
    wl_shell_surface_move(user->shell_surface, user->seat, serial);
  }
}

void
pointer_handle_axis(void *data, struct wl_pointer *wl_pointer,
                    uint32_t time, uint32_t axis, wl_fixed_t value)
{
  SupressUnused(data);
  SupressUnused(wl_pointer);
  SupressUnused(time);
  SupressUnused(axis);
  SupressUnused(value);
}


struct wl_pointer_listener pointer_listener = {
  .enter = pointer_handle_enter,
  .leave = pointer_handle_leave,
  .motion = pointer_handle_motion,
  .button = pointer_handle_button,
  .axis = pointer_handle_axis,
};

static void
seat_handle_capabilities(void *data, struct wl_seat *seat,
                         enum wl_seat_capability caps)
{
  WL_GL_Data *user = (WL_GL_Data *)data;
  
  if ((caps & WL_SEAT_CAPABILITY_POINTER) && !user->pointer) 
  {
    user->pointer = wl_seat_get_pointer(seat);
    wl_pointer_add_listener(user->pointer, &pointer_listener, user);
  } 
  else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && user->pointer) 
  {
    wl_pointer_destroy(user->pointer);
    user->pointer = 0;
  }
  
  if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !user->keyboard) 
  {
    user->keyboard = wl_seat_get_keyboard(seat);
    wl_keyboard_add_listener(user->keyboard, &keyboard_listener, user);
  } 
  else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && user->keyboard) 
  {
    wl_keyboard_destroy(user->keyboard);
    user->keyboard = NULL;
  }
}

struct wl_seat_listener seat_listener = 
{
  .capabilities = seat_handle_capabilities,
};

void
registry_handle_global(void *data, struct wl_registry *registry,
                       uint32_t name, const char *interface, uint32_t version)
{
  SupressUnused(name);
  SupressUnused(version);
  
  WL_GL_Data *user = (WL_GL_Data *)data;
  
  if (strcmp(interface, "wl_compositor") == 0) 
  {
    user->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
  }
  else if (strcmp(interface, "wl_shell") == 0) 
  {
    user->shell = wl_registry_bind(registry, name, &wl_shell_interface, 1);
  }
  else if (strcmp(interface, "wl_seat") == 0) 
  {
    user->seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
    wl_seat_add_listener(user->seat, &seat_listener, user);
  } 
  else if (strcmp(interface, "wl_shm") == 0) 
  {
    user->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    user->cursor_theme = wl_cursor_theme_load(0, 32, user->shm);
    user->default_cursor = wl_cursor_theme_get_cursor(user->cursor_theme, "left_ptr");
  }
}

void
registry_handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
  SupressUnused(data);
  SupressUnused(registry);
  SupressUnused(name);
}

struct wl_registry_listener registry_listener = 
{
  registry_handle_global,
  .global_remove = registry_handle_global_remove,
};

WL_GL_Data global_data = {0};

GLuint
create_shader(const char *source, GLenum shader_type)
{
  GLuint shader = glCreateShader(shader_type);
  if (!shader)
  {
    printf("failed glCreateShader");
    exit(1);
  }
  
  glShaderSource(shader, 1, (const char **) &source, NULL);
  glCompileShader(shader);
  
  GLint status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (!status) {
    char log[1000];
    GLsizei len;
    glGetShaderInfoLog(shader, 1000, &len, log);
    fprintf(stderr, "Error: compiling %s: %*s\n",
            shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment",
            len, log);
    exit(1);
  }
  
  return shader;
}


char *
egl_error_string(int error)
{
#define Case(err) case err: return #err
  
  switch (error)
  {
    Case(EGL_SUCCESS);
    Case(EGL_NOT_INITIALIZED);
    Case(EGL_BAD_ACCESS);
    Case(EGL_BAD_ALLOC);
    Case(EGL_BAD_ATTRIBUTE);
    Case(EGL_BAD_CONTEXT);
    Case(EGL_BAD_CONFIG);
    Case(EGL_BAD_CURRENT_SURFACE);
    Case(EGL_BAD_DISPLAY);
    Case(EGL_BAD_SURFACE);
    Case(EGL_BAD_MATCH);
    Case(EGL_BAD_PARAMETER);
    Case(EGL_BAD_NATIVE_PIXMAP);
    Case(EGL_BAD_NATIVE_WINDOW);
    Case(EGL_CONTEXT_LOST);
  }
  return("unkown error");
  
#undef Case
}

void redraw(void *data, struct wl_callback *callback, uint32_t time);

void
configure_callback(void *data, struct wl_callback *callback, uint32_t time)
{
  WL_GL_Data *user = (WL_GL_Data *)data;
  
  wl_callback_destroy(callback);
  
  user->configured = 1;
  
  if (user->callback == 0)
  {
    redraw(data, 0, time);
  }
}

struct wl_callback_listener frame_listener = {
  redraw
};

void
redraw(void *data, struct wl_callback *callback, uint32_t time)
{
  WL_GL_Data *user = (WL_GL_Data *)data;
  
  static const GLfloat verts[3][2] = {
    { -0.5, -0.5 },
    {  0.5, -0.5 },
    {  0,    0.5 }
  };
  static const GLfloat colors[3][3] = {
    { 1, 0, 0 },
    { 0, 1, 0 },
    { 0, 0, 1 }
  };
  GLfloat angle;
  GLfloat rotation[4][4] = {
    { 1, 0, 0, 0 },
    { 0, 1, 0, 0 },
    { 0, 0, 1, 0 },
    { 0, 0, 0, 1 }
  };
  static const int32_t speed_div = 5;
  static uint32_t start_time = 0;
  
  Assert(user->callback == callback);
  user->callback = NULL;
  
  if (callback)
  {
    wl_callback_destroy(callback);
  }
  
  if (!user->configured)
  {
    return;
  }
  
  if (start_time == 0)
  {
    start_time = time;
  }
  
  angle = ((time-start_time) / speed_div) % 360 * M_PI / 180.0;
  rotation[0][0] =  cos(angle);
  rotation[0][2] =  sin(angle);
  rotation[2][0] = -sin(angle);
  rotation[2][2] =  cos(angle);
  
  glViewport(0, 0, user->width, user->height);
  
  glUniformMatrix4fv(user->rotation_uniform, 1, GL_FALSE,
                     (GLfloat *) rotation);
  
  glClearColor(0.0, 0.0, 0.0, 0.5);
  glClear(GL_COLOR_BUFFER_BIT);
  
  glVertexAttribPointer(user->pos, 2, GL_FLOAT, GL_FALSE, 0, verts);
  glVertexAttribPointer(user->col, 3, GL_FLOAT, GL_FALSE, 0, colors);
  glEnableVertexAttribArray(user->pos);
  glEnableVertexAttribArray(user->col);
  
  glDrawArrays(GL_TRIANGLES, 0, 3);
  
  glDisableVertexAttribArray(user->pos);
  glDisableVertexAttribArray(user->col);
  
  wl_surface_set_opaque_region(user->surface, NULL);
  
  user->callback = wl_surface_frame(user->surface);
  wl_callback_add_listener(user->callback, &frame_listener, user);
  
  eglSwapBuffers(user->egl_display, user->egl_surface);
}

struct wl_callback_listener configure_callback_listener = 
{
  configure_callback,
};

void
signal_int(int signum)
{
  SupressUnused(signum);
  global_data.running = 0;
}

#define error_out(string) \
do {  \
printf("[error] " string "\n"); \
printf("  location: %s:%d\n", __FILE__, __LINE__); \
printf("  errno: %s\n", strerror(errno)); \
printf("  egl error: %s\n", egl_error_string(eglGetError())); \
exit(1); \
} while (0)

int
main(int argc, char **argv)
{
  SupressUnused(argc);
  SupressUnused(argv);
  
  //- display setup
  struct wl_display *display = 0;
  struct wl_registry *registry = 0;
  {
    display = wl_display_connect(0);
    if (!display) 
    {
      error_out("failed to connect display");
    }
    
    registry = wl_display_get_registry(display);
    if (!registry)
    {
      error_out("failed to connect registry");
    }
    
    wl_registry_add_listener(registry, &registry_listener, &global_data);
#if 0
    wl_display_roundtrip(display);
    wl_display_roundtrip(display);
#else
    wl_display_dispatch(display);
#endif
  }
  
  //- egl setup
  EGLConfig egl_config;
  EGLContext egl_context;
  {
    //- egl display
    {
      global_data.egl_display = eglGetDisplay(display);
      if (!global_data.egl_display)
      {
        error_out("faile to get egl display");
      }
      
      EGLint version_major;
      EGLint version_minor;
      EGLBoolean egl_inititalized= eglInitialize(global_data.egl_display, 
                                                 &version_major, 
                                                 &version_minor);
      if (egl_inititalized != EGL_TRUE)
      {
        error_out("failed to initialize egl");
      }
      else
      {
        printf("EGL VERSION: %d.%d\n", version_major, version_minor);
      }
      
      EGLBoolean egl_binded = eglBindAPI(EGL_OPENGL_ES_API);
      if (egl_binded != EGL_TRUE)
      {
        error_out("failed to bind opengl with egl");
      }
    }
    
    //- egl config
    {
      EGLint config_attribs[] = 
      {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_ALPHA_SIZE, 1,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
      };
      
      //- determine number of configs
      EGLint egl_num_config = 0;
      eglChooseConfig(global_data.egl_display, config_attribs, 0, 0, &egl_num_config);
      if (egl_num_config == 0)
      {
        error_out("failed to get amout of egl configs");
      }
      
      //- load all available configs
      EGLConfig *temp_configs = malloc(egl_num_config * sizeof(EGLConfig));
      EGLint loaded_config_count = 0;
      EGLint chose_config = eglChooseConfig(global_data.egl_display, config_attribs, 
                                            temp_configs, egl_num_config, 
                                            &loaded_config_count);
      if (chose_config != EGL_TRUE || loaded_config_count != egl_num_config)
      {
        error_out("failed to get egl configs");
      }
      
      //- get single config
      {
        // TODO(erb): make a more intellegent pick of the config here
        egl_config = temp_configs[0];
      }
      
      if (temp_configs)
      {
        free(temp_configs);
      }
    }
    
    //- egl context
    {
      const EGLint context_attribs[] = 
      {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE,
      };
      egl_context = eglCreateContext(global_data.egl_display, egl_config, EGL_NO_CONTEXT, context_attribs);
      if (egl_context == EGL_NO_CONTEXT)
      {
        error_out("failed to create egl context");
      }
    }
  }
  
  //-create surface
  struct wl_egl_window *egl_window;
  {
    global_data.surface = wl_compositor_create_surface(global_data.compositor);
    if (!global_data.surface)
    {
      error_out("failed to create compositor surface");
    }
    
    global_data.shell_surface = wl_shell_get_shell_surface(global_data.shell, global_data.surface);
    if (!global_data.shell_surface)
    {
      error_out("failed to get wl shell surface");
    }
    
    wl_shell_surface_add_listener(global_data.shell_surface, &shell_surface_listener, &global_data);
    
    global_data.width = 300;
    global_data.height = 300;
    egl_window = wl_egl_window_create(global_data.surface, global_data.width, global_data.height);
    if (!egl_window)
    {
      error_out("failed to create egl window");
    }
    
    global_data.egl_surface =
      eglCreateWindowSurface(global_data.egl_display, egl_config, egl_window, 0);
    if (global_data.egl_surface == EGL_NO_SURFACE)
    {
      error_out("failed to create egl surface");
    }
    
    wl_shell_surface_set_title(global_data.shell_surface, "TITLE ME");
    
    EGLBoolean egl_made_current = eglMakeCurrent(global_data.egl_display, global_data.egl_surface, 
                                                 global_data.egl_surface, egl_context);
    if (egl_made_current != EGL_TRUE)
    {
      error_out("failed to make egl current");
    }
    
    wl_shell_surface_set_toplevel(global_data.shell_surface);
    handle_configure(&global_data, global_data.shell_surface, 0, 
                     global_data.width, 
                     global_data.height);
    
    struct wl_callback *callback = wl_display_sync(display);
    wl_callback_add_listener(callback, &configure_callback_listener, &global_data);
  }
  
  //- opengl 
  {
    global_data.fragment_shader = create_shader(frag_shader_text, GL_FRAGMENT_SHADER);
    global_data.vertex_shader = create_shader(vert_shader_text, GL_VERTEX_SHADER);
    
    global_data.program = glCreateProgram();
    if (!global_data.program)
    {
      error_out("failed to glCreateProgram");
    }
    
    glAttachShader(global_data.program, global_data.fragment_shader);
    glAttachShader(global_data.program, global_data.vertex_shader);
    glLinkProgram(global_data.program);
    {
      GLint status;
      glGetProgramiv(global_data.program, GL_LINK_STATUS, &status);
      if (!status) {
        char log[1000];
        GLsizei len;
        glGetProgramInfoLog(global_data.program, 1000, &len, log);
        fprintf(stderr, "Error: linking:\n%*s\n", len, log);
        exit(1);
      }
    }
    glUseProgram(global_data.program);
    
    global_data.pos = 0;
    global_data.col = 1;
    
    glBindAttribLocation(global_data.program, global_data.pos, "pos");
    glBindAttribLocation(global_data.program, global_data.col, "color");
    glLinkProgram(global_data.program);
    
    global_data.rotation_uniform = glGetUniformLocation(global_data.program, "rotation");
  }
  
  //- cursor
  {
    global_data.cursor_surface = wl_compositor_create_surface(global_data.compositor);
  }
  
  //- sigaction
  {
    struct sigaction sigint;
    sigint.sa_handler = signal_int;
    sigemptyset(&sigint.sa_mask);
    sigint.sa_flags = SA_RESETHAND;
    sigaction(SIGINT, &sigint, NULL);
  }
  
  //- main loop
  printf("starting\n");
  global_data.running = true;
  for (;global_data.running;)
  {
    int disp = wl_display_dispatch(display);
    if (disp == -1)
    {
      break;
    }
  }
  
  //- release egl
  {
    /* Required, otherwise segfault in egl_dri2.c: dri2_make_current()
     * on eglReleaseThread(). */
    eglMakeCurrent(global_data.egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    
    eglDestroySurface(global_data.egl_display, global_data.egl_surface);
    wl_egl_window_destroy(egl_window);
    
    wl_shell_surface_destroy(global_data.shell_surface);
    wl_surface_destroy(global_data.surface);
    
    if (global_data.callback)
    {
      wl_callback_destroy(global_data.callback);
    }
    
    eglTerminate(global_data.egl_display);
    eglReleaseThread();
  }
  
  //- destroy surface
  {
    wl_surface_destroy(global_data.cursor_surface);
    
    if (global_data.cursor_theme)
    {
      wl_cursor_theme_destroy(global_data.cursor_theme);
    }
    
    if (global_data.shell)
    {
      wl_shell_destroy(global_data.shell);
    }
    
    if (global_data.compositor)
    {
      wl_compositor_destroy(global_data.compositor);
    }
    
    wl_registry_destroy(registry);
    wl_display_flush(display);
    wl_display_disconnect(display);
  }
  return 0;
}
