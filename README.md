# Simple Motion2Mouse for UEVR (Alpha 0.2)

A lightweight UEVR plugin that translates your right VR controller's position directly into absolute mouse movements.

## Overview
This plugin is designed for UEVR users who want to use their VR controller as a mouse substitute for desktop applications or menu interactions within a VR-injected game.

## Status: Alpha 0.2
**Important:** This project is currently in an early **Alpha (0.2)** phase. The cursor movement is already stable and working well, requiring only some final fine-tuning and minor adjustments. Optimization and compatibility work is ongoing.

## Important Setup Requirements
Please follow these steps for the best experience:
- **Hardware:** Ensure a physical mouse and keyboard are connected to your PC.
- **Windows Settings:** Disable "Enhance pointer precision" (Mouse Acceleration) in your Windows Mouse Settings.
- **UEVR Settings (General):** Within the UEVR in-game menu, ensure that **"Always Show Cursor"** is activated.
- **UEVR Settings (Manor Lords):** Set the UI in the UEVR menue to **maximum size **; you can adjust the distance according to your preference.
- **In-Game Settings (Manor Lords):** 
  - Set the display mode to **Borderless Fullscreen** 
  - Disable the "Move mouse outside the window" and "Scroll at the edge of the screen" settings.
  - # Controls & Key Mapping
The default button assignments are currently optimized for **Manor Lords**. Please keep this in mind when testing with other titles, as the controls are hardcoded for this specific layout in this version.

## Compatibility & Testing
This plugin has been tested with the following titles:

| Game | Status | Notes |
| :--- | :--- | :--- |
| **Manor Lords** | **Great** | Optimized mappings; works perfectly. |
| **HumanitZ** | **Working** | Functional, but requires further optimization. |
| **SurrounDead** | **Working** | Functional, but requires further optimization. |
| **SpaceBourne 2** | **Limited** | Currently works in menus and UI only. |

### Hardware & Environment
- **Tested Setup:** Meta Quest 3 via Virtual Desktop, OpenXR/VDXR and UEVR Nightly 1096+. OpenVR untestet.
- **General Compatibility:** Theoretically, this plugin should work with any VR headset and controllers. However, further community testing is required to confirm full compatibility across different hardware.

## Installation
1. Download `Motion2Mouse_v0.2_alpha.zip` or the direct `Motion2Mouse.dll` from the [Releases](https://github.com/Frederic1323/UEVR-Motion2Mouse-Universal-Plugin/releases) section.
2. Unpack the `Motion2Mouse.dll` (if using the ZIP) and place it into the folder of your UEVR game plugin directory (where the UEVR game profile is installed).
4. Start the game. The plugin will initialize automatically.

## Development & Building
This project is open-source. You can view the source code in `main.cpp`. Pull requests for optimizations or better compatibility are welcome.

### Dependencies
- [UEVR Plugin API](https://github.com/praydog/UEVR)

## License
Distributed under the MIT License. See `LICENSE` for more information.
