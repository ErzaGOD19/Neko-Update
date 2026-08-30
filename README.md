# Neko Void Updater

![Status](https://img.shields.io/badge/status-stable-green)

The Definitive Update Manager for Void Linux

Neko Void Updater is a modern, efficient utility designed for Void Linux users who want an update experience similar to distributions like Linux Mint or Fedora, while maintaining the lightweight nature and granular control of Void.

The application is split into two core components:

**Neko Core (Rust):** A high-performance engine that interfaces directly with xbps and flatpak to manage packages and system maintenance.

**Neko Tray (C++/Qt):** A sleek, minimalist system tray application that handles real-time alerts and user interaction.

## Technical Specifications
* **Core Language:** Rust.
* **Interface Language:** C++ with Qt 6.
* **Supported Package Managers:** XBPS (System) and Flatpak (User applications).
* **Localization:** Bilingual (English / Spanish) with automatic system locale detection.

## Build Dependencies
To compile and install Neko Void Updater, you need the following packages installed on your Void Linux system:

* `cargo`
* `rust`
* `cmake`
* `make`
* `gcc`
* `qt6-base-devel`
* `qt6-svg-devel`

## Key Features
* **Silent Synchronization:** Includes a background daemon that updates repository indexes hourly without interrupting user workflow.
* **Start to Tray:** Launches directly into the system notification area.
* **Persistent State:** Tracks and remembers the last update check timestamp.
* **Deep Maintenance:** Built-in tools to remove orphaned packages and clear the XBPS package cache.
* **Security Integration:** Native integration with Polkit (pkexec) for operations requiring elevated privileges.

## Post-Installation (Enabling the Daemon)
By default, Void Linux does not enable new services automatically. To enable the Neko background synchronization daemon, you must link it to the runit service directory:

```bash
sudo ln -s /etc/sv/neko-void-sync /var/service/
```
Once linked, the service will start automatically and run in the background.

## Usage
* **Left Click:** Toggles the visibility of the primary control window.
* **Right Click:** Opens the context menu providing fast access to check frequencies, maintenance tools, and application exit options.
* **Logging:** Execution logs and runtime diagnostics can be inspected at /tmp/neko_updater.log.

Developed by ANOMALI0x00 for the Neko Void Linux.
