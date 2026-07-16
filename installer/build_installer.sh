#!/usr/bin/env bash
#
# Genera LensProfileOFX-Installer.pkg: un instalador nativo de macOS
# (doble clic, asistente grafico, sin Terminal ni Xcode necesarios del lado
# de quien lo instala).
#
# Requisito: el plugin YA tiene que estar compilado. Corre primero:
#   ../install.sh
# o al menos:
#   cmake -S .. -B ../build && cmake --build ../build
#
# Uso:
#   ./build_installer.sh                 usa ../build/LensProfileOFX.ofx.bundle
#   ./build_installer.sh /ruta/al/bundle  usa un bundle en otra ubicacion

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

BUNDLE_NAME="LensProfileOFX.ofx.bundle"
BUNDLE_SRC="${1:-${PROJECT_DIR}/build/${BUNDLE_NAME}}"
PKG_IDENTIFIER="cl.bitacorafilms.LensProfileOFX.pkg"
PKG_VERSION="0.1.0"
OUTPUT_PKG="${PROJECT_DIR}/LensProfileOFX-Installer.pkg"

step() { printf "\n[%s] %s\n" "$1" "$2"; }
fail() { printf "    ERROR: %s\n" "$1" >&2; exit 1; }

echo "=========================================================="
echo " LensProfileOFX - Generador de instalador .pkg"
echo "=========================================================="

step "1/4" "Verificando requisitos..."
[[ "$(uname -s)" == "Darwin" ]] || fail "Esto solo corre en macOS (pkgbuild/productbuild son de Apple)."
command -v pkgbuild >/dev/null 2>&1 || fail "No se encontro pkgbuild. Instala Xcode Command Line Tools."
command -v productbuild >/dev/null 2>&1 || fail "No se encontro productbuild. Instala Xcode Command Line Tools."
[[ -d "${BUNDLE_SRC}" ]] || fail "No encontre ${BUNDLE_SRC}. Compila el plugin primero (ver comentario arriba)."
echo "    Bundle a empaquetar: ${BUNDLE_SRC}"

step "2/4" "Armando el arbol de archivos del paquete..."
PKG_ROOT="$(mktemp -d)"
trap 'rm -rf "${PKG_ROOT}"' EXIT

mkdir -p "${PKG_ROOT}/Library/OFX/Plugins"
cp -R "${BUNDLE_SRC}" "${PKG_ROOT}/Library/OFX/Plugins/${BUNDLE_NAME}"
chmod +x "${SCRIPT_DIR}/scripts/postinstall"

step "3/4" "Empaquetando componente (pkgbuild)..."
COMPONENT_PKG="${PKG_ROOT}/component.pkg"
pkgbuild \
    --root "${PKG_ROOT}" \
    --identifier "${PKG_IDENTIFIER}" \
    --version "${PKG_VERSION}" \
    --install-location "/" \
    --scripts "${SCRIPT_DIR}/scripts" \
    "${COMPONENT_PKG}" \
    || fail "pkgbuild fallo. Revisa el mensaje de arriba."

step "4/4" "Generando el instalador final (productbuild)..."
productbuild \
    --distribution "${SCRIPT_DIR}/Distribution.xml" \
    --resources "${SCRIPT_DIR}/resources" \
    --package-path "${PKG_ROOT}" \
    "${OUTPUT_PKG}" \
    || fail "productbuild fallo. Revisa el mensaje de arriba."

echo ""
echo "=========================================================="
echo " Listo: ${OUTPUT_PKG}"
echo "=========================================================="
echo " Se lo podes pasar a cualquiera con macOS y Resolve instalado."
echo " Al hacer doble clic se abre el instalador nativo de macOS:"
echo " sigue el asistente, pone su contrasena de administrador, listo."
echo " No necesita Xcode, CMake, ni Terminal."
echo "=========================================================="
