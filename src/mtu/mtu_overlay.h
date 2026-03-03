#pragma once

#include "../dxvk/dxvk_device.h"
#include "../dxvk/dxvk_context.h"

#include <imgui.h>
#include <windows.h>
#include <mutex>

namespace dxvk {

class MtuOverlay {
public:
  MtuOverlay(const Rc<DxvkDevice>& device, HWND window);
  ~MtuOverlay();

  void update(); // CPU side UI build
  void render(DxvkContext* ctx, const Rc<DxvkImageView>& target);

  bool processMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
  void init(DxvkContext* ctx, VkFormat format);
  void renderUI();

  Rc<DxvkDevice> m_device;
  HWND           m_window;

  ImGuiContext*  m_imgui = nullptr;

  VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

  bool m_visible        = false;
  bool m_initialized    = false;
  bool m_fontsUploaded  = false;

  std::mutex m_mutex;
};

}