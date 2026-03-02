#pragma once

#include <vector>
#include <mutex>
#include <memory>
#include <functional>

#include "../dxvk/dxvk_device.h"
#include "../dxvk/dxvk_context.h"
#include "../dxvk/hud/dxvk_hud_renderer.h"

#include <windows.h>
#include "../d3d9/mtu_plugin_loader.h"

#include "imgui/imgui.h"

namespace dxvk {

  /**
   * \brief MTU Overlay Management
   * 
   * Handles ImGui lifecycle, Vulkan backend, and UI logic
   * for the in-game configuration overlay.
   */

   enum class ConfigValueType {
      Bool,
      Int,
      Float,
      String,
      AutoBool,
      FloatEmulation,
      D3D9ShaderModel,
      D3D11FeatureLevel
  };

  struct ConfigOption
{
    const char* section;     // "dxvk", "dxgi", "d3d11", etc
    const char* key;         // "dxvk.allowFse"
    const char* label;       // UI label

    ConfigValueType type;

    void* valuePtr;          // pointer into DxvkConfig

    // numeric ranges (optional)
    int   intMin = 0;
    int   intMax = 0;
    float floatMin = 0.0f;
    float floatMax = 0.0f;
};

  static const char* kAutoBool[] = { "Auto", "True", "False" };
  static const char* kFloatEmulation[] = { "Auto", "True", "False", "Strict" };
  static const char* kD3D9ShaderModel[] = { "0", "1", "2", "3" };
  static const char* kD3D11FeatureLevel[] = {
      "9_1","9_2","9_3","10_0","10_1",
      "11_0","11_1","12_0","12_1"
  };

   enum class AutoBool {
    Auto = 0,
    True,
    False
  };

  enum class FloatEmulationMode {
      Auto = 0,
      True,
      False,
      Strict
  };

  enum class D3D9ShaderModel {
      SM0 = 0,   // Fixed function
      SM1,
      SM2,
      SM3
  };

  enum class D3D11FeatureLevel {
      FL_9_1 = 0,
      FL_9_2,
      FL_9_3,
      FL_10_0,
      FL_10_1,
      FL_11_0,
      FL_11_1,
      FL_12_0,
      FL_12_1
  };
  
  struct DxvkConfig {
    // =========================
    // dxvk.*
    // =========================

    std::string dxvk_deviceFilter;

    bool        dxvk_allowFse = false;
    AutoBool    dxvk_latencySleep = AutoBool::Auto;
    int32_t     dxvk_latencyTolerance = 1000;
    AutoBool    dxvk_disableNvLowLatency2 = AutoBool::Auto;

    AutoBool    dxvk_tearFree = AutoBool::Auto;
    AutoBool    dxvk_tilerMode = AutoBool::Auto;

    bool        dxvk_zeroMappedMemory = false;

    int32_t     dxvk_numCompilerThreads = 0;

    AutoBool    dxvk_useRawSsbo = AutoBool::Auto;
    AutoBool    dxvk_enableGraphicsPipelineLibrary = AutoBool::Auto;
    AutoBool    dxvk_enableDescriptorHeap = AutoBool::Auto;
    AutoBool    dxvk_enableDescriptorBuffer = AutoBool::Auto;

    bool        dxvk_enableUnifiedImageLayouts = true;
    bool        dxvk_enableImplicitResolves = true;

    AutoBool    dxvk_trackPipelineLifetime = AutoBool::Auto;
    AutoBool    dxvk_enableMemoryDefrag = AutoBool::Auto;

    std::string dxvk_hud;

    bool        dxvk_enableDebugUtils = false;
    bool        dxvk_hideIntegratedGraphics = false;

    int32_t     dxvk_maxMemoryBudget = 0;


    // =========================
    // dxgi.*
    // =========================

    bool        dxgi_enableHDR = false;
    bool        dxgi_enableDummyCompositionSwapchain = false;
    bool        dxgi_enableUe4Workarounds = false;
    bool        dxgi_deferSurfaceCreation = false;

    int32_t     dxgi_maxFrameLatency = 0;
    int32_t     dxgi_maxFrameRate = 0;

    std::string dxgi_customDeviceId;     // 4-digit hex
    std::string dxgi_customVendorId;     // 4-digit hex
    std::string dxgi_customDeviceDesc;

    AutoBool    dxgi_hideNvidiaGpu = AutoBool::Auto;
    AutoBool    dxgi_hideNvkGpu = AutoBool::Auto;
    AutoBool    dxgi_hideAmdGpu = AutoBool::Auto;
    AutoBool    dxgi_hideIntelGpu = AutoBool::Auto;

    int32_t     dxgi_maxDeviceMemory = 0;
    int32_t     dxgi_maxSharedMemory = 0;

    int32_t     dxgi_syncInterval = -1;
    int32_t     dxgi_forceRefreshRate = 0;


    // =========================
    // d3d11.*
    // =========================

    D3D11FeatureLevel d3d11_maxFeatureLevel = D3D11FeatureLevel::FL_12_1;

    int32_t     d3d11_maxTessFactor = 0;

    bool        d3d11_relaxedBarriers = false;
    bool        d3d11_relaxedGraphicsBarriers = false;

    int32_t     d3d11_samplerAnisotropy = -1;
    float       d3d11_samplerLodBias = 0.0f;
    bool        d3d11_clampNegativeLodBias = false;

    bool        d3d11_forceSampleRateShading = false;
    bool        d3d11_disableMsaa = false;

    bool        d3d11_forceComputeLdsBarriers = false;
    bool        d3d11_forceComputeUavBarriers = false;

    bool        d3d11_disableDirectImageMapping = false;
    bool        d3d11_enableContextLock = false;

    bool        d3d11_exposeDriverCommandLists = true;
    bool        d3d11_reproducibleCommandStream = false;


    // =========================
    // d3d9.*
    // =========================

    bool        d3d9_deferSurfaceCreation = false;
    int32_t     d3d9_maxFrameLatency = 0;
    int32_t     d3d9_maxFrameRate = 0;

    std::string d3d9_customDeviceId;
    std::string d3d9_customVendorId;
    std::string d3d9_customDeviceDesc;

    AutoBool    d3d9_hideNvidiaGpu = AutoBool::Auto;
    AutoBool    d3d9_hideNvkGpu = AutoBool::Auto;
    AutoBool    d3d9_hideAmdGpu = AutoBool::Auto;
    AutoBool    d3d9_hideIntelGpu = AutoBool::True;

    int32_t     d3d9_presentInterval = -1;

    int32_t     d3d9_samplerAnisotropy = -1;
    float       d3d9_samplerLodBias = 0.0f;
    bool        d3d9_clampNegativeLodBias = false;

    bool        d3d9_invariantPosition = true;
    bool        d3d9_forceSampleRateShading = false;

    bool        d3d9_reproducibleCommandStream = false;

    D3D9ShaderModel d3d9_shaderModel = D3D9ShaderModel::SM3;

    bool        d3d9_dpiAware = true;
    bool        d3d9_strictConstantCopies = false;
    bool        d3d9_strictPow = true;
    bool        d3d9_lenientClear = false;

    int32_t     d3d9_maxAvailableMemory = 4096;
    bool        d3d9_memoryTrackTest = false;

    FloatEmulationMode d3d9_floatEmulation = FloatEmulationMode::Auto;

    AutoBool    dxvk_lowerSinCos = AutoBool::Auto;

    bool        d3d9_deviceLocalConstantBuffers = false;
    bool        d3d9_supportDFFormats = true;
    bool        d3d9_useD32forD24 = false;
    bool        d3d9_supportX4R4G4B4 = true;
    bool        d3d9_disableA8RT = false;
    bool        d3d9_forceSamplerTypeSpecConstants = false;

    std::string d3d9_forceAspectRatio;

    int32_t     d3d9_forceRefreshRate = 0;

    bool        d3d9_modeCountCompatibility = false;
    bool        d3d9_enumerateByDisplays = true;

    bool        d3d9_cachedWriteOnlyBuffers = false;
    bool        d3d9_seamlessCubes = false;

    int32_t     d3d9_textureMemory = 100;

    bool        d3d9_deviceLossOnFocusLoss = false;
    bool        d3d9_countLosableResources = true;
    bool        d3d9_extraFrontbuffer = false;


    // =========================
    // d3d8.*
    // =========================

    int32_t     d3d8_scaleDref = 0;
    bool        d3d8_shadowPerspectiveDivide = false;
    std::string d3d8_forceVsDecl;
    bool        d3d8_batching = false;
    bool        d3d8_placeP8InScratch = false;
    bool        d3d8_forceLegacyDiscard = false;
  };

  class MtuOverlay : public RcObject {
  public:
    MtuOverlay(const Rc<DxvkDevice>& device, HWND window);
    ~MtuOverlay();

    /**
     * \brief Update overlay state
     */
    void update();

    /**
     * \brief Render the overlay
     * 
     * \param [in] ctx Command list to record to
     * \param [in] dstView Final swapchain image view
     */
    void render(
      const DxvkContextObjects& ctx,
      const Rc<DxvkImageView>&  dstView);

    /**
     * \brief Process window messages for ImGui
     */
    bool processMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /**
     * \brief Toggle overlay visibility
     */
    void toggleVisibility() { m_visible = !m_visible; }
    
    bool isVisible() const { return m_visible; }

    void syncConfigFromPlugin();
    void syncConfigToPlugin();

    void setMipBiasCallback(std::function<void(float)> callback) {
      m_onMipBiasChange = std::move(callback);
    }

  private:
    Rc<DxvkDevice>    m_device;
    HWND              m_window;
    bool              m_visible = true;
    bool              m_initialized = false;
    
    MtuConfig         m_config;
    DxvkConfig            config;

    std::function<void(float)> m_onMipBiasChange;

    // ImGui state
    VkDescriptorPool  m_descriptorPool = VK_NULL_HANDLE;

    void init(const DxvkContextObjects& ctx, const Rc<DxvkImageView>& dstView);
    void setupStyle();
    void renderUI();
  };

}
