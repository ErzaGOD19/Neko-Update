# Neko Void Updater - Core (Rust) & Tray (Qt6) Makefile
# Copyright (C) 2026 ErzaGOD19 - Trinity Community

CORE_DIR = neko-updater-core
CORE_BIN = $(CORE_DIR)/target/release/neko-updater-core
TRAY_DIR = neko-updater-tray
TRAY_BIN = $(TRAY_DIR)/neko-updater-tray

all: $(CORE_BIN) $(TRAY_BIN)

$(CORE_BIN):
	@echo "Compilando Core (Rust)..."
	cd $(CORE_DIR) && cargo build --release

$(TRAY_BIN):
	@echo "Compilando Tray (Qt6/Qt5 con CMake)..."
	mkdir -p $(TRAY_DIR)/build
	cd $(TRAY_DIR)/build && cmake .. && make
	cp $(TRAY_DIR)/build/neko-updater-tray $(TRAY_BIN)

clean:
	rm -f $(TRAY_BIN)
	rm -rf $(TRAY_DIR)/build
	-cd $(CORE_DIR) && cargo clean

install: all
	@echo "Instalando Neko Void Updater (Rust/Qt6 Edition)..."
	./install.sh

uninstall:
	@echo "Desinstalando Neko Void Updater..."
	rm -f /usr/bin/neko-updater-core
	rm -f /usr/bin/neko-updater-tray
	rm -rf /usr/share/neko-void
	rm -rf /usr/share/neko-void/translations
	rm -f /usr/share/icons/hicolor/scalable/apps/normal.png
	rm -f /usr/share/icons/hicolor/scalable/apps/warnig.png
	rm -f /usr/share/icons/hicolor/scalable/apps/working.png
	rm -f /usr/share/icons/hicolor/scalable/apps/neko-updater.png
	rm -f /usr/share/pixmaps/neko-updater.png
	rm -f /usr/share/polkit-1/actions/org.neko_void.updater.policy
	rm -f /usr/share/applications/neko-void-updater.desktop
	rm -f /etc/xdg/autostart/neko-void-updater.desktop
	rm -rf /etc/sv/neko-void-sync
	rm -f /var/service/neko-void-sync
.PHONY: all clean install uninstall
