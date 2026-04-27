X(vkCreateInstance, VkResult, (const VkInstanceCreateInfo *create_info, 
                               const VkAllocationCallbacks *allocator, 
                               VkInstance* instance))

X(vkEnumeratePhysicalDevices, VkResult, (VkInstance instance, 
                                         U32* physical_device_count, 
                                         VkPhysicalDevice *physical_device))

X(vkGetPhysicalDeviceProperties2, void, (VkPhysicalDevice physical_device, 
                                         VkPhysicalDeviceProperties2 *properties))

X(vkGetPhysicalDeviceQueueFamilyProperties, void, (VkPhysicalDevice physical_device,
                                                   U32 *queue_family_count,
                                                   VkQueueFamilyProperties *queue_family_properties))

X(vkCreateDevice, VkResult, (VkPhysicalDevice physical_device, 
                             const VkDeviceCreateInfo* create_info, 
                             const VkAllocationCallbacks* allocator, 
                             VkDevice* device))
