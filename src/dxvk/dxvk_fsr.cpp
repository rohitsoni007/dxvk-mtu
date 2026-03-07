#include "dxvk_fsr.h"
#include <dxvk_fsr1_easu.h>
#include <dxvk_fsr1_rcas.h>
#include "../util/log/log.h"
#include "../util/util_string.h"

#define A_CPU 1
#include "shaders/ffx_a.h"
#include "shaders/ffx_fsr1.h"

namespace dxvk {

  struct EasuPushConstants {
    uint32_t con0[4];
    uint32_t con1[4];
    uint32_t con2[4];
    uint32_t con3[4];
  };

  struct RcasPushConstants {
    uint32_t con0[4];
  };

  DxvkFsr::DxvkFsr(DxvkDevice* device)
  : m_device(device) {
    auto vkd = m_device->vkd();

    m_sampler = createSampler();
    m_descriptorSetLayout = createDescriptorSetLayout();

    m_easuPipelineLayout = createPipelineLayout(sizeof(EasuPushConstants));
    m_easuShaderModule = createShaderModule(dxvk_fsr1_easu);
    m_easuPipeline = createPipeline(m_easuPipelineLayout, m_easuShaderModule);

    m_rcasPipelineLayout = createPipelineLayout(sizeof(RcasPushConstants));
    m_rcasShaderModule = createShaderModule(dxvk_fsr1_rcas);
    m_rcasPipeline = createPipeline(m_rcasPipelineLayout, m_rcasShaderModule);
  }

  DxvkFsr::~DxvkFsr() {
    auto vkd = m_device->vkd();

    vkd->vkDestroyPipeline(vkd->device(), m_easuPipeline, nullptr);
    vkd->vkDestroyShaderModule(vkd->device(), m_easuShaderModule, nullptr);
    vkd->vkDestroyPipelineLayout(vkd->device(), m_easuPipelineLayout, nullptr);

    vkd->vkDestroyPipeline(vkd->device(), m_rcasPipeline, nullptr);
    vkd->vkDestroyShaderModule(vkd->device(), m_rcasShaderModule, nullptr);
    vkd->vkDestroyPipelineLayout(vkd->device(), m_rcasPipelineLayout, nullptr);

    vkd->vkDestroyDescriptorSetLayout(vkd->device(), m_descriptorSetLayout, nullptr);
    vkd->vkDestroySampler(vkd->device(), m_sampler, nullptr);
  }

  VkShaderModule DxvkFsr::createShaderModule(const SpirvCodeBuffer& code) const {
    auto vkd = m_device->vkd();
    VkShaderModuleCreateInfo info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    info.codeSize = code.size();
    info.pCode = code.data();

    VkShaderModule result = VK_NULL_HANDLE;
    if (vkd->vkCreateShaderModule(vkd->device(), &info, nullptr, &result) != VK_SUCCESS)
      throw DxvkError("DxvkFsr: Failed to create shader module");
    return result;
  }

  VkSampler DxvkFsr::createSampler() const {
    auto vkd = m_device->vkd();
    VkSamplerCreateInfo info = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.maxAnisotropy = 1.0f;

    VkSampler result = VK_NULL_HANDLE;
    if (vkd->vkCreateSampler(vkd->device(), &info, nullptr, &result) != VK_SUCCESS)
      throw DxvkError("DxvkFsr: Failed to create sampler");
    return result;
  }

  VkDescriptorSetLayout DxvkFsr::createDescriptorSetLayout() const {
    auto vkd = m_device->vkd();

    // std::array<VkDescriptorSetLayoutBinding, 2> bindings = {{
    //   { 0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
    //   { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr }
    // }};
    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {{
      { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
      { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr }
    }};

    VkDescriptorSetLayoutCreateInfo info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    info.bindingCount = bindings.size();
    info.pBindings = bindings.data();

    VkDescriptorSetLayout result = VK_NULL_HANDLE;
    if (vkd->vkCreateDescriptorSetLayout(vkd->device(), &info, nullptr, &result) != VK_SUCCESS)
      throw DxvkError("DxvkFsr: Failed to create descriptor set layout");
    return result;
  }

  VkPipelineLayout DxvkFsr::createPipelineLayout(size_t pushSize) const {
    auto vkd = m_device->vkd();

    VkPushConstantRange push = { VK_SHADER_STAGE_COMPUTE_BIT, 0, pushSize }; // Max size for both

    VkPipelineLayoutCreateInfo info = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    info.setLayoutCount = 1;
    info.pSetLayouts = &m_descriptorSetLayout;
    info.pushConstantRangeCount = 1;
    info.pPushConstantRanges = &push;

    VkPipelineLayout result = VK_NULL_HANDLE;
    if (vkd->vkCreatePipelineLayout(vkd->device(), &info, nullptr, &result) != VK_SUCCESS)
      throw DxvkError("DxvkFsr: Failed to create pipeline layout");
    return result;
  }

  VkPipeline DxvkFsr::createPipeline(VkPipelineLayout layout, VkShaderModule shader) const {
    auto vkd = m_device->vkd();

    VkComputePipelineCreateInfo info = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
    info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    info.stage.module = shader;
    info.stage.pName = "main";
    info.layout = layout;
    info.basePipelineIndex = -1;

    VkPipeline pipeline = VK_NULL_HANDLE;
    if (vkd->vkCreateComputePipelines(vkd->device(), VK_NULL_HANDLE, 1, &info, nullptr, &pipeline) != VK_SUCCESS)
      throw DxvkError("DxvkFsr: Failed to create compute pipeline");
    return pipeline;
  }

  void DxvkFsr::dispatchEasu(
    const DxvkContextObjects& ctx,
    const Rc<DxvkImageView>&  inputView,
          VkRect2D            srcRect,
    const Rc<DxvkImageView>&  outputView,
          VkRect2D            dstRect) {
    Logger::info(str::format("DxvkFsr: dispatchEasu: ",
      "srcRect=", srcRect.offset.x, ",", srcRect.offset.y, " ", srcRect.extent.width, "x", srcRect.extent.height, " -> ",
      "dstRect=", dstRect.offset.x, ",", dstRect.offset.y, " ", dstRect.extent.width, "x", dstRect.extent.height));

    EasuPushConstants push = {};
    FsrEasuConOffset(
      push.con0, push.con1, push.con2, push.con3,
      static_cast<float>(dstRect.extent.width), static_cast<float>(dstRect.extent.height),
      static_cast<float>(inputView->image()->info().extent.width), static_cast<float>(inputView->image()->info().extent.height),
      static_cast<float>(srcRect.extent.width), static_cast<float>(srcRect.extent.height),
      static_cast<float>(srcRect.offset.x), static_cast<float>(srcRect.offset.y)
    );

    VkDescriptorSet dset = ctx.descriptorPool->alloc(m_descriptorSetLayout);

    // VkDescriptorImageInfo srcInfo = { VK_NULL_HANDLE, inputView->handle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    VkDescriptorImageInfo srcInfo = { m_sampler, inputView->handle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    VkDescriptorImageInfo dstInfo = { VK_NULL_HANDLE, outputView->handle(), VK_IMAGE_LAYOUT_GENERAL };

    std::array<VkWriteDescriptorSet, 2> writes = {{
      { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, dset, 0, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &srcInfo, nullptr, nullptr },
      { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, dset, 1, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &dstInfo, nullptr, nullptr }
    }};
    
    ctx.cmd->updateDescriptorSets(writes.size(), writes.data());

    ctx.cmd->cmdBindPipeline(DxvkCmdBuffer::ExecBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_easuPipeline);
    ctx.cmd->cmdBindDescriptorSet(DxvkCmdBuffer::ExecBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_easuPipelineLayout, dset, 0, nullptr);
    ctx.cmd->cmdPushConstants(DxvkCmdBuffer::ExecBuffer, m_easuPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);

    uint32_t workgroupsX = (dstRect.extent.width + 15) / 16;
    uint32_t workgroupsY = (dstRect.extent.height + 15) / 16;
    ctx.cmd->cmdDispatch(DxvkCmdBuffer::ExecBuffer, workgroupsX, workgroupsY, 1);
  }

  void DxvkFsr::dispatchRcas(
    const DxvkContextObjects& ctx,
    const Rc<DxvkImageView>&  inputView,
          VkRect2D            srcRect,
    const Rc<DxvkImageView>&  outputView,
          VkRect2D            dstRect,
          float               sharpness) {
    Logger::info(str::format("DxvkFsr: dispatchRcas: ",
      "sharpness=", sharpness, ", ",
      "srcRect=", srcRect.offset.x, ",", srcRect.offset.y, " ", srcRect.extent.width, "x", srcRect.extent.height, " -> ",
      "dstRect=", dstRect.offset.x, ",", dstRect.offset.y, " ", dstRect.extent.width, "x", dstRect.extent.height));

    RcasPushConstants push = {};
    // FSR RCAS sharpness is a "stops" value where 0 is max and higher is less.
    float stops = 4.0f * (1.0f - std::min(1.0f, sharpness));
    FsrRcasCon(push.con0, stops);

    VkDescriptorSet dset = ctx.descriptorPool->alloc(m_descriptorSetLayout);

    // VkDescriptorImageInfo srcInfo = { VK_NULL_HANDLE, inputView->handle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    VkDescriptorImageInfo srcInfo = { m_sampler, inputView->handle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    VkDescriptorImageInfo dstInfo = { VK_NULL_HANDLE, outputView->handle(), VK_IMAGE_LAYOUT_GENERAL };

    std::array<VkWriteDescriptorSet, 2> writes = {{
      { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, dset, 0, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &srcInfo, nullptr, nullptr },
      { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, dset, 1, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &dstInfo, nullptr, nullptr }
    }};
    
    ctx.cmd->updateDescriptorSets(writes.size(), writes.data());

    ctx.cmd->cmdBindPipeline(DxvkCmdBuffer::ExecBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_rcasPipeline);
    ctx.cmd->cmdBindDescriptorSet(DxvkCmdBuffer::ExecBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_rcasPipelineLayout, dset, 0, nullptr);
    ctx.cmd->cmdPushConstants(DxvkCmdBuffer::ExecBuffer, m_rcasPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);

    uint32_t workgroupsX = (dstRect.extent.width + 15) / 16;
    uint32_t workgroupsY = (dstRect.extent.height + 15) / 16;
    ctx.cmd->cmdDispatch(DxvkCmdBuffer::ExecBuffer, workgroupsX, workgroupsY, 1);
  }

  void DxvkFsr::dispatch(
    const DxvkContextObjects& ctx,
    const Rc<DxvkImageView>&  inputView,
          VkRect2D            srcRect,
    const Rc<DxvkImageView>&  outputView,
          VkRect2D            dstRect,
          float               sharpness) {
    Logger::info(str::format("DxvkFsr: dispatch: ",
      "sharpness=", sharpness, ", ",
      "srcRect=", srcRect.offset.x, ",", srcRect.offset.y, " ", srcRect.extent.width, "x", srcRect.extent.height, " -> ",
      "dstRect=", dstRect.offset.x, ",", dstRect.offset.y, " ", dstRect.extent.width, "x", dstRect.extent.height, ", ",
      "inputView=", (inputView ? "yes" : "no"), ", ",
      "outputView=", (outputView ? "yes" : "no")));

    // Transition input to shader read and output to general
    VkImageMemoryBarrier2 barrierRead = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
    barrierRead.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    barrierRead.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    barrierRead.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    barrierRead.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    // barrierRead.oldLayout = inputView->image()->pickLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    // barrierRead.newLayout = inputView->image()->pickLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    barrierRead.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrierRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrierRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrierRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrierRead.image = inputView->image()->handle();
    barrierRead.subresourceRange = inputView->imageSubresources();

    VkImageMemoryBarrier2 barrierWrite = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
    barrierWrite.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    barrierWrite.srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT;
    barrierWrite.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    barrierWrite.dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    barrierWrite.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrierWrite.newLayout = outputView->image()->pickLayout(VK_IMAGE_LAYOUT_GENERAL);
    barrierWrite.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrierWrite.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrierWrite.image = outputView->image()->handle();
    barrierWrite.subresourceRange = outputView->imageSubresources();

    std::array<VkImageMemoryBarrier2, 2> barriers = { barrierRead, barrierWrite };

    VkDependencyInfo depInfo = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    depInfo.imageMemoryBarrierCount = barriers.size();
    depInfo.pImageMemoryBarriers = barriers.data();

    ctx.cmd->cmdPipelineBarrier(DxvkCmdBuffer::ExecBuffer, &depInfo);

    if (sharpness > 0.0f) {
      // Create/resize RCAS intermediate image if needed
      if (m_rcasImage == nullptr || m_rcasImage->info().extent.width != dstRect.extent.width || m_rcasImage->info().extent.height != dstRect.extent.height) {
        if (m_rcasImage) {
          Logger::info(str::format("DxvkFsr: Resizing RCAS intermediate image: ",
            m_rcasImage->info().extent.width, "x", m_rcasImage->info().extent.height, " -> ",
            dstRect.extent.width, "x", dstRect.extent.height));
        } else {
          Logger::info(str::format("DxvkFsr: Creating RCAS intermediate image: ",
            dstRect.extent.width, "x", dstRect.extent.height));
        }
        DxvkImageCreateInfo imgInfo = { };
        imgInfo.type = VK_IMAGE_TYPE_2D;
        imgInfo.format = outputView->image()->info().format;
        imgInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
        imgInfo.extent = { dstRect.extent.width, dstRect.extent.height, 1u };
        imgInfo.numLayers = 1;
        imgInfo.mipLevels = 1;
        imgInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imgInfo.stages = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        imgInfo.access = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imgInfo.layout = VK_IMAGE_LAYOUT_GENERAL;
        imgInfo.debugName = "FSR1 RCAS Intermediate";

        m_rcasImage = m_device->createImage(imgInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        Logger::info("DxvkFsr: RCAS intermediate image created.");

        DxvkImageViewKey viewInfo = { };
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        viewInfo.format = imgInfo.format;
        viewInfo.aspects = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.mipIndex = 0;
        viewInfo.mipCount = 1;
        viewInfo.layerIndex = 0;
        viewInfo.layerCount = 1;

        m_rcasView = m_rcasImage->createView(viewInfo);
        Logger::info("DxvkFsr: RCAS intermediate view created.");
      }

      // Transition RCAS image to general
      VkImageMemoryBarrier2 rcasBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
      rcasBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
      rcasBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT;
      rcasBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
      rcasBarrier.dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
      rcasBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      rcasBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
      rcasBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      rcasBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      rcasBarrier.image = m_rcasImage->handle();
      rcasBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

      VkDependencyInfo rcasDepInfo = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
      rcasDepInfo.imageMemoryBarrierCount = 1;
      rcasDepInfo.pImageMemoryBarriers = &rcasBarrier;

      ctx.cmd->cmdPipelineBarrier(DxvkCmdBuffer::ExecBuffer, &rcasDepInfo);

      // EASU -> RCAS Intermediate
      Logger::info("DxvkFsr: Dispatching EASU...");
      dispatchEasu(ctx, inputView, srcRect, m_rcasView, { {0, 0}, dstRect.extent });
      Logger::info("DxvkFsr: EASU finished.");

      // Barrier between EASU and RCAS
      rcasBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
      rcasBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
      rcasBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
      rcasBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      
      ctx.cmd->cmdPipelineBarrier(DxvkCmdBuffer::ExecBuffer, &rcasDepInfo);

      // RCAS Intermediate -> Final output
      Logger::info("DxvkFsr: Dispatching RCAS...");
      dispatchRcas(ctx, m_rcasView, { {0, 0}, dstRect.extent }, outputView, dstRect, sharpness);
      Logger::info("DxvkFsr: RCAS finished.");

      ctx.cmd->track(m_rcasImage, DxvkAccess::Write);
    } else {
      // EASU only
      Logger::info("DxvkFsr: Dispatching EASU (no RCAS)...");
      dispatchEasu(ctx, inputView, srcRect, outputView, dstRect);
      Logger::info("DxvkFsr: EASU finished.");
    }
    Logger::info("DxvkFsr: dispatch finished.");
  }

}
