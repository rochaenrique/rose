/* date = April 26th 2026 6:19 pm */

#ifndef WIN32_VULKAN_H
#define WIN32_VULKAN_H

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
