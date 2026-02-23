#pragma once

/**
 * MTU Plugin Loader — present-hook callback type and stub.
 *
 * For now this is a no-op stub. Task 2.4 will expand this to
 * dynamically load mtu_upscaler.dll and resolve the real function.
 */

#include "../dxvk/dxvk_context.h"

#include <vulkan/vulkan.h>

namespace dxvk {

  /**
   * Callback signature for the upscaler plugin's Process function.
   *
   * \param srcImage  Game backbuffer (input to upscaler)
   * \param dstImage  WSI swapchain image (output)
   * \param srcExtent Render resolution (backbuffer size)
   * \param dstExtent Display resolution (swapchain extent)
   * \param ctxObjects DXVK context objects for command recording
   */
  using MtuProcessFn = void(*)(
    VkImage             srcImage,
    VkImage             dstImage,
    VkExtent2D          srcExtent,
    VkExtent2D          dstExtent,
    DxvkContextObjects& ctxObjects);

  /**
   * Stub implementation — does nothing.
   * Will be replaced by a real DLL-loaded function in task 2.4.
   */
  inline void mtuProcessStub(
      VkImage, VkImage,
      VkExtent2D, VkExtent2D,
      DxvkContextObjects&) {
    // No-op: plugin not loaded yet
  }

  /**
   * Current active process function pointer.
   * Points to the stub by default; will be redirected to the
   * real plugin function once mtu_upscaler.dll is loaded.
   */
  inline MtuProcessFn g_mtuProcess = mtuProcessStub;

}
