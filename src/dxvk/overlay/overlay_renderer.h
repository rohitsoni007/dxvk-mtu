#pragma once

#include "../dxvk_include.h"
#include <imgui_impl_vulkan.h>

namespace dxvk {

  class DxvkDevice;

  /**
   * \brief DXVK ImGui Overlay Renderer
   * 
   * Handles Vulkan-specific ImGui rendering.
   */
  class OverlayRenderer : public RcObject {
  public:
    OverlayRenderer(const Rc<DxvkDevice>& device);
    ~OverlayRenderer();

    /**
     * \brief Initialize Vulkan backend for ImGui
     */
    void init();

    /**
     * \brief Start new Vulkan frame for ImGui
     */
    void beginFrame();

    /**
     * \brief Render ImGui draw data to command buffer
     * \param [in] cmdBuffer Command buffer to record to
     * \param [in] targetView Target swapchain image view
     */
    void render(VkCommandBuffer cmdBuffer, VkImageView targetView);

  private:
    Rc<DxvkDevice> m_device;
    
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    bool m_initialized = false;

    void createDescriptorPool();
  };

}
