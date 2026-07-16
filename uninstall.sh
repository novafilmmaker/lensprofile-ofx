#!/usr/bin/env bash
#
# Desinstala LensProfileOFX de /Library/OFX/Plugins.
#
# Uso:
#   ./uninstall.sh          pide confirmacion
#   ./uninstall.sh -y       no pregunta, borra directo

set -euo pipefail

BUNDLE_NAME="LensProfileOFX.ofx.bundle"
INSTALL_DIR="/Library/OFX/Plugins"
TARGET="${INSTALL_DIR}/${BUNDLE_NAME}"
ASSUME_YES=0

for arg in "$@"; do
    case "$arg" in
        -y|--yes) ASSUME_YES=1 ;;
        -h|--help) echo "Uso: $0 [-y|--yes]"; exit 0 ;;
    esac
done

if [[ ! -d "${TARGET}" ]]; then
    echo "No hay nada instalado en ${TARGET}. Nada que hacer."
    exit 0
fi

if pgrep -x "Resolve" >/dev/null 2>&1; then
    echo "DaVinci Resolve esta corriendo. Cerralo antes de desinstalar para evitar"
    echo "que quede referenciando un efecto que ya no existe."
fi

if [[ "$ASSUME_YES" -eq 0 ]]; then
    read -r -p "Vas a borrar ${TARGET}. Continuar? [s/N] " REPLY
    case "$REPLY" in
        s|S|si|Si|SI|y|Y|yes) ;;
        *) echo "Cancelado."; exit 0 ;;
    esac
fi

echo "Borrando ${TARGET} (te va a pedir tu contrasena de administrador)..."
sudo rm -rf "${TARGET}"
echo "Listo. LensProfileOFX desinstalado."
