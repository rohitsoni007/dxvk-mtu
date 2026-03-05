#pragma once

#include "../util/rc/util_rc.h"
#include "dxvk_image.h"
#include "dxvk_shader.h"

namespace dxvk {

  class DxvkDevice;
  class DxvkImageView;
  struct DxvkContextObjects;

  class DxvkFsr {

  public:

    DxvkFsr(DxvkDevice* device);
    ~DxvkFsr();

    void dispatch(
      const DxvkContextObjects& ctx,
      const Rc<DxvkImageView>& input,
      const Rc<DxvkImageView>& output,
      uint32_t srcWidth,
      uint32_t srcHeight,
      uint32_t dstWidth,
      uint32_t dstHeight,
      float sharpness);

  private:

    DxvkDevice* m_device;

    VkSampler m_sampler;

    VkDescriptorSetLayout m_layout;

    VkPipelineLayout m_easuLayout;
    VkPipelineLayout m_rcasLayout;

    VkShaderModule m_easuShader;
    VkShaderModule m_rcasShader;

    VkPipeline m_easuPipe;
    VkPipeline m_rcasPipe;

    VkShaderModule createShader(const uint32_t* code, size_t size);

    VkSampler createSampler();

    VkDescriptorSetLayout createDescriptorLayout();

    VkPipelineLayout createPipelineLayout();

    VkPipeline createPipeline(
      VkPipelineLayout layout,
      VkShaderModule shader);

  };

}