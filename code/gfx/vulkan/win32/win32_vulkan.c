
// TODO(erb): fix error handling here, this should not exit the process, 
// it should allow the caller to handle the error
void 
w32_init_vulkan(HINSTANCE instance)
{
  String error = {0};
  
  HMODULE vk_module = LoadLibraryA("vulkan-1.dll");
  if (vk_module == 0)
  {
    error = S("failed to initialize vulkan-1.dll");
  }
  
  //- load vulkan functions
  if (error.value == 0) 
  {
#define X(NAME, RETURN, PARAMS) GET_PROC_ADDR(vk_module, NAME, #NAME);
#include "gfx/vulkan/vulkan_functions_x_list.h"
#undef X
    
#define X(NAME, RETURN, PARAMS) if (NAME == 0) { error = S("missing vulkan function "## #NAME); }
#include "gfx/vulkan/vulkan_functions_x_list.h"
#undef X
  }
  
  //- load vulkan w32 functions
  if (error.value == 0) 
  {
#define X(NAME, RETURN, PARAMS) GET_PROC_ADDR(vk_module, w32_##NAME, #NAME);
#include "gfx/vulkan/win32/win32_vulkan_functions_x_list.h"
#undef X
    
#define X(NAME, RETURN, PARAMS) if (w32_##NAME == 0) { error = S("missing vulkan function "## #NAME); }
#include "gfx/vulkan/win32/win32_vulkan_functions_x_list.h"
#undef X
  }
  
  
  if (error.length != 0) 
  {
    char msg[64];
    sprintf_s(msg, sizeof(msg), "%.*s\n", str_expand(error));
    OutputDebugStringA(msg);
    ExitProcess(1);
  }
}

// NOTE(erb): copied from https://github.com/kladd/triangle/blob/master/src/main.c

// TODO(erb): fix error handling here, this should not exit the process, 
// it should allow the caller to handle the error

// TODO(erb): pull out win32 window creation into outside function
HWND 
w32_make_vulkan_window(HINSTANCE instance)
{
  String error = {0};
  
  //- create win32 window class
  char *window_class_name = "marker_window_class";
  {
    
    WNDCLASSA window_class = {0};
    //window_class.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    window_class.hInstance = instance;
    window_class.lpfnWndProc = DefWindowProc;
    window_class.lpszClassName = window_class_name;
    ATOM atom = RegisterClass(&window_class);
    if (atom == 0) 
    {
      error = S("failed to create window class");
    }
    
  }
  
  //- create win32 window
  HWND window = 0;
  if (error.value == 0)
  {
    window = CreateWindowA(window_class_name,
                           "Marker",
                           WS_TILEDWINDOW, 
                           CW_USEDEFAULT, CW_USEDEFAULT, // x, y
                           CW_USEDEFAULT, CW_USEDEFAULT, // w, h
                           0, 0, instance, 0); // parent, menu, instance, param
    if (window == INVALID_HANDLE_VALUE) 
    {
      error = S("failed to create window");
    }
  }
  
  //- create vulkan instance
  VkInstance vulkan_instance = 0;
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
    
    VkInstance vulkan;
    if (vkCreateInstance(&instance_info, 0, &vulkan) != VK_SUCCESS) 
    {
      error = S("failed to create vulkan instance");
    }
    vulkan_instance = vulkan;
  }
  
  //- get physical device count
  U32 physical_device_count = 0;
  if (error.value == 0) 
  {
    vkEnumeratePhysicalDevices(vulkan_instance, &physical_device_count, 0);
    if (physical_device_count == 0)
    {
      error = S("no physical devices");
    }
    if (physical_device_count != 1)
    {
      error = S("more than one physical devices");
    }
  }
  
  //- get phyiscal device
  VkPhysicalDevice physical_device = {0};
  if (error.value == 0) 
  {
    vkEnumeratePhysicalDevices(vulkan_instance, &physical_device_count, &physical_device);
    if (physical_device == 0)
    {
      error = S("failed to load vulkan physical device");
    }
  }
  
#if 0
  // NOTE(erb): this isnt used for anything but it gathers all
  // of the info of a pyhiscal device, it might be usefull later
  if (error.value == 0) 
  {
    scratch_block(scratch, 0, 0)
    {
      VkPhysicalDeviceProperties2 *device_properties = push_struct(scratch.arena, VkPhysicalDeviceProperties2);
      vkGetPhysicalDeviceProperties2(physical_device, device_properties);
    }
  }
#endif
  
  
  //- get queue family count
  U32 queue_family_count = 0;
  if (error.value == 0)
  {
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, 0);
    if (queue_family_count == 0) 
    {
      error = S("failed to load physical device queue family");
    }
  }
  
  //- get queue famliy index for device
  U32 queue_idx = 0;
  if (error.value == 0)
  {
    scratch_block(scratch, 0, 0)
    {
      VkQueueFamilyProperties *queue_families = push_array(scratch.arena, queue_family_count, VkQueueFamilyProperties); 
      vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families);
      if (queue_family_count == 0)
      {
        error = S("failed to load physical device queue family properties");
      }
      if (error.value == 0)
      {
        for (U32 queue_family_idx = 0;
             queue_family_idx < queue_family_count;
             queue_family_idx++)
        {
          VkQueueFamilyProperties *queue = queue_families + queue_family_idx;
          if (queue->queueFlags & VK_QUEUE_GRAPHICS_BIT)
          {
            queue_idx = queue_family_idx;
            break;
          }
        }
        
        if (queue_idx == queue_family_count)
        {
          error = S("failed to load a graphics queue family");
        }
      }
    }
  }
  
  //- create logical device
  VkDevice device = {0};
  if (error.value == 0)
  {
    F32 queue_graphics_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = {0};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = queue_idx;
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
    
    if (vkCreateDevice(physical_device, 
                       &device_info, 
                       0, &device) != VK_SUCCESS) {
      error = S("failed to create logical device");
    }
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
    
    if (w32_vkCreateWin32SurfaceKHR(vulkan_instance, 
                                    &create_surface_info, 
                                    0, &surface) != VK_SUCCESS) 
    {
      error = S("failed to create windows vulkan surface");
    }
  }
  
  // TODO(erb): FINISH
  
  if (error.value != 0) 
  {
    char msg[64];
    sprintf_s(msg, sizeof(msg), "%.*s\n", str_expand(error));
    OutputDebugStringA(msg);
  }
  
  return(window);
}
