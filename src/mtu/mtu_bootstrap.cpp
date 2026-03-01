#include <windows.h>
#include <mutex>

namespace mtu {

  static std::once_flag g_initFlag;

  void initialize() {
    std::call_once(g_initFlag, []() {
      HMODULE core = LoadLibraryA("cagif_core.dll");
      if (!core)
        return;

      auto init = (void(*)())GetProcAddress(core, "CAGIF_Initialize");
      if (init)
        init();
    });
  }

}