#pragma once

#include "../dxvk_include.h"

namespace dxvk {

  /**
   * \brief DXVK ImGui Overlay Input Handler
   * 
   * Intercepts Win32 messages to feed ImGui.
   */
  class OverlayInput : public RcObject {
  public:
    OverlayInput();
    ~OverlayInput();

    /**
     * \brief Initialize input for a window
     * \param [in] window The window handle
     */
    void init(HWND window);

    /**
     * \brief Process Win32 message
     * \returns true if message was handled by ImGui
     */
    bool processMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  private:
    HWND m_window = nullptr;
  };

}
