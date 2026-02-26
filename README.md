# dxvk-mtu

**dxvk-mtu** is a specialized fork of [DXVK](https://github.com/doitsujin/dxvk) designed to integrate advanced temporal upscaling into MT Framework games. It adds a high-performance path for **FidelityFX Super Resolution 2.2 (FSR 2.2)** and provides a robust in-game overlay for real-time configuration.

## Features

-   **Integrated FSR 2.2**: High-quality temporal upscaling for smoother performance and improved visuals.
-   **In-Game Overlay**: A premium ImGui-based interface to adjust settings on the fly.
-   **Real-Time Configuration**:
    -   Adjust upscaling presets (Quality, Balanced, Performance, Ultra Performance).
    -   Control sharpening (RCAS) intensity.
    -   Advanced parameters: Jitter scale, Mip Bias offset, Auto Exposure, and Depth inversion.
-   **Persistent Settings**: Save your preferences directly to disk from the overlay.
-   **Vulkan Powered**: Leverages modern Vulkan features for efficient rendering.

## Usage & Overlay

The core of the MTU experience is the configuration overlay.

-   **Toggle Overlay**: Press **`F12`** while in-game.
-   **Default State**: The overlay is **enabled by default** on first launch to confirm it is working. Use F12 to hide it.

Using the overlay, you can enable/disable the upscaler and fine-tune every aspect of the image reconstruction.

## Installation

1.  **Download**: Obtain the latest build artifacts (`d3d9.dll`, `dxvk-mtu.dll`).
2.  **Plugin**: Ensure `mtu_upscaler.dll` is present in your game's executable directory.
3.  **Place DLLs**: Copy the built `d3d9.dll` into the game folder (where the original game executable resides).
4.  **Launch**: Run the game. If successful, you can press **`F12`** to see the MTU settings menu.

## Building

To build **dxvk-mtu**, you need a working Meson and Ninja environment with cross-compilation support for MinGW.

### Cross-Compilation (Linux/Steam Deck)

Initialize submodules first:
```bash
git submodule update --init --recursive
```

Use the provided cross-files for building:

**64-bit Build:**
```bash
meson setup --cross-file build-win64.txt --buildtype release build.64
cd build.64
ninja
```

**32-bit Build:**
```bash
meson setup --cross-file build-win32.txt --buildtype release build.32
cd build.32
ninja
```

./local-release.sh build 2 --no-package
./package-release.sh build 2 --no-package

package-release

### Build Artifacts
Upon completion, the DLLs will be located in the `bin/` directory within your build folder (e.g., `build.64/bin/d3d9.dll`).

## Requirements

-   **Vulkan 1.3 Compatible GPU**: Required for the core DXVK and FSR 2 implementation.
-   **MT Framework Game**: While it may work elsewhere, it is specifically tuned for MT Framework.

## Development

To build the project, ensure you have the necessary submodules initialized:

```bash
git submodule update --init --recursive
```

Refer to the original DXVK instructions for build environment setup (Meson/Ninja).

## Troubleshooting & Logging

If you encounter issues (e.g., the upscaler is not working or the overlay doesn't appear):

1.  **Check Logs**: DXVK generates log files in the game directory (e.g., `executable_d3d9.log`).
2.  **Enable Detailed Logging**: Set the following environment variables or add them to your `dxvk.conf`:
    -   `dxvk.logLevel = info`
    -   `d3d9.logLevel = info`
3.  **Debug Utils**: The user can also enable `dxvk.enableDebugUtils = True` in `dxvk.conf` for Vulkan-level debugging.

---
*Powered by DXVK and AMD FidelityFX.*
