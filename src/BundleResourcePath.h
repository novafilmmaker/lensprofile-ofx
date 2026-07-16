#pragma once

#include <string>

// Ubica la carpeta de recursos (Contents/Resources/...) del propio
// LensProfileOFX.ofx.bundle en tiempo de ejecución, para poder leer los
// perfiles JSON que viajan empaquetados junto al binario.

namespace LensProfile {

// Devuelve la ruta absoluta a "Contents/Resources/profiles/" dentro del
// bundle que contiene este binario, terminada en "/". Devuelve una cadena
// vacía si no se pudo determinar (por ejemplo, si el binario no está dentro
// de un bundle .ofx.bundle con la estructura esperada).
std::string GetProfilesDirectory();

}  // namespace LensProfile
