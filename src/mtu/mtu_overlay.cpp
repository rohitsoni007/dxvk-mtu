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
    ImGui::SetNextWindowSize(ImVec2(450, 750), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("MTU Settings — Super Resolution", &m_visible, ImGuiWindowFlags_NoCollapse)) {
        if (g_mtuGetConfig == nullptr) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            ImGui::TextWrapped("WARNING: mtu_upscaler.dll not found. Scaling restricted.");
            ImGui::PopStyleColor();
            ImGui::Separator();
        }

        bool changed = false;

        // --- Upscaling Section ---
        if (ImGui::CollapsingHeader("Upscaling", ImGuiTreeNodeFlags_DefaultOpen)) {
            const char* upscalingMethods[] = { "FSR 2.2", "FSR 2.1", "DLSS (Stub)", "Off" };
            int currentMethod = 0; // Fixed for now as we are FSR 2.2 focused
            if (ImGui::Combo("Upscaling", &currentMethod, upscalingMethods, IM_ARRAYSIZE(upscalingMethods))) {
                m_config.enabled = (currentMethod != 3);
                changed = true;
            }

            const char* scaleModes[] = { "Quality", "Balanced", "Performance", "Ultra Performance" };
            if (ImGui::Combo("Scale mode", &m_config.qualityPreset, scaleModes, IM_ARRAYSIZE(scaleModes))) {
                changed = true;
            }

            if (ImGui::SliderFloat("Mip LOD bias", &m_config.mipBiasOffset, -5.0f, 5.0f, "%.3f")) {
                changed = true;
                if (m_onMipBiasChange)
                    m_onMipBiasChange(m_config.mipBiasOffset);
            }

            if (ImGui::Checkbox("Dynamic resolution", &m_config.dynamicResolution)) {
                changed = true;
            }

            const char* maskModes[] = { "Manual Reactive Mask Generation", "Auto Mask", "Off" };
            if (ImGui::Combo("Reactive Mask", &m_config.reactiveMaskMode, maskModes, IM_ARRAYSIZE(maskModes))) {
                changed = true;
            }

            if (ImGui::Checkbox("Use Transparency and Composition Mask", &m_config.useTransparencyMask)) {
                changed = true;
            }
        }

        // --- Dev Options Section ---
        if (ImGui::CollapsingHeader("Dev Options", ImGuiTreeNodeFlags_DefaultOpen)) {
            static bool apiDebug = false;
            ImGui::Checkbox("Enable API Debug Checking", &apiDebug);

            if (ImGui::Button("Reset accumulation")) {
                // Future: trigger FSR2 Reset
            }

            if (ImGui::Checkbox("RCAS Sharpening", &m_config.enableSharpening)) {
                changed = true;
            }
            
            if (m_config.enableSharpening) {
                if (ImGui::SliderFloat("Sharpness", &m_config.sharpness, 0.0f, 1.0f)) {
                    changed = true;
                }
            }

            ImGui::Separator();
            
            // Resolution Info (Placeholder or fetched from swapchain)
            ImGui::TextDisabled("Render resolution: %dx%d", 
                (int)(m_device->adapter()->handle() ? 1280 : 0), // Placeholder
                (int)720);
            ImGui::TextDisabled("Display resolution: %dx%d", 1920, 1080);
        }

        // --- Post-Processing Section ---
        if (ImGui::CollapsingHeader("PostProcessing", 0)) {
            const char* tonemappers[] = { "AMD Tonemapper", "Reinhard", "None" };
            static int currentTonemapper = 0;
            ImGui::Combo("Tonemapper", &currentTonemapper, tonemappers, IM_ARRAYSIZE(tonemappers));

            if (ImGui::SliderFloat("Exposure", &m_config.exposureScale, 0.1f, 10.0f, "%.3f")) {
                changed = true;
            }
            
            ImGui::Checkbox("Auto Exposure", &m_config.autoExposure);
        }

        // --- Magnifier Section ---
        if (ImGui::CollapsingHeader("Magnifier", 0)) {
            static bool showMagnifier = false;
            static bool lockMagnifier = false;
            ImGui::Checkbox("Show Magnifier (M)", &showMagnifier);
            ImGui::Checkbox("Lock Position (L)", &lockMagnifier);
        }

        if (changed) {
            syncConfigToPlugin();
        }

        ImGui::Separator();
        if (ImGui::Button("Save Configuration")) {
            if (g_mtuSaveConfig) g_mtuSaveConfig();
        }

        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 25);
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
