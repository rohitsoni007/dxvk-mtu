#include "mtu_overlay.h"

#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_vulkan.h"

#include "../util/log/log.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(
  HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace dxvk {

MtuOverlay::MtuOverlay(
  const Rc<DxvkDevice>& device,
  HWND window)
: m_device(device),
  m_window(window) {

  IMGUI_CHECKVERSION();

  m_imgui = ImGui::CreateContext();
  ImGui::SetCurrentContext(m_imgui);

  ImGui::StyleColorsDark();

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui_ImplWin32_Init(m_window);
}

MtuOverlay::~MtuOverlay() {
  ImGui::SetCurrentContext(m_imgui);

  if (m_initialized) {
    ImGui_ImplVulkan_Shutdown();

    if (m_descriptorPool != VK_NULL_HANDLE) {
      m_device->vkd()->vkDestroyDescriptorPool(
        m_device->vkd()->device(),
        m_descriptorPool,
        nullptr);
    }
  }

  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext(m_imgui);
}

void MtuOverlay::init(const DxvkContextObjects& ctx) {
  auto vkd = m_device->vkd();

  VkDescriptorPoolSize poolSize = {
    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    100
  };

  VkDescriptorPoolCreateInfo poolInfo = {
    VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO
  };

  poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  poolInfo.maxSets       = 100;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes    = &poolSize;

  if (vkd->vkCreateDescriptorPool(
        vkd->device(),
        &poolInfo,
        nullptr,
        &m_descriptorPool) != VK_SUCCESS) {
    Logger::err("MTU Overlay: descriptor pool failed");
    return;
  }

  ImGui_ImplVulkan_LoadFunctions(
    VK_API_VERSION_1_3,
    [](const char* name, void* user) {
      return static_cast<DxvkDevice*>(user)
        ->adapter()->vki()->sym(name);
    },
    m_device.ptr());

  ImGui_ImplVulkan_InitInfo info = {};
  info.Instance       = m_device->adapter()->vki()->instance();
  info.PhysicalDevice = m_device->adapter()->handle();
  info.Device         = vkd->device();
  info.QueueFamily    = m_device->queues().graphics.queueFamily;
  info.Queue          = m_device->queues().graphics.queueHandle;
  info.DescriptorPool = m_descriptorPool;
  info.MinImageCount  = 2;
  info.ImageCount     = 2;
  info.UseDynamicRendering = false;

  ImGui_ImplVulkan_Init(&info);

  m_initialized = true;
}

void MtuOverlay::update() {
  if (!m_visible)
    return;

  std::lock_guard<std::mutex> lock(m_mutex);

  ImGui::SetCurrentContext(m_imgui);

  ImGui_ImplWin32_NewFrame();
  ImGui_ImplVulkan_NewFrame();
  ImGui::NewFrame();

  renderUI();

  ImGui::Render();
}

void MtuOverlay::render(
  const DxvkContextObjects& ctx) {

  if (!m_visible)
    return;

  ImGui::SetCurrentContext(m_imgui);

  if (!m_initialized)
    init(ctx);

  VkCommandBuffer cmd =
    ctx.cmd->getCmdBuffer(DxvkCmdBuffer::ExecBuffer);

  ImGui_ImplVulkan_RenderDrawData(
    ImGui::GetDrawData(),
    cmd);
}

bool MtuOverlay::processMessage(
  HWND hWnd, UINT msg,
  WPARAM wParam, LPARAM lParam) {

  if (msg == WM_KEYDOWN && wParam == VK_F12) {
    m_visible = !m_visible;
    return true;
  }

  ImGui::SetCurrentContext(m_imgui);

  if (ImGui_ImplWin32_WndProcHandler(
        hWnd, msg, wParam, lParam))
    return true;

  return false;
}

void MtuOverlay::renderUI() {
  ImGui::SetNextWindowSize(
    ImVec2(400, 200),
    ImGuiCond_FirstUseEver);

  if (ImGui::Begin("MTU Overlay", &m_visible)) {
    ImGui::Text("DXVK D3D9 Stable Overlay");
    ImGui::Separator();
    ImGui::Text("RE6 / Rev2 Safe Mode");
    ImGui::Text("Press F12 to toggle");
  }

  ImGui::End();
}

}