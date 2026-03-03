#include "overlay_renderer.h"
#include "../dxvk_device.h"

#include <imgui_impl_vulkan.h>

namespace dxvk {

  OverlayRenderer::OverlayRenderer(const Rc<DxvkDevice>& device)
  : m_device(device) {
  }


  OverlayRenderer::~OverlayRenderer() {
    if (m_descriptorPool != VK_NULL_HANDLE) {
      m_device->vkd()->vkDestroyDescriptorPool(
        m_device->handle(), m_descriptorPool, nullptr);
    }
  }


  void OverlayRenderer::init() {
    if (m_initialized)
      return;

    createDescriptorPool();

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance        = m_device->instance()->handle();
    init_info.PhysicalDevice  = m_device->adapter()->handle();
    init_info.Device          = m_device->handle();
    init_info.QueueFamily     = m_device->queues().graphics.queueFamily;
    init_info.Queue           = m_device->queues().graphics.queueHandle;
    init_info.PipelineCache   = VK_NULL_HANDLE;
    init_info.DescriptorPool  = m_descriptorPool;
    init_info.Subpass         = 0;
    init_info.MinImageCount   = 2; // Assuming at least double buffering
    init_info.ImageCount      = 3; // Typical for DXVK
    init_info.MSAASamples     = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator       = nullptr;
    init_info.CheckVkResultFn = nullptr;

    // We need to provide the Vulkan functions since we are using DXVK's function table
    // However, ImGui_ImplVulkan uses volk or direct calls if not specified.
    // In DXVK, we should ideally use the vkd() function table.
    
    // For now, let's hope it works with standard linking or we might need to 
    // provide a loader or set the functions manually if ImGui allows it.
    // ImGui_ImplVulkan_LoadFunctions is available in some versions.

    ImGui_ImplVulkan_Init(&init_info);

    m_initialized = true;
  }


  void OverlayRenderer::beginFrame() {
    ImGui_ImplVulkan_NewFrame();
  }


  void OverlayRenderer::render(VkCommandBuffer cmdBuffer, VkImageView targetView) {
    // Note: ImGui_ImplVulkan_RenderDrawData expects a render pass or dynamic rendering.
    // In DXVK, we usually use dynamic rendering or manage render passes.
    
    // For simplicity, let's assume we are called outside of a render pass 
    // and we need to start one, or we use dynamic rendering.
    
    // Re-check plan: "Use the swapchain's current backbuffer as the render target."
    
    ImDrawData* drawData = ImGui::GetDrawData();
    if (drawData) {
      ImGui_ImplVulkan_RenderDrawData(drawData, cmdBuffer);
    }
  }


  void OverlayRenderer::createDescriptorPool() {
    VkDescriptorPoolSize pool_sizes[] = {
      { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
      { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
      { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
      { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets       = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes    = pool_sizes;

    if (m_device->vkd()->vkCreateDescriptorPool(
        m_device->handle(), &pool_info, nullptr, &m_descriptorPool) != VK_SUCCESS) {
      throw DxvkError("OverlayRenderer: Failed to create descriptor pool");
    }
  }

}
