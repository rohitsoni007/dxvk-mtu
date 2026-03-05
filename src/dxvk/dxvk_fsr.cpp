#include "dxvk_fsr.h"

#include "dxvk_context.h"
#include "dxvk_device.h"
#include "dxvk_cmdlist.h"
#include "dxvk_image.h"
#include "dxvk_descriptor.h"

#include <dxvk_fsr1_easu.h>
#include <dxvk_fsr1_rcas.h>

#define A_CPU 1
#include "shaders/ffx_a.h"
#include "shaders/ffx_fsr1.h"

namespace dxvk {

struct EasuPush {
  uint32_t con0[4];
  uint32_t con1[4];
  uint32_t con2[4];
  uint32_t con3[4];
};

struct RcasPush {
  uint32_t con0[4];
};


DxvkFsr::DxvkFsr(DxvkDevice* device)
: m_device(device) {

  m_sampler = createSampler();
  m_layout = createDescriptorLayout();

  m_easuLayout = createPipelineLayout();
  m_rcasLayout = createPipelineLayout();

  m_easuShader = createShader(dxvk_fsr1_easu, sizeof(dxvk_fsr1_easu));
  m_rcasShader = createShader(dxvk_fsr1_rcas, sizeof(dxvk_fsr1_rcas));

  m_easuPipe = createPipeline(m_easuLayout, m_easuShader);
  m_rcasPipe = createPipeline(m_rcasLayout, m_rcasShader);
}


DxvkFsr::~DxvkFsr() {

  auto vkd = m_device->vkd();

  vkd->vkDestroyPipeline(vkd->device(), m_easuPipe, nullptr);
  vkd->vkDestroyPipeline(vkd->device(), m_rcasPipe, nullptr);

  vkd->vkDestroyShaderModule(vkd->device(), m_easuShader, nullptr);
  vkd->vkDestroyShaderModule(vkd->device(), m_rcasShader, nullptr);

  vkd->vkDestroyPipelineLayout(vkd->device(), m_easuLayout, nullptr);
  vkd->vkDestroyPipelineLayout(vkd->device(), m_rcasLayout, nullptr);

  vkd->vkDestroyDescriptorSetLayout(vkd->device(), m_layout, nullptr);
  vkd->vkDestroySampler(vkd->device(), m_sampler, nullptr);
}


VkShaderModule DxvkFsr::createShader(const uint32_t* code, size_t size) {

  auto vkd = m_device->vkd();

  VkShaderModuleCreateInfo info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
  info.codeSize = size;
  info.pCode    = code;

  VkShaderModule shader;

  if (vkd->vkCreateShaderModule(
        vkd->device(),
        &info,
        nullptr,
        &shader) != VK_SUCCESS)
    throw DxvkError("FSR: shader creation failed");

  return shader;
}


VkSampler DxvkFsr::createSampler() {

  auto vkd = m_device->vkd();

  VkSamplerCreateInfo info = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

  info.magFilter = VK_FILTER_LINEAR;
  info.minFilter = VK_FILTER_LINEAR;

  info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

  VkSampler sampler;

  if (vkd->vkCreateSampler(vkd->device(), &info, nullptr, &sampler) != VK_SUCCESS)
    throw DxvkError("FSR: sampler creation failed");

  return sampler;
}


VkDescriptorSetLayout DxvkFsr::createDescriptorLayout() {

  auto vkd = m_device->vkd();

  std::array<VkDescriptorSetLayoutBinding,2> bindings = {{

    {0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},

    {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}

  }};

  VkDescriptorSetLayoutCreateInfo info = {
    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };

  info.bindingCount = bindings.size();
  info.pBindings = bindings.data();

  VkDescriptorSetLayout layout;

  vkd->vkCreateDescriptorSetLayout(vkd->device(), &info, nullptr, &layout);

  return layout;
}


VkPipelineLayout DxvkFsr::createPipelineLayout() {

  auto vkd = m_device->vkd();

  VkPushConstantRange push;

  push.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  push.offset = 0;
  push.size = sizeof(EasuPush);

  VkPipelineLayoutCreateInfo info =
    { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

  info.setLayoutCount = 1;
  info.pSetLayouts = &m_layout;

  info.pushConstantRangeCount = 1;
  info.pPushConstantRanges = &push;

  VkPipelineLayout layout;

  vkd->vkCreatePipelineLayout(vkd->device(), &info, nullptr, &layout);

  return layout;
}


VkPipeline DxvkFsr::createPipeline(
  VkPipelineLayout layout,
  VkShaderModule shader) {

  auto vkd = m_device->vkd();

  VkComputePipelineCreateInfo info =
    { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };

  info.stage.sType =
    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

  info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  info.stage.module = shader;
  info.stage.pName = "main";

  info.layout = layout;

  VkPipeline pipe;

  vkd->vkCreateComputePipelines(
    vkd->device(),
    VK_NULL_HANDLE,
    1,
    &info,
    nullptr,
    &pipe);

  return pipe;
}


void DxvkFsr::dispatch(
  const DxvkContextObjects& ctx,
  const Rc<DxvkImageView>& input,
  const Rc<DxvkImageView>& output,
  uint32_t srcW,
  uint32_t srcH,
  uint32_t dstW,
  uint32_t dstH,
  float sharpness) {

  Logger::info("FSR: DxvkFsr::dispatch");

  EasuPush push;

  FsrEasuCon(
    push.con0,
    push.con1,
    push.con2,
    push.con3,
    srcW, srcH,
    srcW, srcH,
    dstW, dstH);

  ctx.cmd->cmdBindPipeline(
    DxvkCmdBuffer::ExecBuffer,
    VK_PIPELINE_BIND_POINT_COMPUTE,
    m_easuPipe);

  ctx.cmd->cmdPushConstants(
    DxvkCmdBuffer::ExecBuffer,
    m_easuLayout,
    VK_SHADER_STAGE_COMPUTE_BIT,
    0,
    sizeof(push),
    &push);

  uint32_t gx = (dstW + 15) / 16;
  uint32_t gy = (dstH + 15) / 16;

  ctx.cmd->cmdDispatch(
    DxvkCmdBuffer::ExecBuffer,
    gx, gy, 1);


  // ---- RCAS SHARPENING ----

  if (sharpness > 0.0f) {

    RcasPush rpush;

    float stops = 4.0f * (1.0f - std::min(1.0f, sharpness));

    FsrRcasCon(rpush.con0, stops);

    ctx.cmd->cmdBindPipeline(
      DxvkCmdBuffer::ExecBuffer,
      VK_PIPELINE_BIND_POINT_COMPUTE,
      m_rcasPipe);

    ctx.cmd->cmdPushConstants(
      DxvkCmdBuffer::ExecBuffer,
      m_rcasLayout,
      VK_SHADER_STAGE_COMPUTE_BIT,
      0,
      sizeof(rpush),
      &rpush);

    ctx.cmd->cmdDispatch(
      DxvkCmdBuffer::ExecBuffer,
      gx, gy, 1);
  }
}

}