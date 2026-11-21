#include <math.h>

// headers
#include "base/base.h"
#include "img/img.h"
#include "gfx/gfx.h"
#include "fnt/fnt.h"
#include "gfx/q2d/q2d.h"
#include "app_bridge.h"
#include "ui/ui.h"

// implementation
#include "base/base.c"
#include "img/img.c"
#include "gfx/gfx.c"
#include "fnt/fnt.c"
#include "gfx/q2d/q2d.c"
#include "ui/ui.c"

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

void
gfx_eat_event(GFX_Event_List *event_list, GFX_Event *event)
{
  event_list->count--;
  DLLRemove(event_list->first, event_list->last, event);
}

typedef struct Update_Params Update_Params;
struct Update_Params
{
  GFX_Window window;
  App_Memory *app_memory;
  Q2D_State *render_state;
  Q2D_Font *font; 
  GFX_Event_List *events;
  Platform_API platform_api;
  F32 delta_time;
  String8 current_directory;
  
  Update_And_Render_Func *update_func;
};

void
update(void *data)
{
  if(data != 0)
  {
    Update_Params *params = (Update_Params *)data;
    q2d_begin(params->window);
    {
      params->update_func(params->app_memory, 
                          params->render_state, 
                          params->font, 
                          gfx_window_get_client_size(params->window), 
                          params->events, 
                          params->platform_api, 
                          params->delta_time, 
                          params->current_directory);
    }
    q2d_render(params->render_state);
    q2d_end(params->window, params->render_state);
  }
}

typedef struct Game_Room Game_Room;
struct Game_Room
{
  String8 name;
  U32 port;
  U64 ip_addr;
  char ip_str[INET_ADDRSTRLEN];
};

typedef struct App_State App_State;
struct App_State
{
  Arena arena;
  UI_Ctx ui;
  UI_Font_Face font_face;
  B32 initialized;
  Thread_Context thread_context;
#if DEBUG
  UI_Debug_Ctx debug_ctx;
#endif
  
  // beacon
  SOCKET beacon_socket;
  B32 room_created;
  U8 room_name_buffer[32];
  UI_Line_Input_State room_name_input;
  
  // discovery
  SOCKET discovery_socket;
  
  // NOTE(erb): error
  String8 error_str;
  F32 error_timer;
};

void
render_ui_cmds(Q2D_State *render_state, UI_Draw_Cmd_List *draw_commands)
{
  for(EachNode(UI_Draw_Cmd, cmd, draw_commands->first))
  {
    switch(cmd->kind)
    {
      case UIDrawKind_Text:
      {
        Assert(cmd->text.font->height_px != 0);
        
        F32 scale = (cmd->text.font_size / cmd->text.font->height_px);
        F32 ascent = (cmd->text.font->data.ascent_em * cmd->text.font->data.scale_em_to_pixels * scale);
        V2f top_left = v2f(cmd->range.min.x, cmd->range.min.y + ascent);
        
        q2d_render_string_baseline_px(render_state, cmd->text.content, top_left, 
                                      cmd->text.font, cmd->text.font_size, cmd->text.blank_spacing, 
                                      cmd->text.char_spacing, cmd->text.color, true);
      } break;
      
      case UIDrawKind_Rect:
      case UIDrawKind_RectLines:
      {
        F32 edge_softness = lerp(0, 2, cmd->rect.corner_radius/150.0f);
        F32 border_thickness = (cmd->kind == UIDrawKind_RectLines ? cmd->rect.border_thickness : 0.0f);
        
        Quad_Colors colors = {0};
        if(cmd->kind == UIDrawKind_RectLines)
        {
          colors = quad_colors_single(cmd->color);
        }
        else
        {
          Orientation2 orient = cmd->background.orientation;
          V4f color0 = cmd->background.color0;
          V4f color1 = cmd->background.color1;
          switch(orient)
          {
            case Orientation2_NS:
            {
              colors.top_left = colors.top_right = color0;
              colors.bottom_left = colors.bottom_right = color1;
            } break;
            
            case Orientation2_WE:
            {
              colors.top_left = colors.bottom_left = color0;
              colors.top_right = colors.bottom_right = color1;
            } break;
            
            case Orientation2_NESW:
            {
              colors.top_right = color0;
              colors.bottom_left = color1;
              colors.top_left = colors.bottom_right = v4f_lerp(color0, color1, 0.5f);
            } break;
            
            case Orientation2_NWSE:
            {
              colors.top_left = color0;
              colors.bottom_right = color1;
              colors.top_right = colors.bottom_left = v4f_lerp(color0, color1, 0.5f);
            } break;
          }
        }
        q2d_render_rect_color_ex(render_state, cmd->range, colors, 
                                 cmd->rect.corner_radius, edge_softness, border_thickness);
      } break;
      
      case UIDrawKind_ClipBegin:
      {
        Range_V2i rect = rng_v2i_from_inclusive_rng_v2f(cmd->range);
        q2d_scissor_begin(render_state, rect);
      } break;
      
      case UIDrawKind_ClipEnd:
      {
        q2d_scissor_end(render_state);
      } break;
      
      InvalidDefaultCase;
    }
  }
}

#pragma pack(push, 1)
typedef struct Beacon_Packet Beacon_Packet;
struct Beacon_Packet
{
  U32 magic;
  U32 version;
  U16 game_port;
  
  U8 name_buffer[32];
  U16 name_size;
};
#pragma pack(pop)

#define BEACON_MAGIC 0xDEADBEEF
#define DISCOVERY_PORT 47777
#define GAME_PORT 47778

void
app_set_ui_error(App_State *app_state, String8 error_str)
{
  app_state->error_str = error_str;
  app_state->error_timer = 5.0f;
}

UPDATE_AND_RENDER_FUNC(update_and_render)
{
  App_State *app_state = (App_State *)memory->buffer;
  if(memory->first_attach)
  {
    memory->first_attach = false;
    B32 os_initted = os_init();
    Assert(os_initted);
    thread_context_init(&app_state->thread_context);
    os_thread_context_set(&app_state->thread_context);
  }
  if(!app_state->initialized)
  {
    app_state->initialized = true;
    arena_init(&app_state->arena, (U8 *)memory->buffer+sizeof(App_State), 
               memory->size-sizeof(App_State));
    app_state->font_face.font = font;
    app_state->font_face.blank_spacing = 10.0f;
    app_state->font_face.char_spacing = 0.0f;
    app_state->font_face.line_spacing = 1.0f;
    app_state->font_face.size = 28.0f;
    ui_init(&app_state->ui, &app_state->arena);
    
#if DEBUG
    global_ui_debug_ctx = &app_state->debug_ctx;
#endif
    
    // NOTE(erb): discovery socket setup
    {
      app_state->discovery_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      Assert(app_state->discovery_socket != INVALID_SOCKET);
      
      {
        BOOL on = TRUE;
        int result = setsockopt(app_state->discovery_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on));
        Assert(result != SOCKET_ERROR);
      }
      
      // bind
      struct sockaddr_in addr = {0};
      addr.sin_family = AF_INET;
      addr.sin_port = htons(DISCOVERY_PORT);
      addr.sin_addr.s_addr = htonl(INADDR_ANY);
      {
        int result = bind(app_state->discovery_socket, (struct sockaddr *)&addr, sizeof(addr));
        Assert(result != SOCKET_ERROR);
      }
      
      // nonblocking
      {
        u_long on = 1;
        ioctlsocket(app_state->discovery_socket, FIONBIO, &on);
      }
    }
    
    // NOTE(erb): room input
    {
      UI_Line_Input_State *li = &app_state->room_name_input;
      li->key = ui_keyf("room_name_input");
      str8_writer_init(&li->writer, app_state->room_name_buffer, 
                       ArraySize(app_state->room_name_buffer));
      
      str8_writer_str(&li->writer, S8("My Room"));
    }
  }
  
  Scratch scratch = scratch_begin(0, 0);
  
  // NOTE(erb): ui
  UI_Ctx *ui = &app_state->ui;
  ui_set_clipboard_api(ui, platform_api.clipboard);
  ui_set_cursor_api(ui, platform_api.cursor);
  
  Range_V2f client_rect = rng_v2f_point_add(v2ff(0), v2fi(client_size));
  ui_begin_frame(ui, app_state->font_face, events, client_rect, delta_time);
  ui_font_color(ui, color_black)
  {
    ui_set_next_layout_axis(ui, Axis2_Y);
    ui_set_next_sizing(ui, ui_grow(), ui_grow());
    ui_parentf(ui, 0, S8Z())
    {
      ui_labelf(ui, S8("BROADCASTERRRR!!"));
      
      UI_Line_Input_State *input = &app_state->room_name_input;
      
      ui_set_next_layout_axis(ui, Axis2_X);
      ui_set_next_sizing_x(ui, ui_grow());
      ui_parentf(ui, 0, S8Z())
      {
        ui_set_next_border_color(ui, color_blue);
        ui_set_next_sizing_x(ui, ui_grow());
        ui_line_input_update_state(ui, input);
        
        if(ui_buttonf(ui, S8("Make Room##ui_room_submit")).left_clicked)
        {
          if(input->writer.str.size == 0)
          {
            app_set_ui_error(app_state, S8("Room name empty"));
          }
          else if(app_state->room_created)
          {
            app_set_ui_error(app_state, S8("Room allready created"));
          }
          else
          {
            app_state->room_created = true;
            
            // open socket
            app_state->beacon_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            Assert(app_state->beacon_socket != INVALID_SOCKET);
            
            // set as broadcast socket
            BOOL on = TRUE;
            int result = setsockopt(app_state->beacon_socket, SOL_SOCKET, SO_BROADCAST, (const char *)&on, sizeof(on));
            Assert(result != SOCKET_ERROR);
          }
        }
      }
      
      if(app_state->room_created)
      {
        String8 my_name = input->writer.str;
        
        ui_set_next_layout_axis(ui, Axis2_X);
        ui_parentf(ui, 0, S8Z())
        {
          ui_labelf(ui, S8("ROOM: %S"), my_name);
        }
        
        Beacon_Packet packet = {0};
        {
          Assert(my_name.size < ArraySize(packet.name_buffer));
          Assert(my_name.size < u8_max);
          
          packet.magic = htonl(BEACON_MAGIC);
          packet.version = htonl(1);
          packet.game_port = htons(1);
          packet.name_size = htons((U16)my_name.size);
          ArrayCopy(my_name.value, packet.name_buffer, my_name.size);
        }
        
        struct sockaddr_in dest_address = {0};
        dest_address.sin_family = AF_INET;
        dest_address.sin_port = htons(DISCOVERY_PORT);
        dest_address.sin_addr.s_addr = htonl(INADDR_BROADCAST);
        
        // send
        int send_result = sendto(app_state->beacon_socket, 
                                 (const char *)&packet, sizeof(Beacon_Packet),
                                 0, 
                                 (struct sockaddr *)&dest_address, sizeof(dest_address));
        Assert(send_result != SOCKET_ERROR);
      }
      
      Game_Room rooms_array[32];
      U32 rooms_count = 0;
      {
        Beacon_Packet incoming_beacon = {0};
        struct sockaddr_in from_addr;
        int from_addr_len = sizeof(from_addr);
        
        for(;rooms_count+1 < ArraySize(rooms_array);)
        {
          int rec_result = recvfrom(app_state->discovery_socket,
                                    (char *)&incoming_beacon, sizeof(incoming_beacon),
                                    0,
                                    (struct sockaddr *)&from_addr, &from_addr_len);
          if(rec_result == SOCKET_ERROR || rec_result != sizeof(Beacon_Packet))
          {
            break;
          }
          
          if(ntohl(incoming_beacon.magic) == BEACON_MAGIC)
          {
            U32 name_size = ntohs(incoming_beacon.name_size);
            Assert(name_size < 1024);
            String8 name = S8Struct(incoming_beacon.name_buffer, name_size);
            U32 port = ntohs(incoming_beacon.game_port);
            
            // check if room does not exist
            B32 room_exists = false;
            for(EachPtr(Game_Room, room, rooms_array, rooms_count))
            {
              if(room->ip_addr == from_addr.sin_addr.s_addr &&
                 room->port == port &&
                 str8_equals(room->name, name))
              {
                room_exists = true;
                break;
              }
            }
            
            if(!room_exists)
            { 
              Assert(rooms_count+1 < ArraySize(rooms_array));
              Game_Room *room = (rooms_array + rooms_count++);
              room->name = push_str8_copy(scratch.arena, name);
              room->port = port;
              room->ip_addr = from_addr.sin_addr.s_addr;
              const char *to_ip_str_result = inet_ntop(AF_INET, &from_addr.sin_addr, room->ip_str, ArraySize(room->ip_str));
              Assert(to_ip_str_result != 0);
            }
          }
        }
      }
      
      // NOTE(erb): discovery pool
      ui_set_next_layout_axis(ui, Axis2_Y);
      ui_parentf(ui, 0, S8Z())
      {
        ui_labelf(ui, S8("Rooms available: %d"), rooms_count);
        for(EachPtr(Game_Room, room, rooms_array, rooms_count))
        {
          ui_labelf(ui, S8("- '%S' @ %s:%u"), room->name, room->ip_str, room->port);
        }
      }
      
      if(app_state->error_timer > 0)
      {
        app_state->error_timer -= delta_time;
        
        ui_set_next_sizing_x(ui, ui_grow());
        ui_set_next_background_color(ui, color_yellow);
        ui_parentf(ui, UIFlags_DrawBackGround, S8Z())
        {
          ui_set_next_font_color(ui, color_red);
          ui_labelf(ui, S8("error: %S"), app_state->error_str);
        }
      }
    }
  }
  UI_Draw_Cmd_List draw_commands = ui_layout_and_draw(ui, scratch.arena);
  render_ui_cmds(render_state, &draw_commands);
  ui_end_frame(ui);
  scratch_end(&scratch);
}

OS_ENTRY_POINT()
{
  //- os init
  B32 os_initialized = os_init();
  if (!os_initialized)
  {
    os_handle_bad_error(err_get_last_error());
  }
  Thread_Context tctx = {0};
  thread_context_init(&tctx);
  os_thread_context_set(&tctx);
  
  Arena arena_store = {0};
  {
    U64 size = MB(128);
    arena_init(&arena_store, os_memory_commit(size), size);
  }
  Arena *arena = &arena_store;
  
  //- gfx init
  B32 gfx_initialized = gfx_init();
  if (!gfx_initialized)
  {
    os_handle_bad_error(err_get_last_error());
  }
  B32 gl_initialized = gfx_gl_init();
  if (!gl_initialized)
  {
    os_handle_bad_error(err_get_last_error());
  }
  
  //- window init
  GFX_Window window = gfx_window_create();
  if (window == 0) 
  {
    os_handle_bad_error(err_get_last_error());
  }
  gfx_window_set_title(window, S8("BROADCAST"));
  
  //- equip gl
  B32 equipped = gfx_gl_equip(window);
  if (!equipped)
  {
    os_handle_bad_error(err_get_last_error());
  }
  gfx_window_show(window);
  
  //- q2d
  String8 shader_search_folder = S8("code");
  B32 q2d_initialized = q2d_init(shader_search_folder);
  if (!q2d_initialized)
  {
    os_handle_bad_error(err_get_last_error());
  }
  
  Q2D_State *render_state = make_render_data(arena);
  
  B32 fnt_initialized = fnt_init();
  if(!fnt_initialized)
  {
    os_handle_bad_error(err_get_last_error());
  }
  
  //- font baking
  Q2D_Font font = {0};
  {
    String8 font_file = os_debug_read_entire_file(S8("data/times.ttf"));
    font = q2d_load_font(arena, font_file, 48.0f);
    os_debug_free_file_memory((void *)font_file.value);
  }
  
  GFX_Event_List events = {0};
  
  App_Memory app_memory = {0};
  {
    app_memory.size = GB(2);
    app_memory.buffer = os_memory_commit(app_memory.size);
    app_memory.first_attach = true;
  }
  
  ////////////////////////////////////////////
  // NOTE(erb): TEST CODE - START
  {
    
    // init net
    {
      WSADATA wsa_data = {0};
      WORD wsa_version = MAKEWORD(2, 2);
      int res = WSAStartup(wsa_version, &wsa_data);
      Assert(res == 0);
    }
  }
  // NOTE(erb): END
  ////////////////////////////////////////////
  
  Platform_API platform_api = {0};
  F32 target_seconds_per_frame = 1.0f/60.0f;
  
  Update_Params update_params = {0};
  update_params.window = window;
  update_params.app_memory = &app_memory;
  update_params.render_state = render_state; 
  update_params.font = &font;
  update_params.platform_api = platform_api;
  update_params.delta_time = target_seconds_per_frame;
  update_params.update_func = update_and_render;
  update_params.events = &events;
  update_params.current_directory = os_get_current_directory(arena); // NOTE(erb): allocation
  
  gfx_window_set_resize_update_func(window, update);
  gfx_window_set_resize_update_data(window, &update_params);
  
  //- update loop
  F32 tick = 0;
  F32 delta_time = target_seconds_per_frame;
  OS_Clock_Time last_clock = os_get_clock_time();
  
  for(;;++tick) 
  {
    Scratch frame_scratch = arena_get_scratch(arena);
    
    events = gfx_poll_events(frame_scratch.arena);
    
    B32 should_stop = false;
    for(EachNode(GFX_Event, event, events.first))
    {
      if (event->kind == GFXEventKind_WindowClose)
      {
        Assert(event->window == window);
        gfx_eat_event(&events, event);
        should_stop = true;
        break;
      }
    }
    if(should_stop)
    {
      break;
    }
    
    {
      update_params.events = &events;
      update_params.delta_time = delta_time;
      update_params.update_func = update_and_render;
      update(&update_params);
      gfx_window_set_resize_update_data(window, &update_params);
    }
    
    arena_scratch_reset(&frame_scratch);
    
    F32 seconds_elapsed = os_seconds_elapsed(last_clock, os_get_clock_time());
    if(seconds_elapsed <= target_seconds_per_frame)
    {
      os_sleep_ms(floor_f32_to_i32(seconds_elapsed*1000.0f));
      do
      {
        seconds_elapsed = os_seconds_elapsed(last_clock, os_get_clock_time());
      }
      while(seconds_elapsed <= target_seconds_per_frame);
    }
    else
    {
      // NOTE(erb): skipped frame
      os_log(S8("skipped frame"));
    }
    OS_Clock_Time end_clock = os_get_clock_time();
    delta_time = os_seconds_elapsed(last_clock, end_clock);
    last_clock = end_clock;
  }
  
  // close net
  {
    WSACleanup();
  }
  
  // TODO(erb): destroy render_data
  //- release
  gfx_window_destroy(window);
  os_release();
  
  return(0);
}