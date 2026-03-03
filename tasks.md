# ImGui Integration Tasks

## Phase 1: Preparation
- [x] Analyze existing `plan.md`
- [x] Research ImGui integration with DXVK/Vulkan
- [x] Update `plan.md` with researched details
- [ ] Add ImGui as a Meson subproject

## Phase 2: Core Implementation
- [ ] Create `src/dxvk/overlay` directory
- [ ] Implement `OverlayManager` (context and lifecycle)
- [ ] Implement `OverlayRenderer` (Vulkan draw data)
- [ ] Implement `OverlayInput` (Win32 message handle)

## Phase 3: D3D9 Integration
- [ ] Initialize overlay in `D3D9Device`
- [ ] Hook `D3D9SwapChainEx::PresentImage`
- [ ] Implement visibility toggle (F11)

## Phase 4: Verification
- [ ] Build for x32
- [ ] Test with Resident Evil 6 (D3D9)
- [ ] Verify input capture and rendering stability
