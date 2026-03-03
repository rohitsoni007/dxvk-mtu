#pragma once

#include "../dxvk/dxvk_device.h"
#include "../dxvk/dxvk_context.h"
#include "../util/rc/util_rc.h"

#include <imgui.h>
#include <windows.h>
#include <mutex>

namespace dxvk {

class MtuOverlay : public RcObject {

public:
  MtuOverlay(const Rc<DxvkDevice>& device, HWND window);
  ~MtuOverlay();

  void update();
  void render(const DxvkContextObjects& ctx);

  bool processMessage(HWND hWnd, UINT msg,
                      WPARAM wParam, LPARAM lParam);

private:
  void init(const DxvkContextObjects& ctx);
  void renderUI();

  Rc<DxvkDevice> m_device;
  HWND m_window;

  ImGuiContext* m_imgui = nullptr;
  VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

  bool m_visible     = false;
  bool m_initialized = false;

  std::mutex m_mutex;
};

}