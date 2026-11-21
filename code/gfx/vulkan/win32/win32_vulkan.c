global W32_Vulkan_Window w32_vulkan_window_slots[W32_GFX_WINDOW_SLOT_COUNT];
global W32_Vulkan_State w32_vulkan_state;

#define w32_vulkan_gfx_handle_from_slot(slot) (GFX_Window)(((slot) - w32_vulkan_window_slots) + 1)
#define w32_vulkan_slot_from_gfx_handle(handle) (W32_Vulkan_Window *)((w32_vulkan_window_slots + (handle)) - 1)

// TODO(erb): Change the source later to something understandable
const uint32_t FRAG_SHADER[] = {
  0x07230203, 0x00010000, 0x000d000b, 0x00000013, 0x00000000, 0x00020011,
  0x00000001, 0x0006000b, 0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
  0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x0007000f, 0x00000004,
  0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x0000000c, 0x00030010,
  0x00000004, 0x00000007, 0x00030003, 0x00000002, 0x000001c2, 0x000a0004,
  0x475f4c47, 0x4c474f4f, 0x70635f45, 0x74735f70, 0x5f656c79, 0x656e696c,
  0x7269645f, 0x69746365, 0x00006576, 0x00080004, 0x475f4c47, 0x4c474f4f,
  0x6e695f45, 0x64756c63, 0x69645f65, 0x74636572, 0x00657669, 0x00040005,
  0x00000004, 0x6e69616d, 0x00000000, 0x00050005, 0x00000009, 0x4374756f,
  0x726f6c6f, 0x00000000, 0x00050005, 0x0000000c, 0x67617266, 0x6f6c6f43,
  0x00000072, 0x00040047, 0x00000009, 0x0000001e, 0x00000000, 0x00040047,
  0x0000000c, 0x0000001e, 0x00000000, 0x00020013, 0x00000002, 0x00030021,
  0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017,
  0x00000007, 0x00000006, 0x00000004, 0x00040020, 0x00000008, 0x00000003,
  0x00000007, 0x0004003b, 0x00000008, 0x00000009, 0x00000003, 0x00040017,
  0x0000000a, 0x00000006, 0x00000003, 0x00040020, 0x0000000b, 0x00000001,
  0x0000000a, 0x0004003b, 0x0000000b, 0x0000000c, 0x00000001, 0x0004002b,
  0x00000006, 0x0000000e, 0x3f800000, 0x00050036, 0x00000002, 0x00000004,
  0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x0004003d, 0x0000000a,
  0x0000000d, 0x0000000c, 0x00050051, 0x00000006, 0x0000000f, 0x0000000d,
  0x00000000, 0x00050051, 0x00000006, 0x00000010, 0x0000000d, 0x00000001,
  0x00050051, 0x00000006, 0x00000011, 0x0000000d, 0x00000002, 0x00070050,
  0x00000007, 0x00000012, 0x0000000f, 0x00000010, 0x00000011, 0x0000000e,
  0x0003003e, 0x00000009, 0x00000012, 0x000100fd, 0x00010038};

// TODO(erb): Change the source later to something understandable
const uint32_t VERT_SHADER[] = {
  0x07230203, 0x00010000, 0x000d000b, 0x00000036, 0x00000000, 0x00020011,
  0x00000001, 0x0006000b, 0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
  0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x0008000f, 0x00000000,
  0x00000004, 0x6e69616d, 0x00000000, 0x00000022, 0x00000026, 0x00000031,
  0x00030003, 0x00000002, 0x000001c2, 0x000a0004, 0x475f4c47, 0x4c474f4f,
  0x70635f45, 0x74735f70, 0x5f656c79, 0x656e696c, 0x7269645f, 0x69746365,
  0x00006576, 0x00080004, 0x475f4c47, 0x4c474f4f, 0x6e695f45, 0x64756c63,
  0x69645f65, 0x74636572, 0x00657669, 0x00040005, 0x00000004, 0x6e69616d,
  0x00000000, 0x00050005, 0x0000000c, 0x69736f70, 0x6e6f6974, 0x00000073,
  0x00040005, 0x00000017, 0x6f6c6f63, 0x00007372, 0x00060005, 0x00000020,
  0x505f6c67, 0x65567265, 0x78657472, 0x00000000, 0x00060006, 0x00000020,
  0x00000000, 0x505f6c67, 0x7469736f, 0x006e6f69, 0x00070006, 0x00000020,
  0x00000001, 0x505f6c67, 0x746e696f, 0x657a6953, 0x00000000, 0x00070006,
  0x00000020, 0x00000002, 0x435f6c67, 0x4470696c, 0x61747369, 0x0065636e,
  0x00070006, 0x00000020, 0x00000003, 0x435f6c67, 0x446c6c75, 0x61747369,
  0x0065636e, 0x00030005, 0x00000022, 0x00000000, 0x00060005, 0x00000026,
  0x565f6c67, 0x65747265, 0x646e4978, 0x00007865, 0x00050005, 0x00000031,
  0x67617266, 0x6f6c6f43, 0x00000072, 0x00030047, 0x00000020, 0x00000002,
  0x00050048, 0x00000020, 0x00000000, 0x0000000b, 0x00000000, 0x00050048,
  0x00000020, 0x00000001, 0x0000000b, 0x00000001, 0x00050048, 0x00000020,
  0x00000002, 0x0000000b, 0x00000003, 0x00050048, 0x00000020, 0x00000003,
  0x0000000b, 0x00000004, 0x00040047, 0x00000026, 0x0000000b, 0x0000002a,
  0x00040047, 0x00000031, 0x0000001e, 0x00000000, 0x00020013, 0x00000002,
  0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020,
  0x00040017, 0x00000007, 0x00000006, 0x00000002, 0x00040015, 0x00000008,
  0x00000020, 0x00000000, 0x0004002b, 0x00000008, 0x00000009, 0x00000003,
  0x0004001c, 0x0000000a, 0x00000007, 0x00000009, 0x00040020, 0x0000000b,
  0x00000006, 0x0000000a, 0x0004003b, 0x0000000b, 0x0000000c, 0x00000006,
  0x0004002b, 0x00000006, 0x0000000d, 0x00000000, 0x0004002b, 0x00000006,
  0x0000000e, 0xbf000000, 0x0005002c, 0x00000007, 0x0000000f, 0x0000000d,
  0x0000000e, 0x0004002b, 0x00000006, 0x00000010, 0x3f000000, 0x0005002c,
  0x00000007, 0x00000011, 0x00000010, 0x00000010, 0x0005002c, 0x00000007,
  0x00000012, 0x0000000e, 0x00000010, 0x0006002c, 0x0000000a, 0x00000013,
  0x0000000f, 0x00000011, 0x00000012, 0x00040017, 0x00000014, 0x00000006,
  0x00000003, 0x0004001c, 0x00000015, 0x00000014, 0x00000009, 0x00040020,
  0x00000016, 0x00000006, 0x00000015, 0x0004003b, 0x00000016, 0x00000017,
  0x00000006, 0x0004002b, 0x00000006, 0x00000018, 0x3f800000, 0x0006002c,
  0x00000014, 0x00000019, 0x00000018, 0x0000000d, 0x0000000d, 0x0006002c,
  0x00000014, 0x0000001a, 0x0000000d, 0x00000018, 0x0000000d, 0x0006002c,
  0x00000014, 0x0000001b, 0x0000000d, 0x0000000d, 0x00000018, 0x0006002c,
  0x00000015, 0x0000001c, 0x00000019, 0x0000001a, 0x0000001b, 0x00040017,
  0x0000001d, 0x00000006, 0x00000004, 0x0004002b, 0x00000008, 0x0000001e,
  0x00000001, 0x0004001c, 0x0000001f, 0x00000006, 0x0000001e, 0x0006001e,
  0x00000020, 0x0000001d, 0x00000006, 0x0000001f, 0x0000001f, 0x00040020,
  0x00000021, 0x00000003, 0x00000020, 0x0004003b, 0x00000021, 0x00000022,
  0x00000003, 0x00040015, 0x00000023, 0x00000020, 0x00000001, 0x0004002b,
  0x00000023, 0x00000024, 0x00000000, 0x00040020, 0x00000025, 0x00000001,
  0x00000023, 0x0004003b, 0x00000025, 0x00000026, 0x00000001, 0x00040020,
  0x00000028, 0x00000006, 0x00000007, 0x00040020, 0x0000002e, 0x00000003,
  0x0000001d, 0x00040020, 0x00000030, 0x00000003, 0x00000014, 0x0004003b,
  0x00000030, 0x00000031, 0x00000003, 0x00040020, 0x00000033, 0x00000006,
  0x00000014, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003,
  0x000200f8, 0x00000005, 0x0003003e, 0x0000000c, 0x00000013, 0x0003003e,
  0x00000017, 0x0000001c, 0x0004003d, 0x00000023, 0x00000027, 0x00000026,
  0x00050041, 0x00000028, 0x00000029, 0x0000000c, 0x00000027, 0x0004003d,
  0x00000007, 0x0000002a, 0x00000029, 0x00050051, 0x00000006, 0x0000002b,
  0x0000002a, 0x00000000, 0x00050051, 0x00000006, 0x0000002c, 0x0000002a,
  0x00000001, 0x00070050, 0x0000001d, 0x0000002d, 0x0000002b, 0x0000002c,
  0x0000000d, 0x00000018, 0x00050041, 0x0000002e, 0x0000002f, 0x00000022,
  0x00000024, 0x0003003e, 0x0000002f, 0x0000002d, 0x0004003d, 0x00000023,
  0x00000032, 0x00000026, 0x00050041, 0x00000033, 0x00000034, 0x00000017,
  0x00000032, 0x0004003d, 0x00000014, 0x00000035, 0x00000034, 0x0003003e,
  0x00000031, 0x00000035, 0x000100fd, 0x00010038};

B32
vulkan_create_shader(VkDevice device, 
                     const uint32_t *source, U32 source_size, 
                     VkShaderModule *out_shader)
{
  VkShaderModuleCreateInfo module = {0};
  module.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  module.codeSize = source_size;
  module.pCode = source;
  B32 result = (vkCreateShaderModule(device, &module, 
                                     0, out_shader) == VK_SUCCESS);
  return result;
}

B32  
gfx_vulkan_init()
{
  String8 error = {0};
  HMODULE module = LoadLibraryA("vulkan-1.dll");
  if (module == 0)
  {
    error = S8("failed to initialize vulkan-1.dll");
  }
  
  //- load vulkan functions
  if (error.value == 0) 
  {
#define X(NAME, RETURN, PARAMS) GET_PROC_ADDR(module, NAME, #NAME);
#include "gfx/vulkan/vulkan_functions_x_list.h"
#undef X
    
#define X(NAME, RETURN, PARAMS) if (NAME == 0) { error = S8("missing vulkan function " #NAME); }
#include "gfx/vulkan/vulkan_functions_x_list.h"
#undef X
  }
  
  //- load vulkan w32 functions
  if (error.value == 0) 
  {
#define X(NAME, RETURN, PARAMS) GET_PROC_ADDR(module, w32_##NAME, #NAME);
#include "gfx/vulkan/win32/win32_vulkan_functions_x_list.h"
#undef X
    
#define X(NAME, RETURN, PARAMS) if (w32_##NAME == 0) { error = S8("missing vulkan function " #NAME); }
#include "gfx/vulkan/win32/win32_vulkan_functions_x_list.h"
#undef X
  }
  
  //- create vulkan instance
  VkInstance instance = 0;
  if (error.value == 0)
  {
    VkApplicationInfo app_info = {0};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "None";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;
    
    const char *validation_layers[] = 
    {
      "VK_LAYER_KHRONOS_validation"
    };
    const char *enabled_extensions[] = 
    {
      VK_KHR_SURFACE_EXTENSION_NAME,
      VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    };
    
    VkInstanceCreateInfo instance_info = {0};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledLayerCount = 1;
    instance_info.ppEnabledLayerNames = validation_layers;
    instance_info.enabledExtensionCount = 2;
    instance_info.ppEnabledExtensionNames = enabled_extensions;
    
    // NOTE(erb): expensive call
    if (vkCreateInstance(&instance_info, 0, &instance) != VK_SUCCESS) 
    {
      error = S8("failed to create vulkan instance");
    }
  }
  
  //- load gpu
  VkPhysicalDevice physical_device = {0};
  {
    //- get physical device count
    U32 physical_device_count = 0;
    if (error.value == 0) 
    {
      vkEnumeratePhysicalDevices(instance, &physical_device_count, 0);
      if (physical_device_count == 0)
      {
        error = S8("no physical devices");
      }
      
      // TODO(erb): handle more than one GPU. Meaning pick one. This should not be a failure case.
      if (physical_device_count != 1)
      {
        error = S8("more than one physical devices");
      }
    }
    
    //- get phyiscal device
    if (error.value == 0) 
    {
      vkEnumeratePhysicalDevices(instance, &physical_device_count, &physical_device);
      if (physical_device == 0)
      {
        error = S8("failed to load vulkan physical device");
      }
    }
  }
  
  //- queue familiy index
  U32 queue_family_idx = 0;
  {
    //- get queue family count
    U32 queue_family_count = 0;
    if (error.value == 0)
    {
      vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, 0);
      if (queue_family_count == 0) 
      {
        error = S8("failed to load physical device queue family");
      }
    }
    
    //- get queue famliy index for device
    if (error.value == 0)
    {
      Scratch scratch = scratch_begin(0, 0);
      VkQueueFamilyProperties *queue_families = PushArray(scratch.arena, queue_family_count, VkQueueFamilyProperties); 
      vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families);
      if (queue_family_count == 0)
      {
        error = S8("failed to load physical device queue family properties");
      }
      
      if (error.value == 0)
      {
        queue_family_idx = queue_family_count;
        for (EachU32(queue_family, queue_family_count))
        {
          VkQueueFamilyProperties *queue = queue_families + queue_family_idx;
          if (queue->queueFlags & VK_QUEUE_GRAPHICS_BIT)
          {
            queue_family_idx = queue_family;
            break;
          }
        }
        if (queue_family_idx == queue_family_count)
        {
          error = S8("failed to load a graphics queue family");
        }
      }
      scratch_end(&scratch);
    }
  }
  
  //- vulkan device
  VkDevice device = 0;
  if (error.value == 0)
  {
    F32 queue_graphics_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = {0};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = queue_family_idx;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &queue_graphics_priority;
    
    VkPhysicalDeviceFeatures device_features = {0};
    const char *device_extensions[] = 
    {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    
    VkDeviceCreateInfo device_info = {0};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.pEnabledFeatures = &device_features;
    device_info.enabledExtensionCount = 1;
    device_info.ppEnabledExtensionNames = device_extensions;
    
    // NOTE(erb): expensive call
    VkDevice logical_device = {0};
    if (vkCreateDevice(physical_device, 
                       &device_info, 
                       0, &logical_device) != VK_SUCCESS) {
      error = S8("failed to create logical logical device");
    }
    device = logical_device;
  }
  
  //- graphics queue
  VkQueue queue = 0;
  if (error.value == 0)
  {
    vkGetDeviceQueue(device, queue_family_idx, 0, &queue);
  }
  
  //- pipeline layout
  VkPipelineLayout pipeline_layout = 0;
  if (error.value == 0)
  {
    VkPipelineLayoutCreateInfo pipeline_info = {0};
    pipeline_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (vkCreatePipelineLayout(device, 
                               &pipeline_info, 
                               0, &pipeline_layout) != VK_SUCCESS) 
    {
      error = S8("failed to create pipeline layout");
    }
  }
  
  
  //- vertex shader
  VkShaderModule vertex_shader_module = 0;
  if (error.value == 0)
  {
    if (!vulkan_create_shader(device, 
                              VERT_SHADER, 
                              sizeof(VERT_SHADER), 
                              &vertex_shader_module)) 
    {
      error = S8("failed to create vertex shader\n");
    }
  }
  
  //- fragment shader
  VkShaderModule fragment_shader_module = 0;
  if (error.value == 0)
  {
    if (!vulkan_create_shader(device, 
                              FRAG_SHADER,
                              sizeof(FRAG_SHADER), 
                              &fragment_shader_module)) 
    {
      error = S8("failed to create fragment shader\n");
    }
  }
  
  if (error.value != 0)
  {
    err_set_error(error);
    if (device != 0)
    {
      if (vertex_shader_module != 0)
      {
        vkDestroyShaderModule(device, vertex_shader_module, 0);
      }
      if (fragment_shader_module != 0)
      {
        vkDestroyShaderModule(device, fragment_shader_module, 0);
      }
      if (pipeline_layout != 0)
      {
        vkDestroyPipelineLayout(device, pipeline_layout, 0);
      }
      
      vkDestroyDevice(device, 0);
    }
    
    if (instance != 0)
    {
      vkDestroyInstance(instance, 0);
    }
    
    if (module != 0)
    {
      FreeLibrary(module);
    }
  }
  
  if (error.value == 0)
  {
    // NOTE(erb): set the global state
    w32_vulkan_state.module = module;
    w32_vulkan_state.instance = instance;
    w32_vulkan_state.physical_device = physical_device;
    w32_vulkan_state.device = device;
    w32_vulkan_state.queue_family_idx = queue_family_idx;
    w32_vulkan_state.queue = queue;
    w32_vulkan_state.pipeline_layout = pipeline_layout;
    w32_vulkan_state.vertex_shader_module = vertex_shader_module;
    w32_vulkan_state.fragment_shader_module = fragment_shader_module;
  }
  B32 result = (error.value == 0);
  return(result);
}

// NOTE(erb): copied from https://github.com/kladd/triangle/blob/master/src/main.c

B32 vulkan_create_semaphore(VkDevice device, VkSemaphore *out_semaphore)
{
  VkSemaphoreCreateInfo semaphore_info = {0};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  
  B32 result = vkCreateSemaphore(device, 
                                 &semaphore_info, 0,
                                 out_semaphore) == VK_SUCCESS;
  return result;
}

B32
gfx_vulkan_equip(GFX_Window handle)
{
  String8 error = {0};
  
  HINSTANCE instance = w32_state.instance;
  W32_Window *window_slot = w32_slot_from_gfx_handle(handle);
  HWND window = 0;
  if (gfx_window_is_valid(handle))
  {
    window = window_slot->hwnd;
  }
  else
  {
    error = S8("windows window is not valid");
  }
  
  //- create win32 surface
  VkSurfaceKHR surface = {0};
  if (error.value == 0)
  {
    // TODO(erb): pull this out into w32 vulkan function? 
    // it might be the only windows specific call
    
    VkWin32SurfaceCreateInfoKHR create_surface_info = {0};
    create_surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    create_surface_info.hwnd = window;
    create_surface_info.hinstance = instance;
    
    if (w32_vkCreateWin32SurfaceKHR(w32_vulkan_state.instance, 
                                    &create_surface_info, 
                                    0, &surface) != VK_SUCCESS) 
    {
      error = S8("failed to create windows vulkan surface");
    }
  }
  
  //- surface format
  VkSurfaceFormatKHR surface_format = {0};
  surface_format.format = VK_FORMAT_B8G8R8A8_SRGB;
  surface_format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  
  VkSurfaceCapabilitiesKHR surface_capabilities = {0};
  if (error.value == 0) 
  {
    VkResult capabilities_result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(w32_vulkan_state.physical_device, 
                                                                             surface, 
                                                                             &surface_capabilities);
    if (capabilities_result != VK_SUCCESS)
    {
      error = S8("failed to get surface capabilities");
    }
  }
  
  U32 present_mode_count = 0;
  if (error.value == 0)
  {
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(w32_vulkan_state.physical_device, surface,
                                                  &present_mode_count, 0) != VK_SUCCESS)
    {
      error = S8("failed to get surface present modes");
    }
  }
  
  VkPresentModeKHR present_mode = 0;
  if (error.value == 0)
  {
    Scratch scratch = scratch_begin(0, 0);
    VkPresentModeKHR *mode_arr = PushArray(scratch.arena, present_mode_count, VkPresentModeKHR);
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(w32_vulkan_state.physical_device, surface,
                                                  &present_mode_count,
                                                  mode_arr) != VK_SUCCESS)
    {
      error = S8("failed to get surface present modes");
    }
    
    if (error.value == 0)
    {
      //- mode priorities
      typedef struct Present_Mode_Choice Present_Mode_Choice;
      struct Present_Mode_Choice
      {
        VkPresentModeKHR mode;
        B32 exists;
      };
      
      Present_Mode_Choice mode_choices[] = 
      {
        {VK_PRESENT_MODE_FIFO_KHR, false},
        {VK_PRESENT_MODE_MAILBOX_KHR, false},
        {VK_PRESENT_MODE_FIFO_RELAXED_KHR, false},
        {VK_PRESENT_MODE_IMMEDIATE_KHR, false},
      };
      
      //- test all modes
      
      for (EachU32(mode_idx, present_mode_count))
      {
        VkPresentModeKHR mode = mode_arr[mode_idx];
        for (EachElement(Present_Mode_Choice, choice, mode_choices))
        {
          if (choice->mode == mode)
          {
            choice->exists = true;
            break;
          }
        }
      }
      
      //- pick the mode in priority order
      B32 found = false;
      for (EachElement(Present_Mode_Choice, choice, mode_choices))
      {
        if (choice->exists)
        {
          present_mode = choice->mode;
          found = true;
          break;
        }
      }
      
      if (!found)
      {
        error = S8("failed to pick a regular surface present mode");
      }
    }
    scratch_end(&scratch);
  }
  
  //- create swapchain
  VkSwapchainKHR swapchain = {0};
  if (error.value == 0)
  {
    VkSwapchainCreateInfoKHR swapchain_info = {0};
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageColorSpace = surface_format.colorSpace;
    swapchain_info.imageExtent = surface_capabilities.currentExtent;
    swapchain_info.imageFormat = surface_format.format;
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_info.minImageCount = surface_capabilities.minImageCount + 1;
    swapchain_info.oldSwapchain = VK_NULL_HANDLE;
    swapchain_info.preTransform = surface_capabilities.currentTransform;
    swapchain_info.presentMode = present_mode;
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface = surface;
    
    // NOTE(erb): expensive call
    if (vkCreateSwapchainKHR(w32_vulkan_state.device, 
                             &swapchain_info, 
                             0, &swapchain) != VK_SUCCESS)
    {
      error = S8("failed to create a vulkan swapchain");
    }
  }
  
  //- swapchain image count
  U32 image_count = 0;
  if (error.value == 0)
  {
    vkGetSwapchainImagesKHR(w32_vulkan_state.device, swapchain, &image_count, 0);
    if (image_count == 0)
    {
      error = S8("failed to get swapchain images count");
    }
    if (image_count >= W32_VULKAN_MAX_IMAGE_COUNT)
    {
      error = S8("too many frame buffers");
    }
  }
  
  Scratch buffers_scratch = scratch_begin(0, 0);
  
  //- create swapchain image views
  VkImageView *image_views = 0;
  if (error.value == 0)
  {
    Scratch scratch = scratch_begin(&buffers_scratch.arena, 1);
    VkImage *images = PushArray(scratch.arena, image_count, VkImage);
    VkResult result = vkGetSwapchainImagesKHR(w32_vulkan_state.device, swapchain, 
                                              &image_count, images);
    if ((result != VK_SUCCESS && result != VK_INCOMPLETE) ||
        image_count == 0)
    {
      error = S8("failed to get swapchain images");
    }
    
    if (error.value == 0)
    {
      image_views = PushArray(buffers_scratch.arena, image_count, VkImageView);
      for (EachU32(image_idx, image_count))
      {
        VkComponentMapping view_components = {0};
        view_components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        
        VkImageSubresourceRange view_resource_range = {0};
        view_resource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_resource_range.levelCount = 1;
        view_resource_range.layerCount = 1;
        
        VkImageViewCreateInfo view_info = {0};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image= images[image_idx];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = surface_format.format;
        view_info.components = view_components;
        view_info.subresourceRange = view_resource_range;
        
        VkImageView *view = (image_views + image_idx);
        result = vkCreateImageView(w32_vulkan_state.device, &view_info, 0, view);
        if (result != VK_SUCCESS) 
        {
          error = S8("failed to create image view");
          break;
        }
      }
    }
    scratch_end(&scratch);
  }
  
  VkPipelineShaderStageCreateInfo stage_create_infos[2] = {0};
  {
    VkPipelineShaderStageCreateInfo *vshader_info = stage_create_infos;
    vshader_info->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vshader_info->stage = VK_SHADER_STAGE_VERTEX_BIT;
    vshader_info->module = w32_vulkan_state.vertex_shader_module;
    vshader_info->pName = "main";
    
    VkPipelineShaderStageCreateInfo *fshader_info = (stage_create_infos + 1);
    fshader_info->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fshader_info->stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fshader_info->module = w32_vulkan_state.fragment_shader_module;
    fshader_info->pName = "main";
  }
  
  VkPipelineVertexInputStateCreateInfo vertex_input_state = {0};
  vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  
  VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {0};
  input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly_state.primitiveRestartEnable = VK_FALSE;
  
  VkPipelineViewportStateCreateInfo viewport_state = {0};
  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.viewportCount = 1;
  viewport_state.scissorCount = 1;
  
  VkDynamicState dynamic_states[] = 
  {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };
  
  VkPipelineDynamicStateCreateInfo dynamic_state = {0};
  dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state.dynamicStateCount = ArraySize(dynamic_states);
  dynamic_state.pDynamicStates = dynamic_states;
  
  VkPipelineRasterizationStateCreateInfo raster_state = {0};
  raster_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  raster_state.depthClampEnable = VK_FALSE;
  raster_state.rasterizerDiscardEnable = VK_FALSE;
  raster_state.polygonMode = VK_POLYGON_MODE_FILL;
  raster_state.lineWidth = 1.0f;
  raster_state.cullMode = VK_CULL_MODE_BACK_BIT;
  raster_state.frontFace = VK_FRONT_FACE_CLOCKWISE;
  raster_state.depthBiasEnable = VK_FALSE;
  
  VkPipelineMultisampleStateCreateInfo multisample_state = {0};
  multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisample_state.minSampleShading = 1.0f;
  
  VkPipelineColorBlendAttachmentState color_blend_attachment = {0};
  color_blend_attachment.blendEnable = VK_FALSE;
  color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
  color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
  color_blend_attachment.colorWriteMask = (VK_COLOR_COMPONENT_R_BIT|
                                           VK_COLOR_COMPONENT_G_BIT|
                                           VK_COLOR_COMPONENT_B_BIT|
                                           VK_COLOR_COMPONENT_A_BIT);
  
  VkPipelineColorBlendStateCreateInfo color_blend_state = {0};
  color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blend_state.attachmentCount = 1;
  color_blend_state.pAttachments = &color_blend_attachment;
  
  VkRect2D scissor = {0};
  scissor.extent = surface_capabilities.currentExtent;
  
  VkViewport viewport = {0};
  viewport.x = 0.0f;
  viewport.y = 0.0f,
  viewport.width = (F32)surface_capabilities.currentExtent.width;
  viewport.height = (F32)surface_capabilities.currentExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  
  VkRenderPass render_pass = {0};
  if (error.value == 0)
  {
    VkAttachmentDescription  color_attachment = {0};
    color_attachment.format = surface_format.format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentReference attachment_ref = {0};
    attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attachment_ref;
    
    VkSubpassDependency subpass_dependency = {0};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    VkRenderPassCreateInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &subpass_dependency;
    
    if (vkCreateRenderPass(w32_vulkan_state.device, 
                           &render_pass_info, 
                           0, &render_pass) != VK_SUCCESS) 
    {
      error = S8("failed to create render pass");
    }
  }
  
  VkPipeline graphics_pipeline = {0};
  if (error.value == 0)
  {
    VkGraphicsPipelineCreateInfo pipeline_info = {0};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = ArraySize(stage_create_infos);
    pipeline_info.pStages = stage_create_infos;
    pipeline_info.pVertexInputState = &vertex_input_state;
    pipeline_info.pInputAssemblyState = &input_assembly_state;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &raster_state;
    pipeline_info.pMultisampleState = &multisample_state;
    pipeline_info.pColorBlendState = &color_blend_state;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = w32_vulkan_state.pipeline_layout;
    pipeline_info.renderPass = render_pass;
    
    if (vkCreateGraphicsPipelines(w32_vulkan_state.device, 
                                  VK_NULL_HANDLE, 1,
                                  &pipeline_info, 0,
                                  &graphics_pipeline) != VK_SUCCESS) 
    {
      error = S8("failed to create graphics pipeline");
    }
  }
  
  VkFramebuffer *frame_buffers = 0;
  if (error.value == 0)
  {
    frame_buffers = PushArray(buffers_scratch.arena, image_count, VkFramebuffer);
    for (EachU32(image_idx, image_count))
    {
      VkImageView *image_view = (image_views + image_idx);
      
      VkFramebufferCreateInfo frame_buffer_info = {0};
      frame_buffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      frame_buffer_info.renderPass = render_pass;
      frame_buffer_info.attachmentCount = 1;
      frame_buffer_info.pAttachments = image_view;
      frame_buffer_info.width = surface_capabilities.currentExtent.width;
      frame_buffer_info.height = surface_capabilities.currentExtent.height;
      frame_buffer_info.layers = 1;
      
      VkFramebuffer *frame_buffer = (frame_buffers + image_idx);
      if (vkCreateFramebuffer(w32_vulkan_state.device, 
                              &frame_buffer_info, 
                              0, frame_buffer) != VK_SUCCESS) {
        error = S8("failed to create frame buffer");
        break;
      }
    }
  }
  
  VkCommandPool command_pool = {0};
  if (error.value == 0)
  {
    VkCommandPoolCreateInfo command_pool_info = {0};
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_info.queueFamilyIndex = w32_vulkan_state.queue_family_idx;
    
    if (vkCreateCommandPool(w32_vulkan_state.device, 
                            &command_pool_info, 
                            0, &command_pool) != VK_SUCCESS) {
      error = S8("failed to create command pool");
    }
  }
  
  VkCommandBuffer command_buffer = {0};
  if (error.value == 0)
  {
    VkCommandBufferAllocateInfo command_buffer_alloc = {0};
    command_buffer_alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc.commandPool = command_pool;
    command_buffer_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_alloc.commandBufferCount = 1;
    
    if (vkAllocateCommandBuffers(w32_vulkan_state.device, 
                                 &command_buffer_alloc,
                                 &command_buffer) != VK_SUCCESS) {
      error = S8("failed to allocate command buffer");
    }
  }
  
  VkSemaphore image_available_sem = {0};
  if (error.value == 0)
  {
    if (!vulkan_create_semaphore(w32_vulkan_state.device, &image_available_sem))
    {
      error = S8("failed to create image semaphore");
    }
  }
  
  VkSemaphore render_finished_sem = {0};
  if (error.value == 0)
  {
    if (!vulkan_create_semaphore(w32_vulkan_state.device, &render_finished_sem))
    {
      error = S8("failed to create render finished semaphore");
    }
  }
  
  VkFence in_flight_fence = {0};
  if (error.value == 0)
  {
    VkFenceCreateInfo fence_info = {0};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    if (vkCreateFence(w32_vulkan_state.device, 
                      &fence_info, 0, 
                      &in_flight_fence) != VK_SUCCESS)
    {
      error = S8("failed to create fence");
    }
  }
  
  if (error.value != 0)
  {
    err_set_error(error);
    // TODO(erb): cleanup all resources that have been created
  }
  
  if (error.value == 0)
  {
    window_slot->release = gfx_vulkan_destroy;
    
    W32_Vulkan_Window *vk = w32_vulkan_slot_from_gfx_handle(handle);
    
    vk->window = window;
    vk->command_buffer = command_buffer;
    vk->swapchain = swapchain;
    vk->render_pass = render_pass;
    vk->graphics_pipeline = graphics_pipeline;
    vk->surface_capabilities = surface_capabilities;
    
    vk->in_flight_fence = in_flight_fence;
    vk->image_available_sem = image_available_sem;
    vk->render_finished_sem = render_finished_sem;
    
    vk->image_count = image_count;
    ArrayCopy(image_views, vk->image_views, image_count);
    ArrayCopy(frame_buffers, vk->frame_buffers, image_count);
    
    vk->viewport = viewport;
    vk->scissor = scissor;
    
    vk->command_pool = command_pool;
    vk->surface = surface;
  }
  
  scratch_end(&buffers_scratch);
  
  B32 result = (error.value == 0);
  return(result);
}

void
gfx_vulkan_render_begin(GFX_Window handle)
{
  String8 error = {0};
  
  W32_Vulkan_Window *vk = 0; 
  if (gfx_window_is_valid(handle))
  {
    vk = w32_vulkan_slot_from_gfx_handle(handle);
  }
  else
  {
    error = S8("invalid window handle");
  }
  
  if (vk)
  {
    vkWaitForFences(w32_vulkan_state.device, 1, &vk->in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(w32_vulkan_state.device, 1, &vk->in_flight_fence);
    
    vk->current_image_idx = 0;
    vkAcquireNextImageKHR(w32_vulkan_state.device, vk->swapchain, UINT64_MAX,
                          vk->image_available_sem, VK_NULL_HANDLE, 
                          &vk->current_image_idx);
    vkResetCommandBuffer(vk->command_buffer, 0);
    
    VkCommandBufferBeginInfo command_buffer_begin = {0};
    command_buffer_begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    
    if (vkBeginCommandBuffer(vk->command_buffer, &command_buffer_begin) != VK_SUCCESS) 
    {
      error = S8("failed to begin command buffer");
    }
  }
  
  if (error.value != 0)
  {
    err_set_error(error);
  }
}

void
gfx_vulkan_render_end(GFX_Window handle)
{
  String8 error = {0};
  
  W32_Vulkan_Window *vk = 0; 
  if (gfx_window_is_valid(handle))
  {
    vk = w32_vulkan_slot_from_gfx_handle(handle);
  }
  else
  {
    error = S8("invalid window handle");
  }
  
  if (vk)
  {
    V4f clear_color = color_debug;
    
    VkClearValue vk_clear = {0};
    vk_clear.color.float32[0] = clear_color.r;
    vk_clear.color.float32[1] = clear_color.g;
    vk_clear.color.float32[2] = clear_color.b;
    vk_clear.color.float32[3] = clear_color.a;
    
    VkRenderPassBeginInfo render_pass_begin = {0};
    render_pass_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin.renderPass = vk->render_pass;
    render_pass_begin.framebuffer = vk->frame_buffers[vk->current_image_idx];
    render_pass_begin.renderArea.extent = vk->surface_capabilities.currentExtent;
    render_pass_begin.clearValueCount = 1;
    render_pass_begin.pClearValues = &vk_clear;
    
    vkCmdBeginRenderPass(vk->command_buffer, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);
    {
      vkCmdBindPipeline(vk->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->graphics_pipeline);
      vkCmdSetViewport(vk->command_buffer, 0, 1, &vk->viewport);
      vkCmdSetScissor(vk->command_buffer, 0, 1, &vk->scissor);
      vkCmdDraw(vk->command_buffer, 3, 1, 0, 0);
    }
    vkCmdEndRenderPass(vk->command_buffer);
    
    if (vkEndCommandBuffer(vk->command_buffer) != VK_SUCCESS) 
    {
      error = S8("failed to end command buffer");
    }
    
    VkSemaphore wait_semaphores[] = 
    {
      vk->image_available_sem,
    };
    VkSemaphore signal_semaphores[] = 
    {
      vk->render_finished_sem,
    };
    
    if (error.value == 0)
    {
      
      VkPipelineStageFlags wait_stages[] = 
      {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      };
      
      VkSubmitInfo submit_info = {0};
      submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submit_info.pWaitDstStageMask = wait_stages;
      
      submit_info.waitSemaphoreCount = ArraySize(wait_semaphores);
      submit_info.pWaitSemaphores = wait_semaphores;
      
      submit_info.signalSemaphoreCount = ArraySize(signal_semaphores);
      submit_info.pSignalSemaphores = signal_semaphores;
      
      submit_info.commandBufferCount = 1;
      submit_info.pCommandBuffers = &vk->command_buffer;
      
      if (vkQueueSubmit(w32_vulkan_state.queue, 1, 
                        &submit_info, 
                        vk->in_flight_fence) !=VK_SUCCESS) 
      {
        error = S8("queue submit failed");
      }
    }
    if (error.value == 0)
    {
      VkSwapchainKHR swaps[] = 
      {
        vk->swapchain,
      };
      VkPresentInfoKHR present_info = {0};
      present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
      present_info.waitSemaphoreCount = ArraySize(signal_semaphores);
      present_info.pWaitSemaphores = signal_semaphores;
      present_info.swapchainCount = ArraySize(swaps);
      present_info.pSwapchains = swaps;
      present_info.pImageIndices = &vk->current_image_idx;
      vkQueuePresentKHR(w32_vulkan_state.queue, &present_info);
    }
  }
  
  if (error.value != 0)
  {
    err_set_error(error);
  }
}


B32
gfx_vulkan_destroy(GFX_Window handle)
{
  B32 result = false;
  if (gfx_window_is_valid(handle))
  {
    result = true;
    W32_Vulkan_Window *vk = w32_vulkan_slot_from_gfx_handle(handle);
    vkDeviceWaitIdle(w32_vulkan_state.device);
    
    vkDestroySemaphore(w32_vulkan_state.device, vk->image_available_sem, 0);
    vkDestroySemaphore(w32_vulkan_state.device, vk->render_finished_sem, 0);
    vkDestroyFence(w32_vulkan_state.device, vk->in_flight_fence, 0);
    vkDestroyCommandPool(w32_vulkan_state.device, vk->command_pool, 0);
    
    for (EachU32(image_idx, vk->image_count)) 
    {
      VkFramebuffer buffer = vk->frame_buffers[image_idx];
      vkDestroyFramebuffer(w32_vulkan_state.device, buffer, 0);
    }
    
    vkDestroyPipeline(w32_vulkan_state.device, vk->graphics_pipeline, 0);
    vkDestroyRenderPass(w32_vulkan_state.device, vk->render_pass, 0);
    
    for (EachU32(image_idx, vk->image_count)) 
    {
      VkImageView view = vk->image_views[image_idx];
      vkDestroyImageView(w32_vulkan_state.device, view, 0);
    }
    vkDestroySwapchainKHR(w32_vulkan_state.device, vk->swapchain, 0);
    vkDestroySurfaceKHR(w32_vulkan_state.instance, vk->surface, 0);
    
    ZeroStruct(vk);
  }
  return(result);
}

void
gfx_vulkan_release()
{
  vkDestroyShaderModule(w32_vulkan_state.device, w32_vulkan_state.vertex_shader_module, 0);
  vkDestroyShaderModule(w32_vulkan_state.device, w32_vulkan_state.fragment_shader_module, 0);
  vkDestroyPipelineLayout(w32_vulkan_state.device, w32_vulkan_state.pipeline_layout, 0);
  vkDestroyDevice(w32_vulkan_state.device, 0);
  vkDestroyInstance(w32_vulkan_state.instance, 0);
  FreeLibrary(w32_vulkan_state.module);
  
  ZeroStruct(&w32_vulkan_state);
}
