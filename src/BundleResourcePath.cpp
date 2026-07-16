#include "BundleResourcePath.h"

#include <dlfcn.h>
#include <climits>
#include <cstdlib>

namespace LensProfile {

namespace {

// Cualquier símbolo de esta librería sirve de "ancla": dladdr() nos dice
// desde qué archivo binario fue cargada la función que le pasemos, y ese
// binario es justamente LensProfileOFX.ofx (dentro de Contents/MacOS/).
void AnchorSymbol() {}

}  // namespace

std::string GetProfilesDirectory()
{
    Dl_info info;
    if (dladdr(reinterpret_cast<void*>(&AnchorSymbol), &info) == 0 ||
        info.dli_fname == nullptr) {
        return std::string();
    }

    std::string binaryPath(info.dli_fname);

    // Resolvemos symlinks / rutas relativas a una ruta absoluta real.
    char resolved[PATH_MAX];
    if (realpath(binaryPath.c_str(), resolved) != nullptr) {
        binaryPath = resolved;
    }

    // Estructura esperada:
    // .../LensProfileOFX.ofx.bundle/Contents/MacOS/LensProfileOFX.ofx
    const std::string marker = "/Contents/MacOS/";
    size_t pos = binaryPath.rfind(marker);
    if (pos == std::string::npos) {
        return std::string();
    }

    std::string contentsDir = binaryPath.substr(0, pos) + "/Contents";
    return contentsDir + "/Resources/profiles/";
}

}  // namespace LensProfile
