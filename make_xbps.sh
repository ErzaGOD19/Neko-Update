#!/bin/bash
# Neko Void Updater - XBPS Package Generator

echo "󰄛 Preparando construcción del paquete .xbps..."

# 1. Compilar componentes
echo "󰚰 Compilando Core (Rust)..."
cd neko-updater-core && cargo build --release && cd ..

echo "󰚰 Compilando Tray (C++/Qt)..."
cd neko-updater-tray && /usr/lib/qt6/bin/qmake6 && make clean && make && cd ..

# 2. Crear estructura del paquete (Root del paquete)
rm -rf pkg
mkdir -p pkg/usr/bin
mkdir -p pkg/usr/share/neko-void
mkdir -p pkg/usr/share/applications
mkdir -p pkg/usr/share/pixmaps
mkdir -p pkg/usr/share/polkit-1/actions
mkdir -p pkg/etc/xdg/autostart
mkdir -p pkg/etc/sv/neko-void-sync

# 3. Copiar archivos a la estructura
echo "󱐋 Organizando archivos..."
cp neko-updater-core/target/release/neko-updater-core pkg/usr/bin/
cp neko-updater-tray/neko-updater-tray pkg/usr/bin/
cp -r neko-updater-core/Data/* pkg/usr/share/neko-void/
cp neko-updater-core/Data/logo.png pkg/usr/share/pixmaps/neko-updater.png
cp neko-void-updater.desktop pkg/usr/share/applications/
cp neko-void-updater.desktop pkg/etc/xdg/autostart/
cp void.pkexec.xbps.policy pkg/usr/share/polkit-1/actions/org.neko_void.updater.policy
cp neko-void-sync/run pkg/etc/sv/neko-void-sync/run
chmod +x pkg/etc/sv/neko-void-sync/run

# 4. Generar el paquete XBPS
echo "󰄛 Generando archivo .xbps final..."
# Obtenemos la arquitectura actual
ARCH=$(uname -m)

xbps-create -A "$ARCH" -n "neko-void-updater-1.0.0_1" \
    -s "Modern system updater for Neko Void Linux (XBPS + Flatpak)" \
    -m "Alexander <https://github.com/ErzaGOD19>" \
    pkg/
if [ $? -eq 0 ]; then
    echo ""
    echo "---------------------------------------------------"
    echo "¡ÉXITO! Paquete creado para $ARCH"
    echo "Para instalarlo localmente:"
    echo "sudo xbps-install --repository=. neko-void-updater"
    echo "---------------------------------------------------"
else
    echo "Error: Asegúrate de tener las herramientas de xbps instaladas."
fi
