# Neko Void Updater

The Definitive Update Manager for Void Linux

Neko Void Updater is a modern, efficient utility designed for Void Linux users who want an update experience similar to distributions like Linux Mint or Fedora, while maintaining the lightweight nature and granular control of Void.

The application is split into two core components:

**Neko Core (Rust):** A high-performance engine that interfaces directly with xbps and flatpak to manage packages and system maintenance.

**Neko Tray (C++/Qt):** A sleek, minimalist system tray application that handles real-time alerts and user interaction.

## Technical Specifications
* **Core Language:** Rust.
* **Interface Language:** C++ with Qt 6 and QML.
* **Visual Theme:** Dracula / Catppuccin color palette with Nerd Fonts support.
* **Supported Package Managers:** XBPS (System) and Flatpak (User applications).
* **Localization:** Bilingual (English / Spanish) with automatic system locale detection.

## Key Features
* **Silent Synchronization:** Includes a background daemon that updates repository indexes hourly without interrupting user workflow.
* **Start to Tray:** Launches directly into the system notification area.
* **Persistent State:** Tracks and remembers the last update check timestamp.
* **Deep Maintenance:** Built-in tools to remove orphaned packages and clear the XBPS package cache.
* **Security Integration:** Native integration with Polkit (pkexec) for operations requiring elevated privileges.

## Usage
* **Left Click:** Toggles the visibility of the primary control window.
* **Right Click:** Opens the context menu providing fast access to check frequencies, maintenance tools, and application exit options.
* **Logging:** Execution logs and runtime diagnostics can be inspected at /tmp/neko_updater.log.

Developed by ErzaGOD19 for the Neko Void Linux Trinity Community.
