#!/usr/bin/env bash
#
# Instalador guiado de LensProfileOFX (Fase 1 - Aberracion Cromatica).
# Pensado para vos, iterando sobre el codigo: compila desde el fuente y
# deja el plugin instalado y listo para probar en Resolve Studio.
#
# Si en cambio queres generar un .pkg de doble clic para que alguien mas lo
# instale sin Xcode ni Terminal, usa installer/build_installer.sh despues
# de compilar (ver README.md), o mejor: usa el workflow automatico de
# GitHub Actions (ver GUIA-INSTALADOR-SIN-CODIGO.md).
#
# Uso:
#   ./install.sh            interactivo, pide confirmacion antes de instalar
#   ./install.sh -y         no pregunta nada, instala directo
#   ./install.sh --uninstall   desinstala (delega en uninstall.sh)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BUNDLE_NAME="LensProfileOFX.ofx.bundle"
INSTALL_DIR="/Library/OFX/Plugins"
ASSUME_YES=0

for arg in "$@"; do
    case "$arg" in
        -y|--yes) ASSUME_YES=1 ;;
        --uninstall) exec "${SCRIPT_DIR}/uninstall.sh" ;;
        -h|--help)
            echo "Uso: $0 [-y|--yes] [--uninstall]"
            exit 0
            ;;
        *)
            echo "Argumento no reconocido: $arg (usa --help)"
            exit 1
            ;;
    esac
done

step() { printf "\n[%s] %s\n" "$1" "$2"; }
ok()   { printf "    OK: %s\n" "$1"; }
fail() { printf "    ERROR: %s\n" "$1" >&2; exit 1; }

echo "=========================================================="
echo " LensProfileOFX - Fase 1 - Instalador"
echo "=========================================================="

# ---------------------------------------------------------------------------
step "1/7" "Verificando que estamos en macOS..."
if [[ "$(uname -s)" != "Darwin" ]]; then
    fail "Este instalador es solo para macOS. El backend Windows/CUDA es la Fase 8 de la hoja de ruta."
fi
ok "macOS detectado."

# ---------------------------------------------------------------------------
step "2/7" "Verificando Xcode Command Line Tools..."
if ! xcode-select -p >/dev/null 2>&1; then
    fail "No estan instaladas. Corre esto y volve a intentar:
        xcode-select --install"
fi
ok "Xcode Command Line Tools presentes."

# ---------------------------------------------------------------------------
step "3/7" "Verificando CMake (>= 3.20)..."
if ! command -v cmake >/dev/null 2>&1; then
    fail "No se encontro cmake. Instalalo con:
        brew install cmake
    (si no tenes Homebrew: https://brew.sh)"
fi
CMAKE_VERSION="$(cmake --version | head -1 | awk '{print $3}')"
ok "cmake ${CMAKE_VERSION} encontrado."

# ---------------------------------------------------------------------------
step "4/7" "Verificando si DaVinci Resolve esta corriendo..."
if pgrep -x "Resolve" >/dev/null 2>&1; then
    echo "    Resolve esta abierto ahora mismo."
    echo "    Hay que cerrarlo por completo (Resolve > Quit Resolve, no solo el proyecto)"
    echo "    para que la instalacion tome efecto la proxima vez que lo abras."
    if [[ "$ASSUME_YES" -eq 0 ]]; then
        read -r -p "    Cerralo y presiona Enter para continuar (o Ctrl+C para cancelar)... "
        if pgrep -x "Resolve" >/dev/null 2>&1; then
            fail "Resolve sigue corriendo. Cerralo del todo y volve a correr ./install.sh"
        fi
    fi
else
    ok "Resolve no esta corriendo."
fi

# ---------------------------------------------------------------------------
step "5/7" "Confirmacion..."
echo "    Esto va a:"
echo "      - Compilar LensProfileOFX desde ${SCRIPT_DIR}"
echo "      - Descargar el OpenFX SDK y nlohmann/json la primera vez (necesita internet)"
echo "      - Copiar el resultado a ${INSTALL_DIR}/${BUNDLE_NAME} (necesita tu contrasena de administrador)"
if [[ "$ASSUME_YES" -eq 0 ]]; then
    read -r -p "    Continuar? [s/N] " REPLY
    case "$REPLY" in
        s|S|si|Si|SI|y|Y|yes) ;;
        *) echo "    Cancelado."; exit 0 ;;
    esac
fi

# ---------------------------------------------------------------------------
step "6/7" "Compilando (esto puede tardar varios minutos la primera vez)..."
cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release \
    || fail "Fallo la configuracion de CMake. Revisa el mensaje de arriba."
cmake --build "${BUILD_DIR}" --config Release \
    || fail "Fallo la compilacion. Copiame el error completo y lo resolvemos."

BUNDLE_SRC="${BUILD_DIR}/${BUNDLE_NAME}"
[[ -d "${BUNDLE_SRC}" ]] || fail "La compilacion termino pero no se genero ${BUNDLE_SRC}."
ok "Compilado: ${BUNDLE_SRC}"

# ---------------------------------------------------------------------------
step "7/7" "Instalando en ${INSTALL_DIR}..."
sudo mkdir -p "${INSTALL_DIR}"
sudo rm -rf "${INSTALL_DIR:?}/${BUNDLE_NAME}"
sudo cp -R "${BUNDLE_SRC}" "${INSTALL_DIR}/"
sudo chmod -R a+rX "${INSTALL_DIR}/${BUNDLE_NAME}"

if [[ -d "${INSTALL_DIR}/${BUNDLE_NAME}" ]]; then
    ok "Instalado en ${INSTALL_DIR}/${BUNDLE_NAME}"
else
    fail "La copia no llego a destino. Revisa permisos de ${INSTALL_DIR}."
fi

echo ""
echo "=========================================================="
echo " Instalacion completa."
echo "=========================================================="
echo " Proximos pasos:"
echo "   1. Abri (o reabri) DaVinci Resolve Studio."
echo "   2. Panel Effects -> OpenFX -> grupo 'Bitacora Films'."
echo "   3. Arrastra 'Lens Profile - Aberracion Cromatica' sobre un clip."
echo ""
echo " Para desinstalar: ./uninstall.sh"
echo "=========================================================="
