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

X(vkGetPhysicalDeviceSurfaceCapabilitiesKHR, VkResult, (VkPhysicalDevice physical_device,
                                                        VkSurfaceKHR surface,
                                                        VkSurfaceCapabilitiesKHR* surface_capabilities))

X(vkCreateSwapchainKHR, VkResult, (VkDevice device,
                                   const VkSwapchainCreateInfoKHR* create_info,
                                   const VkAllocationCallbacks* allocator,
                                   VkSwapchainKHR* swap_chain))

X(vkGetDeviceQueue, void, (VkDevice device, 
                           uint32_t queue_familiy_index,
                           uint32_t queue_index, 
                           VkQueue* queue))

X(vkGetSwapchainImagesKHR, VkResult, (VkDevice device, 
                                      VkSwapchainKHR swap_chain,
                                      uint32_t* image_count,
                                      VkImage* images))

X(vkCreateImageView, VkResult, (VkDevice device,
                                const VkImageViewCreateInfo* create_info,
                                const VkAllocationCallbacks* allocator,
                                VkImageView* view))

X(vkCreateShaderModule, VkResult, (VkDevice device,
                                   const VkShaderModuleCreateInfo* create_info,
                                   const VkAllocationCallbacks* allocator,
                                   VkShaderModule* shader_module))

X(vkCreatePipelineLayout, VkResult, (VkDevice device,
                                     const VkPipelineLayoutCreateInfo* create_info,
                                     const VkAllocationCallbacks* allocator,
                                     VkPipelineLayout* pipeline_layout))

X(vkCreateRenderPass, VkResult, (VkDevice device,
                                 const VkRenderPassCreateInfo* create_info,
                                 const VkAllocationCallbacks* allocator,
                                 VkRenderPass* render_pass))

X(vkCreateGraphicsPipelines, VkResult, (VkDevice device,
                                        VkPipelineCache pipeline_cache,
                                        uint32_t create_info_count,
                                        const VkGraphicsPipelineCreateInfo* create_infos,
                                        const VkAllocationCallbacks* allocator,
                                        VkPipeline* pipelines))

X(vkGetPhysicalDeviceSurfacePresentModesKHR, VkResult,(VkPhysicalDevice physical_device,
                                                       VkSurfaceKHR surface,
                                                       uint32_t* present_mode_count,
                                                       VkPresentModeKHR* present_modes))


X(vkCreateFramebuffer, VkResult, (VkDevice device,
                                  const VkFramebufferCreateInfo* create_info,
                                  const VkAllocationCallbacks* allocator,
                                  VkFramebuffer* frame_buffer))

X(vkCreateCommandPool, VkResult, (VkDevice device,
                                  const VkCommandPoolCreateInfo* create_info,
                                  const VkAllocationCallbacks* allocator,
                                  VkCommandPool* command_pool))

X(vkAllocateCommandBuffers, VkResult, (VkDevice device,
                                       const VkCommandBufferAllocateInfo* allocator_info,
                                       VkCommandBuffer* command_buffers))

X(vkCreateSemaphore, VkResult, (VkDevice device,
                                const VkSemaphoreCreateInfo* create_info,
                                const VkAllocationCallbacks* allocator,
                                VkSemaphore* semaphore))

X(vkCreateFence, VkResult, (VkDevice device,
                            const VkFenceCreateInfo* create_info,
                            const VkAllocationCallbacks* allocator,
                            VkFence* fence))

X(vkResetFences, VkResult, (VkDevice device,
                            uint32_t fence_count,
                            const VkFence* fences))

X(vkWaitForFences, VkResult, (VkDevice device,
                              uint32_t fence_count,
                              const VkFence* fences,
                              VkBool32 wait_all,
                              uint64_t timeout))

X(vkAcquireNextImageKHR, VkResult, (VkDevice device,
                                    VkSwapchainKHR swapchain,
                                    uint64_t timeout,
                                    VkSemaphore semaphore,
                                    VkFence fence,
                                    uint32_t *image_index))

X(vkResetCommandBuffer, VkResult, (VkCommandBuffer command_buffer,
                                   VkCommandBufferResetFlags flags))

X(vkBeginCommandBuffer, VkResult, (VkCommandBuffer command_buffer,
                                   const VkCommandBufferBeginInfo* begin_info))

X(vkCmdBeginRenderPass, void, (VkCommandBuffer commandBuffer,
                               const VkRenderPassBeginInfo* pRenderPassBegin,
                               VkSubpassContents contents))


X(vkCmdBindPipeline, void, (VkCommandBuffer command_buffer,
                            VkPipelineBindPoint pipelineBindPoint,
                            VkPipeline pipeline))


X(vkCmdSetViewport, void, (VkCommandBuffer command_buffer,
                           uint32_t first_viewport,
                           uint32_t viewport_count,
                           const VkViewport* viewports))


X(vkCmdSetScissor, void, (VkCommandBuffer command_buffer,
                          uint32_t first_scissor,
                          uint32_t scissor_count,
                          const VkRect2D* scissors))


X(vkCmdDraw, void, (VkCommandBuffer command_buffer,
                    uint32_t vertex_count,
                    uint32_t instance_count,
                    uint32_t first_vertex,
                    uint32_t first_instance))

X(vkCmdEndRenderPass, void, (VkCommandBuffer command_buffer))

X(vkEndCommandBuffer, VkResult, (VkCommandBuffer command_buffer))

X(vkQueueSubmit, VkResult, (VkQueue queue,
                            uint32_t submit_count,
                            const VkSubmitInfo *submits,
                            VkFence fence))

X(vkQueuePresentKHR, VkResult, (VkQueue queue,
                                const VkPresentInfoKHR *pPresentInfo))


X(vkDeviceWaitIdle, VkResult, (VkDevice device))


X(vkDestroySemaphore, void, (VkDevice device,
                             VkSemaphore semaphore,
                             const VkAllocationCallbacks* allocator))

X(vkDestroyFence, void, (VkDevice device,
                         VkFence fence,
                         const VkAllocationCallbacks* allocator))

X(vkDestroyCommandPool, void, (VkDevice device,
                               VkCommandPool command_pool,
                               const VkAllocationCallbacks* allocator))

X(vkDestroyFramebuffer, void, (VkDevice device,
                               VkFramebuffer frame_buffer,
                               const VkAllocationCallbacks* allocator))

X(vkDestroyPipeline, void, (VkDevice device,
                            VkPipeline pipeline,
                            const VkAllocationCallbacks* allocator))

X(vkDestroyRenderPass, void, (VkDevice device,
                              VkRenderPass render_pass,
                              const VkAllocationCallbacks* allocator))

X(vkDestroyPipelineLayout, void, (VkDevice device,
                                  VkPipelineLayout pipeline_layout,
                                  const VkAllocationCallbacks* allocator))

X(vkDestroyShaderModule, void, (VkDevice device,
                                VkShaderModule shader_module,
                                const VkAllocationCallbacks* allocator))

X(vkDestroyImageView, void, (VkDevice device,
                             VkImageView image_view,
                             const VkAllocationCallbacks* allocator))

X(vkDestroySwapchainKHR, void, (VkDevice device,
                                VkSwapchainKHR swapchain,
                                const VkAllocationCallbacks* allocator))

X(vkDestroySurfaceKHR, void, (VkInstance instance,
                              VkSurfaceKHR surface,
                              const VkAllocationCallbacks* allocator))

X(vkDestroyDevice, void, (VkDevice device,
                          const VkAllocationCallbacks* allocator))

X(vkDestroyInstance, void, (VkInstance instance,
                            const VkAllocationCallbacks* allocator))

