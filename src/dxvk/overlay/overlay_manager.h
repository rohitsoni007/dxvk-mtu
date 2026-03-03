#pragma once

#include "../dxvk_device.h"
#include <imgui.h>

namespace dxvk {

  class OverlayRenderer;
  class OverlayInput;

  /**
   * \brief DXVK ImGui Overlay Manager
   * 
   * Manages the lifecycle of the ImGui context
   * and coordinates rendering and input.
   */
  class OverlayManager : public RcObject {
  public:
    OverlayManager(const Rc<DxvkDevice>& device);
    ~OverlayManager();

    /**
     * \brief Initialize the overlay for a specific window
     * \param [in] window The window handle
     */
    void init(HWND window);

    /**
     * \brief Process Win32 message
     * \returns true if message was handled
     */
    bool processMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /**
     * \brief Update and begin a new ImGui frame
     */
    void beginFrame();

    /**
     * \brief Render the ImGui frame to a command buffer
     * \param [in] cmdBuffer The Vulkan command buffer
     * \param [in] targetView The target image view (swapchain backbuffer)
     */
    void endFrame(VkCommandBuffer cmdBuffer, VkImageView targetView);

    /**
     * \brief Toggle overlay visibility
     */
    void toggle();

    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }

  private:
    Rc<DxvkDevice>      m_device;
    Rc<OverlayRenderer> m_renderer;
    Rc<OverlayInput>    m_input;

    bool m_visible = true;
    bool m_initialized = false;
  };

}
