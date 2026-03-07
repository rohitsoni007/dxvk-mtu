#pragma once

#include "dxvk_context.h"
#include "dxvk_device.h"
#include "dxvk_image.h"

namespace dxvk {

  class DxvkFsr {
  public:

    DxvkFsr(DxvkDevice* device);
    ~DxvkFsr();

    /**
     * \brief Dispatches the FSR compute shaders
     *
     * Upscales inputView to outputView using EASU, and optionally RCAS.
     */
    void dispatch(
      const DxvkContextObjects& ctx,
      const Rc<DxvkImageView>&  inputView,
            VkRect2D            srcRect,
      const Rc<DxvkImageView>&  outputView,
            VkRect2D            dstRect,
            float               sharpness);

  private:
    DxvkDevice*           m_device = nullptr;
    VkSampler             m_sampler = VK_NULL_HANDLE;

    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;

    VkPipelineLayout      m_easuPipelineLayout = VK_NULL_HANDLE;
    VkShaderModule        m_easuShaderModule = VK_NULL_HANDLE;
    VkPipeline            m_easuPipeline = VK_NULL_HANDLE;

    VkPipelineLayout      m_rcasPipelineLayout = VK_NULL_HANDLE;
    VkShaderModule        m_rcasShaderModule = VK_NULL_HANDLE;
    VkPipeline            m_rcasPipeline = VK_NULL_HANDLE;
    
    Rc<DxvkImage>         m_rcasImage = nullptr;
    Rc<DxvkImageView>     m_rcasView = nullptr;

    VkShaderModule createShaderModule(const SpirvCodeBuffer& code) const;
    VkDescriptorSetLayout createDescriptorSetLayout() const;
    VkPipelineLayout createPipelineLayout(size_t pushSize) const;
    VkPipeline createPipeline(VkPipelineLayout layout, VkShaderModule shader) const;
    VkSampler createSampler() const;

    void dispatchEasu(
      const DxvkContextObjects& ctx,
      const Rc<DxvkImageView>&  inputView,
            VkRect2D            srcRect,
      const Rc<DxvkImageView>&  outputView,
            VkRect2D            dstRect);

    void dispatchRcas(
      const DxvkContextObjects& ctx,
      const Rc<DxvkImageView>&  inputView,
            VkRect2D            srcRect,
      const Rc<DxvkImageView>&  outputView,
            VkRect2D            dstRect,
            float               sharpness);
  };

}
