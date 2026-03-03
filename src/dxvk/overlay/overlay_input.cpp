#include "overlay_input.h"
#include <imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace dxvk {

  OverlayInput::OverlayInput() {
  }


  OverlayInput::~OverlayInput() {
  }


  void OverlayInput::init(HWND window) {
    m_window = window;
    ImGui_ImplWin32_Init(window);
  }


  bool OverlayInput::processMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYDOWN && wParam == VK_F11) {
      // Toggle logic would need to be in OverlayManager or passed here
      // For now, let's keep it simple and assume OverlayManager calls this
      return false; // Let it fall through so we can handle it in OverlayManager if we prefer
    }

    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
      return true;

    return false;
  }

}
