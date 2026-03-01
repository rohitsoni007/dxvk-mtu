#include <windows.h>
#include <mutex>
#include "../util/log/log.h"
namespace mtu {

  static std::once_flag g_initFlag;

  void initialize() {
    dxvk::Logger::info("MTU: initialize");
    std::call_once(g_initFlag, []() {
      dxvk::Logger::info("MTU: cagif_core.dll");
      HMODULE core = LoadLibraryA("cagif_core.dll");
      if (!core)
        return;
      dxvk::Logger::info("MTU: after cagif_core.dll");
      auto init = (void(*)())GetProcAddress(core, "CAGIF_Initialize");
      dxvk::Logger::info("MTU: after GetProcAddress");
      if (init)
        init();
      dxvk::Logger::info("MTU: after init");
    });
  }

}