#pragma once

#include "LensProfile.h"
#include <string>

// Carga un perfil óptico desde un archivo JSON en disco.
//
// Deliberadamente no depende de ningún header de OFX ni de Metal: es lógica
// pura de C++ que se puede compilar y probar de forma aislada (ver
// tests/test_profile_loader.cpp), incluso en una máquina sin Xcode.

namespace LensProfile {

// Lanza std::runtime_error si el archivo no existe, no se puede abrir, o el
// JSON no es válido / no tiene el bloque "chromatic_aberration".
Profile LoadProfileFromFile(const std::string& jsonPath);

}  // namespace LensProfile
