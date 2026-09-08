/* date = April 26th 2026 6:19 pm */

#ifndef WIN32_VULKAN_H
#define WIN32_VULKAN_H

#define W32_VULKAN_MAX_IMAGE_COUNT 8

typedef struct W32_Vulkan_Window W32_Vulkan_Window;
struct W32_Vulkan_Window
{
  HWND window;
  
  VkCommandBuffer command_buffer;
  VkSwapchainKHR swapchain;
  VkRenderPass render_pass;
  VkPipeline graphics_pipeline;
  VkSurfaceCapabilitiesKHR surface_capabilities;
  
  VkFence in_flight_fence;
  VkSemaphore image_available_sem;
  VkSemaphore render_finished_sem;
  
  U32 image_count;
  VkFramebuffer frame_buffers[W32_VULKAN_MAX_IMAGE_COUNT];
  VkImageView image_views[W32_VULKAN_MAX_IMAGE_COUNT];
  
  VkCommandPool command_pool;
  VkSurfaceKHR surface;
  
  VkViewport viewport;
  VkRect2D scissor;
  U32 current_image_idx;
};

typedef struct W32_Vulkan_State W32_Vulkan_State;
struct W32_Vulkan_State
{
  HMODULE module;
  VkInstance instance;
  VkPhysicalDevice physical_device;
  VkDevice device;
  
  // NOTE(erb): device specific
  U32 queue_family_idx;
  VkQueue queue;
  VkPipelineLayout pipeline_layout;
  VkShaderModule vertex_shader_module;
  VkShaderModule fragment_shader_module;
};

//////////////////////////////////////////
// NOTE(erb): vulkan functions

#define VK_KHR_WIN32_SURFACE_EXTENSION_NAME "VK_KHR_win32_surface"
typedef VkFlags VkWin32SurfaceCreateFlagsKHR;
typedef struct VkWin32SurfaceCreateInfoKHR {
  VkStructureType                 sType;
  const void*                     pNext;
  VkWin32SurfaceCreateFlagsKHR    flags;
  HINSTANCE                       hinstance;
  HWND                            hwnd;
} VkWin32SurfaceCreateInfoKHR;

// NOTE(erb): vulkan function types
#define X(NAME, RETURN, PARAMS) typedef RETURN W32_##NAME PARAMS;
#include "win32_vulkan_functions_x_list.h"
#undef X

// NOTE(erb): vulkan function pointers
#define X(NAME, RETURN, PARAMS) global W32_##NAME *w32_##NAME = 0;
#include "win32_vulkan_functions_x_list.h"
#undef X

#endif //WIN32_VULKAN_H
