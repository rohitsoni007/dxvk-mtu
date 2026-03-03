# DXVK + ImGui Overlay Integration Plan
Base Version: DXVK v2.6.2  
Project: MT-Framework Upscaler / MTU-DXVK  
Target: D3D9 (primary), extensible to D3D11 later  
Architecture: x32 (32-bit) focus  

---

# 1. Objectives

- Integrate Dear ImGui directly into DXVK (no external injector).
- Render overlay on top of swapchain output.
- Support D3D9 first (RE6 target).
- Minimal performance impact.
- Toggle overlay with hotkey (F11).
- Designed to later expose MTU upscaler controls.

---

# 2. High-Level Architecture

Game (D3D9)  
    ↓  
DXVK D3D9 layer  
    ↓  
Vulkan backend  
    ↓  
ImGui Overlay Pass (New)  
    ↓  
Present

Overlay will be injected inside DXVK swapchain present path.

---

# 3. Integration Strategy

## 3.1 Where to Hook

Primary hook location:

src/d3d9/d3d9_swapchain.cpp

Function:
- `D3D9SwapChainEx::PresentImage()`

Overlay rendering must occur:
- After game rendering is complete (frame is submitted to CS)
- Before the final present call to the Vulkan presenter.

---

# 4. Required Components

## 4.1 Add Dear ImGui

Add as subproject:
`subprojects/imgui`

Backend to use:
- `imgui_impl_vulkan.cpp`
- `imgui_impl_win32.cpp` (for message handling)

---

# 5. New Module Structure

Create:
`src/dxvk/overlay/`

Files:
- `overlay_manager.h` / `.cpp`: Lifecycle and context management.
- `overlay_renderer.h` / `.cpp`: Vulkan rendering logic.
- `overlay_input.h` / `.cpp`: Hotkey and message interception.

---

# 6. Vulkan Integration Plan

## 6.1 Initialization

On first swapchain creation or device init:
- Create ImGui context.
- Initialize Vulkan backend (reusing DXVK device/queues).
- Create dedicated descriptor pool.

---

# 7. Render Pass Strategy

Use Option B:
- End main rendering section.
- Start a specific command recording for ImGui.
- Use the swapchain's current backbuffer as the render target.

---

# 8. Thread Safety

- Overlay logic must execute on the thread that calls `Present`.
- Coordinate with DXVK's CS (Command Stream) thread to ensure rendering completes correctly.

---

# 9. Build System Changes

Modify:
- `meson.build`: Add `imgui` subproject and new sources.
- `meson_options.txt`: Add `enable_overlay` option.

---

# 10. Success Criteria

- Overlay toggles with F11 in D3D9 games (x32).
- No performance regressions when overlay is hidden.
- UI is responsive and stable.
