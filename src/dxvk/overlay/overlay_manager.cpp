#include "overlay_manager.h"
#include "overlay_renderer.h"
#include "overlay_input.h"
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_win32.h>

namespace dxvk {

  OverlayManager::OverlayManager(const Rc<DxvkDevice>& device)
  : m_device(device) {
    m_renderer = new OverlayRenderer(device);
    m_input    = new OverlayInput();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    ImGui::StyleColorsDark();
  }


  OverlayManager::~OverlayManager() {
    shutdown();
    ImGui::DestroyContext();
  }


  void OverlayManager::init(HWND window) {
    if (m_initialized)
      return;

    m_input->init(window);
    m_renderer->init();

    m_initialized = true;
  }


  bool OverlayManager::processMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (!m_initialized)
      return false;

    if (msg == WM_KEYDOWN && wParam == VK_F11) {
      toggle();
      return true;
    }

    if (m_visible)
      return m_input->processMessage(hWnd, msg, wParam, lParam);

    return false;
  }


  void OverlayManager::shutdown() {
    if (!m_initialized)
      return;

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplWin32_Shutdown();
    
    m_initialized = false;
  }


  void OverlayManager::beginFrame() {
    if (!m_initialized || !m_visible)
      return;

    m_renderer->beginFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Default UI for testing
    ImGui::Begin("DXVK + ImGui Overlay");
    ImGui::Text("Render Backend: Vulkan");
    ImGui::Text("Target: D3D9 x32");
    
    if (ImGui::Button("Toggle Visibility (F11)")) {
      toggle();
    }
    
    ImGui::End();
  }


  void OverlayManager::endFrame(VkCommandBuffer cmdBuffer, VkImageView targetView) {
    if (!m_initialized || !m_visible)
      return;

    ImGui::Render();
    m_renderer->render(cmdBuffer, targetView);
  }


  void OverlayManager::toggle() {
    m_visible = !m_visible;
  }

}
