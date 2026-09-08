/* date = April 30th 2026 0:15 pm */

#ifndef GFX_H
#define GFX_H

//- gfx backends
#define GFX_BACKEND_OPENGL 0x1
#define GFX_BACKEND_VULKAN 0x2
#define GFX_BACKEND_DEFAULT GFX_BACKEND_OPENGL

//- fallback to default
#ifndef GFX_BACKEND
#define GFX_BACKEND GFX_BACKEND_DEFAULT
#endif

typedef U64  GFX_Window;
typedef B32  GFX_Window_Release_Func(GFX_Window handle);
typedef void GFX_Window_Resize_Update_Func(void *data);


// NOTE(erb): init
B32         gfx_init(void);

// NOTE(erb): window
GFX_Window  gfx_window_create(void);
B32         gfx_window_destroy(GFX_Window handle);
B32         gfx_window_is_valid(GFX_Window handle);
void        gfx_window_set_resize_update_func(GFX_Window handle, GFX_Window_Resize_Update_Func *func);
void        gfx_window_set_resize_update_data(GFX_Window handle, void *data);

void        gfx_window_show(GFX_Window handle);
V2i         gfx_window_get_client_size(GFX_Window handle);
void        gfx_window_set_size(GFX_Window handle, V2i size);
void        gfx_window_set_position(GFX_Window handle, V2i position);
void        gfx_window_set_title(GFX_Window handle, String8 text);
V2f         gfx_window_get_cursor_position(GFX_Window handle);

// NOTE(erb): clipboard
String8     gfx_get_clipboard_str8(Arena *arena);
void        gfx_set_clipboard_str8(String8 str);

// NOTE(erb): misc
typedef enum GFX_Cursor_Kind
{
  GFXCursorKind_None,
  GFXCursorKind_Arrow,
  GFXCursorKind_Beam,
  GFXCursorKind_Hand,
  GFXCursorKind_Cross,
  GFXCursorKind_Wait,
  GFXCursorKind_ArrowNS,
  GFXCursorKind_ArrowWE,
  GFXCursorKind_ArrowNWSE,
  GFXCursorKind_ArrowSWNE,
  GFXCursorKind_ArrowAll,
  GFXCursorKind_COUNT,
} GFX_Cursor_Kind;

void        gfx_set_cursor(GFX_Cursor_Kind kind);
V2f         gfx_screen_get_cursor_position(void);

// NOTE(erb): events
typedef enum GFX_Mouse_Code
{
  GFXMouseCode_None,
  GFXMouseCode_Left,
  GFXMouseCode_Right,
  GFXMouseCode_Middle,
  GFXMouseCode_COUNT,
} GFX_Mouse_Code;

typedef enum GFX_Event_Kind
{
  GFXEventKind_None = 0,
  GFXEventKind_WindowClose,
  GFXEventKind_WindowLoseFocus,
  GFXEventKind_KeyDown,
  GFXEventKind_KeyUp,
  GFXEventKind_MouseDown,
  GFXEventKind_MouseUp,
  GFXEventKind_MouseWheel,
  GFXEventKind_MouseMove,
  GFXEventKind_Text,
} GFX_Event_Kind;

typedef enum GFX_Modifiers
{
  GFXModifiers_Shift   = (1 << 0),
  GFXModifiers_Control = (1 << 1),
  GFXModifiers_Alt     = (1 << 2),
} GFX_Modifiers;

typedef enum GFX_Key
{
  GFXKey_None = 0,
  
  // TODO(erb): add all keys
  // NOTE(erb): ascii values
  GFXKey_0          = 48,
  GFXKey_1          = 49,
  GFXKey_2          = 50,
  GFXKey_3          = 51,
  GFXKey_4          = 52,
  GFXKey_5          = 53,
  GFXKey_6          = 54,
  GFXKey_7          = 55,
  GFXKey_8          = 56,
  GFXKey_9          = 57,
  
  GFXKey_A          = 65,
  GFXKey_B          = 66,
  GFXKey_C          = 67,
  GFXKey_D          = 68,
  GFXKey_E          = 69,
  GFXKey_F          = 70,
  GFXKey_G          = 71,
  GFXKey_H          = 72,
  GFXKey_I          = 73,
  GFXKey_J          = 74,
  GFXKey_K          = 75,
  GFXKey_L          = 76,
  GFXKey_M          = 77,
  GFXKey_N          = 78,
  GFXKey_O          = 79,
  GFXKey_P          = 80,
  GFXKey_Q          = 81,
  GFXKey_R          = 82,
  GFXKey_S          = 83,
  GFXKey_T          = 84,
  GFXKey_U          = 85,
  GFXKey_V          = 86,
  GFXKey_W          = 87,
  GFXKey_X          = 88,
  GFXKey_Y          = 89,
  GFXKey_Z          = 90,
  
  GFXKey_Space,
  GFXKey_Quote,
  GFXKey_Comma,
  GFXKey_Minus,
  GFXKey_Period,
  GFXKey_Slash,
  GFXKey_Semicolon,
  GFXKey_Equal,
  
  GFXKey_LeftBracket,
  GFXKey_RightBracket,
  GFXKey_Backslash,
  GFXKey_Backtick,
  GFXKey_Escape,
  GFXKey_Enter,
  GFXKey_Tab,
  GFXKey_Backspace,
  GFXKey_Insert,
  GFXKey_Delete,
  
  GFXKey_Right,
  GFXKey_Left,
  GFXKey_Down,
  GFXKey_Up,
  
  GFXKey_F1,
  GFXKey_F2,
  GFXKey_F3,
  GFXKey_F4,
  GFXKey_F5,
  GFXKey_F6,
  GFXKey_F7,
  GFXKey_F8,
  GFXKey_F9,
  GFXKey_F10,
  GFXKey_F11,
  GFXKey_F12,
  
  GFXKey_Shift,
  GFXKey_Control,
  GFXKey_Alt,
  GFXKey_Super,
  
  GFXKey_COUNT,
} GFX_Key;

typedef struct GFX_Event GFX_Event;
struct GFX_Event
{
  GFX_Event_Kind kind;
  GFX_Event *next;
  GFX_Event *prev;
  
  GFX_Modifiers modifiers;
  GFX_Window window;
  
  union 
  {
    struct
    {
      GFX_Mouse_Code code;
      V2f position;
      V2f wheel;
      V2i wheel_clicks;
    } mouse;
    
    struct
    {
      GFX_Key code;
      U32 repeat_count;
      I32 codepoint;
    } key;
  };
};

typedef struct GFX_Event_List GFX_Event_List;
struct GFX_Event_List
{
  U32 count;
  GFX_Event *first;
  GFX_Event *last;
};
GFX_Event_List gfx_poll_events(Arena *arena);
GFX_Modifiers  gfx_get_modifiers(void);


//////////////////////////////////////////////////////////
// os includes
#if defined(OS_WIN32)
#include "win32/win32_gfx.h"
#elif defined(OS_LINUX)
#include "linux/linux_gfx.h"
#endif

#if GFX_BACKEND == GFX_BACKEND_OPENGL
#include "gl/gl.h"
#elif GFX_BACKEND == GFX_BACKEND_VULKAN
#include "vulkan/vulkan.h"
#else
#error "No other graphics backend supported"
#endif

#endif //GFX_H
