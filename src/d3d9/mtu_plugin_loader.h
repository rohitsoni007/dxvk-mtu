#pragma once

/**
 * MTU Plugin Loader — present-hook callback type and stub.
 *
 * For now this is a no-op stub. Task 2.4 will expand this to
 * dynamically load mtu_upscaler.dll and resolve the real function.
 */

#include <vulkan/vulkan.h>
#include <windows.h>
#include "../util/util_win32_compat.h"
#include "../util/log/log.h"

namespace dxvk {

  struct MtuRenderParams {
    float cameraFovAngleVertical;
    float cameraNear;
    float cameraFar;
    float jitterOffset[2];
    float frameTimeDelta;
    bool  resetVfx;
  };

  struct MtuConfig {
    bool enabled;
    bool enableSharpening;
    float sharpness;
    float resolutionScale;
    int  qualityPreset;
  };

  /**
   * Callback signature for the upscaler plugin's Process function.
   *
   * \param cmdBuffer Command buffer for recording operations
   * \param device    Vulkan device
   * \param srcImage  Game backbuffer (input to upscaler)
   * \param dstImage  WSI swapchain image (output)
   * \param depthImage Game depth-stencil buffer (can be VK_NULL_HANDLE)
   * \param srcExtent Render resolution (backbuffer size)
   * \param dstExtent Display resolution (swapchain extent)
   * \param params    FSR2 rendering parameters
   */
  using MtuProcessFn = void(*)(
    VkCommandBuffer     cmdBuffer,
    VkDevice            device,
    VkImage             srcImage,
    VkImage             dstImage,
    VkImage             depthImage,
    VkExtent2D          srcExtent,
    VkExtent2D          dstExtent,
    const MtuRenderParams* params);

  using MtuGetConfigFn = void(*)(MtuConfig* outConfig);
  using MtuSetConfigFn = void(*)(const MtuConfig* inConfig);
  using MtuSaveConfigFn = void(*)();

  /**
   * Stub implementation — does nothing.
   * Will be replaced by a real DLL-loaded function in task 2.4.
   */
  inline void mtuProcessStub(
      VkCommandBuffer, VkDevice,
      VkImage, VkImage, VkImage,
      VkExtent2D, VkExtent2D,
      const MtuRenderParams*) {
    // No-op: plugin not loaded yet
  }

  /**
   * Current active process function pointer.
   * Points to the stub by default; will be redirected to the
   * real plugin function once mtu_upscaler.dll is loaded.
   */
  inline MtuProcessFn g_mtuProcess = mtuProcessStub;
  inline MtuGetConfigFn g_mtuGetConfig = nullptr;
  inline MtuSetConfigFn g_mtuSetConfig = nullptr;
  inline MtuSaveConfigFn g_mtuSaveConfig = nullptr;

  /**
   * Dynamically loads the MTU upscaler plugin DLL and resolves imports.
   * Called once by the swapchain when MTU is enabled.
   */
  inline bool loadMtuPlugin() {
    static bool s_loaded = false;
    static bool s_attempted = false;

    if (s_loaded) return true;
    if (s_attempted) return false;
    s_attempted = true;

    HMODULE hModule = ::LoadLibraryA("mtu_upscaler.dll");
    if (!hModule) {
      Logger::err("MTU: Failed to load mtu_upscaler.dll");
      return false;
    }

    // Resolve init (optional but recommended)
    auto initFn = reinterpret_cast<int(*)()>(::GetProcAddress(hModule, "mtuPluginInit"));
    if (initFn) {
      if (initFn() != 0) {
        Logger::err("MTU: mtuPluginInit failed");
        return false;
      }
    } else {
      Logger::warn("MTU: mtuPluginInit not found in plugin");
    }

    // Resolve main process function
    auto processFn = reinterpret_cast<MtuProcessFn>(::GetProcAddress(hModule, "mtuProcess"));
    if (!processFn) {
      Logger::err("MTU: mtuProcess not found in plugin");
      return false;
    }

    g_mtuProcess = processFn;

    g_mtuGetConfig = reinterpret_cast<MtuGetConfigFn>(::GetProcAddress(hModule, "mtuGetConfig"));
    g_mtuSetConfig = reinterpret_cast<MtuSetConfigFn>(::GetProcAddress(hModule, "mtuSetConfig"));
    g_mtuSaveConfig = reinterpret_cast<MtuSaveConfigFn>(::GetProcAddress(hModule, "mtuSaveConfig"));

    s_loaded = true;
    Logger::info("MTU: Successfully loaded mtu_upscaler.dll and resolved hooks.");
    return true;
  }

}
