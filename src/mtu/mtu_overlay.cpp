#include "mtu_overlay.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include <functional>
#include "../dxvk/dxvk_device.h"
#include "../dxvk/dxvk_context.h"
#include "../dxvk/hud/dxvk_hud_renderer.h"

#include <windows.h>
#include "../d3d9/mtu_plugin_loader.h"

#include "imgui/imgui.h"

#include "../util/util_win32_compat.h"
#include "../util/log/log.h"

// Forward declare ImGui_ImplWin32_WndProcHandler
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace dxvk {

  MtuOverlay::MtuOverlay(const Rc<DxvkDevice>& device, HWND window)
  : m_device(device), m_window(window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    setupStyle();
    
    ImGui_ImplWin32_Init(m_window);
  }

  MtuOverlay::~MtuOverlay() {
    if (m_initialized) {
      m_device->vkd()->vkDestroyDescriptorPool(m_device->vkd()->device(), m_descriptorPool, nullptr);
      ImGui_ImplVulkan_Shutdown();
    }
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
  }

  void MtuOverlay::update() {
    if (ImGui::IsKeyPressed(ImGuiKey_F12)) {
      toggleVisibility();
      if (m_visible)
        syncConfigFromPlugin();
      Logger::info(str::format("MTU: Overlay visibility toggled (", m_visible ? "on" : "off", ")"));
    }
  }

  void MtuOverlay::render(
    const DxvkContextObjects& ctx,
    const Rc<DxvkImageView>&  dstView) {
    
    if (!m_initialized)
      init(ctx, dstView);

    if (!m_visible)
      return;

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    renderUI();

    ImGui::Render();
    
    // VkRenderPassBeginInfo rpInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    // We'd ideally want to render directly to dstView without a renderpass if possible,
    // but ImGui_ImplVulkan_RenderDrawData needs a command buffer and handles its own stuff.
    // DXVK uses dynamic rendering and custom blitters.
    
    // For now, let's assume we can use the command buffer directly.
    // Note: ImGui_ImplVulkan_RenderDrawData requires a renderpass to be active or dynamic rendering.
    // Since DXVK 2.0+ uses dynamic rendering primarily:
    
    ImDrawData* drawData = ImGui::GetDrawData();
    
    // Prepare dynamic rendering
    VkRenderingAttachmentInfo colorAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
    colorAttachment.imageView = dstView->handle();
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo renderingInfo = { VK_STRUCTURE_TYPE_RENDERING_INFO };
    renderingInfo.renderArea = {{0, 0}, {dstView->image()->info().extent.width, dstView->image()->info().extent.height}};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

    m_device->vkd()->vkCmdBeginRendering(ctx.cmd->getCmdBuffer(DxvkCmdBuffer::ExecBuffer), &renderingInfo);
    ImGui_ImplVulkan_RenderDrawData(drawData, ctx.cmd->getCmdBuffer(DxvkCmdBuffer::ExecBuffer));
    m_device->vkd()->vkCmdEndRendering(ctx.cmd->getCmdBuffer(DxvkCmdBuffer::ExecBuffer));
  }

  bool MtuOverlay::processMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
      return true;
    
    if (m_visible) {
        // Eat input meant for the game if overlay is up
        // (This might be too aggressive, but common for overlays)
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse || io.WantCaptureKeyboard)
            return true;
    }
    
    return false;
  }

  void MtuOverlay::init(const DxvkContextObjects& ctx, const Rc<DxvkImageView>& dstView) {
    auto vkd = m_device->vkd();
    
    // Create descriptor pool for ImGui
    std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
    };
    VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    
    if (vkd->vkCreateDescriptorPool(vkd->device(), &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
      Logger::err("MTU: Failed to create ImGui descriptor pool");
      return;
    }

    ImGui_ImplVulkan_InitInfo initInfo = {};

    // Load Vulkan functions dynamically to avoid linker issues with DXVK's dynamic loader
    ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_3, [](const char* function_name, void* user_data) {
        auto device = static_cast<DxvkDevice*>(user_data);
        return device->adapter()->vki()->sym(function_name);
    }, m_device.ptr());

    initInfo.Instance = m_device->adapter()->vki()->instance();
    initInfo.PhysicalDevice = m_device->adapter()->handle();
    initInfo.Device = vkd->device();
    initInfo.QueueFamily = m_device->queues().graphics.queueFamily;
    initInfo.Queue = m_device->queues().graphics.queueHandle;
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = m_descriptorPool;
    initInfo.MinImageCount = 2; // Arbitrary
    initInfo.ImageCount = 3;    // Arbitrary
    initInfo.Allocator = nullptr;
    initInfo.CheckVkResultFn = [](VkResult err) { 
        if (err != VK_SUCCESS) Logger::err(str::format("MTU: ImGui Vulkan error: ", err)); 
    };
    
    // Enable dynamic rendering
    initInfo.UseDynamicRendering = true;
    initInfo.PipelineInfoMain.Subpass = 0;
    initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    VkFormat format = dstView->image()->info().format;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &format;

    ImGui_ImplVulkan_Init(&initInfo);
    
    // Load fonts
    ctx.cmd->cmdBeginDebugUtilsLabel(DxvkCmdBuffer::ExecBuffer, vk::makeLabel(0xffffff, "ImGui Font Upload"));
    // ImGui_ImplVulkan_CreateFontsTexture() is now internal or handled via UpdateTexture in newer versions.
    // In most cases with Init, it's handled.
    ctx.cmd->cmdEndDebugUtilsLabel(DxvkCmdBuffer::ExecBuffer);
    
    m_initialized = true;
  }

  void MtuOverlay::setupStyle() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Premium Look: Rounded corners, subtle transparency
    style.WindowRounding = 8.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 12.0f;
    
    auto& colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 0.94f);
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
  }

  void MtuOverlay::renderUI() {
    ImGui::SetNextWindowSize(ImVec2(450, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("MTU Settings — FidelityFX Super Resolution 2.2", &m_visible, ImGuiWindowFlags_NoCollapse)) {
        
        bool changed = false;

        if (ImGui::CollapsingHeader("Upscaling Mode", ImGuiTreeNodeFlags_DefaultOpen)) {
            const char* items[] = { "Quality", "Balanced", "Performance", "Ultra Performance" };
            if (ImGui::Combo("Preset", &m_config.qualityPreset, items, IM_ARRAYSIZE(items))) {
                changed = true;
            }
            
            if (ImGui::SliderFloat("Resolution Scale", &m_config.resolutionScale, 0.1f, 1.0f, "%.2f")) {
                changed = true;
            }
        }
        
        if (ImGui::CollapsingHeader("Post-Processing", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Checkbox("Enable Sharpening (RCAS)", &m_config.enableSharpening)) {
                changed = true;
            }
            
            if (ImGui::SliderFloat("Sharpness", &m_config.sharpness, 0.0f, 1.0f)) {
                changed = true;
            }
        }
        
        if (ImGui::CollapsingHeader("Advanced & Experimental", 0)) {
            if (ImGui::Checkbox("Enabled", &m_config.enabled)) {
                changed = true;
            }
            
            if (ImGui::Checkbox("Auto Exposure", &m_config.autoExposure)) {
                changed = true;
            }
            
            if (!m_config.autoExposure) {
                if (ImGui::SliderFloat("Exposure Scale", &m_config.exposureScale, 0.1f, 10.0f, "%.2f")) {
                    changed = true;
                }
            }
            
            if (ImGui::Checkbox("Depth Inverted", &m_config.depthInverted)) {
                changed = true;
            }
            
            if (ImGui::SliderFloat("Jitter Scale", &m_config.jitterScale, 0.0f, 2.0f, "%.2f")) {
                changed = true;
            }
            
            if (ImGui::SliderFloat("Mip Bias Offset", &m_config.mipBiasOffset, -2.0f, 2.0f, "%.2f")) {
                changed = true;
                if (m_onMipBiasChange)
                    m_onMipBiasChange(m_config.mipBiasOffset);
            }

            ImGui::Separator();
            if (ImGui::Button("Save Settings to Disk")) {
                if (g_mtuSaveConfig) g_mtuSaveConfig();
            }
        }

        if (changed) {
            syncConfigToPlugin();
        }
        
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 40);
        ImGui::Separator();
        ImGui::TextDisabled("MTU v1.0 | Press F12 to toggle overlay");
    }
    ImGui::End();
  }

  void MtuOverlay::syncConfigFromPlugin() {
    if (g_mtuGetConfig)
        g_mtuGetConfig(&m_config);
  }

  void MtuOverlay::syncConfigToPlugin() {
    if (g_mtuSetConfig)
        g_mtuSetConfig(&m_config);
  }

}
