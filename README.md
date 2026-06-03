# Neko Void Updater 🚀

The Definitive Update Manager for Void Linux

Neko Void Updater is a modern, efficient utility designed specifically for Void Linux users who want an update experience similar to distributions like Linux Mint or Fedora, while maintaining the lightweight nature and granular control of Void.

The application is split into two core components:

**Neko Core (Rust):** A high-performance engine that interfaces directly with xbps and flatpak to manage packages and system maintenance.

**Neko Tray (C++/Qt):** A sleek, minimalist system tray application that handles real-time alerts and user interaction.

## Technical Specifications
* **Core Language:** Rust (Ensures memory safety and maximum execution speed).
* **Interface Language:** C++ with Qt 6 and QML (Fluid, hardware-accelerated UI).
* **Visual Theme:** Dracula / Catppuccin color palette with Nerd Fonts support.
* **Supported Package Managers:** XBPS (System) and Flatpak (User applications).
* **Localization:** Bilingual (English / Spanish) with automatic system locale detection.

## Key Features
* **Silent Synchronization:** Includes a background daemon that updates repository indexes hourly without interrupting user workflow.
* **Start to Tray:** Launches directly into the system notification area to keep the workspace clean.
* **Persistent State:** The system tracks and remembers the last update check timestamp across system reboots.
* **Deep Maintenance:** Built-in maintenance tools to safely remove orphaned packages and clear the XBPS package cache.
* **Security Integration:** Native integration with Polkit (pkexec) for operations requiring elevated privileges.

## Installation Guide

### 1. Prerequisites
Ensure the following packages are installed on your system:

```bash
sudo xbps-install -S rust cargo qt6-base-devel qt6-declarative-devel polkit
```

### 2. Compiling the Core Engine
Navigate to the core directory and build the release binary:

```bash
cd neko-updater-core
cargo build --release
```
The compiled binary will be located at `target/release/neko-updater-core`.

### 3. Compiling the Tray Interface
Navigate to the tray directory, generate the Makefile using Qt 6, and compile:

```bash
cd ../neko-updater-tray
qmake6
make -j$(nproc)
```
The executable `neko-updater-tray` will be generated in the root of that directory.

## Service Configuration and Activation

### Step 1: Enable the Synchronization Daemon
This service ensures that the XBPS local database is kept up to date in the background.

Copy the service unit file:
```bash
sudo cp neko-void-sync.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now neko-void-sync.service
```

### Step 2: Configure Desktop Autostart
To ensure the tray icon initializes upon user login:
```bash
mkdir -p ~/.config/autostart
cp neko-void-updater.desktop ~/.config/autostart/
```

## Usage Notes
* **Left Click:** Toggles the visibility of the primary control window.
* **Right Click:** Opens the context menu providing fast access to check frequencies, maintenance tools, and application exit options.
* **Logging:** Execution logs and runtime diagnostics can be inspected at `/tmp/neko_updater.log`.

Developed by ErzaGOD19 for the Neko Void Linux Trinity Community.
