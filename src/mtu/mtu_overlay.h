#pragma once

#include <vector>
#include <mutex>
#include <memory>
#include <functional>

#include "../dxvk/dxvk_device.h"
#include "../dxvk/dxvk_context.h"
#include "../dxvk/hud/dxvk_hud_renderer.h"

#include <windows.h>
#include "../d3d9/mtu_plugin_loader.h"

#include "imgui/imgui.h"

namespace dxvk {

  /**
   * \brief MTU Overlay Management
   * 
   * Handles ImGui lifecycle, Vulkan backend, and UI logic
   * for the in-game configuration overlay.
   */
  class MtuOverlay : public RcObject {
  public:
    MtuOverlay(const Rc<DxvkDevice>& device, HWND window);
    ~MtuOverlay();

    /**
     * \brief Update overlay state
     */
    void update();

    /**
     * \brief Render the overlay
     * 
     * \param [in] ctx Command list to record to
     * \param [in] dstView Final swapchain image view
     */
    void render(
      const DxvkContextObjects& ctx,
      const Rc<DxvkImageView>&  dstView);

    /**
     * \brief Process window messages for ImGui
     */
    bool processMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /**
     * \brief Toggle overlay visibility
     */
    void toggleVisibility() { m_visible = !m_visible; }
    
    bool isVisible() const { return m_visible; }

    void syncConfigFromPlugin();
    void syncConfigToPlugin();

    void setMipBiasCallback(std::function<void(float)> callback) {
      m_onMipBiasChange = std::move(callback);
    }

  private:
    Rc<DxvkDevice>    m_device;
    HWND              m_window;
    bool              m_visible = false;
    bool              m_initialized = false;
    
    MtuConfig         m_config;

    std::function<void(float)> m_onMipBiasChange;

    // ImGui state
    VkDescriptorPool  m_descriptorPool = VK_NULL_HANDLE;

    void init(const DxvkContextObjects& ctx, const Rc<DxvkImageView>& dstView);
    void setupStyle();
    void renderUI();
  };

}
