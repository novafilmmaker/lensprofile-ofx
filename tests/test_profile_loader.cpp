// Smoke test de ProfileLoader, sin ninguna dependencia de OFX ni Metal.
// Se puede compilar y correr en cualquier maquina con un compilador C++ y
// nlohmann/json instalado -- no hace falta Xcode ni Resolve para esto.
//
// Compilar (macOS/Linux, con nlohmann-json instalado):
//   g++ -std=c++14 -I../src -I/usr/include ../src/ProfileLoader.cpp test_profile_loader.cpp -o test_profile_loader
//   ./test_profile_loader ../profiles/canon_k35_50mm.json

#include "ProfileLoader.h"

#include <iostream>
#include <cmath>

namespace {

bool nearlyEqual(double a, double b, double eps = 1e-9)
{
    return std::fabs(a - b) < eps;
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <ruta-al-perfil.json>" << std::endl;
        return 2;
    }

    int failures = 0;

    try {
        LensProfile::Profile profile = LensProfile::LoadProfileFromFile(argv[1]);

        std::cout << "profile_id:   " << profile.profileId << std::endl;
        std::cout << "display_name: " << profile.displayName << std::endl;
        std::cout << "k1_red:       " << profile.chromaticAberration.k1_red << std::endl;
        std::cout << "k1_green:     " << profile.chromaticAberration.k1_green << std::endl;
        std::cout << "k1_blue:      " << profile.chromaticAberration.k1_blue << std::endl;

        if (profile.profileId.empty()) {
            std::cerr << "FALLO: profile_id vino vacio" << std::endl;
            ++failures;
        }
        if (!nearlyEqual(profile.chromaticAberration.k1_red, 0.0009)) {
            std::cerr << "FALLO: k1_red esperado 0.0009, obtuve "
                      << profile.chromaticAberration.k1_red << std::endl;
            ++failures;
        }
        if (!nearlyEqual(profile.chromaticAberration.k1_blue, -0.0007)) {
            std::cerr << "FALLO: k1_blue esperado -0.0007, obtuve "
                      << profile.chromaticAberration.k1_blue << std::endl;
            ++failures;
        }
    } catch (const std::exception& e) {
        std::cerr << "FALLO: excepcion cargando el perfil: " << e.what() << std::endl;
        ++failures;
    }

    // Caso negativo: un archivo que no existe debe lanzar, no crashear.
    try {
        LensProfile::LoadProfileFromFile("/ruta/que/no/existe.json");
        std::cerr << "FALLO: se esperaba una excepcion para un archivo inexistente" << std::endl;
        ++failures;
    } catch (const std::exception&) {
        std::cout << "OK: archivo inexistente lanza excepcion como se espera" << std::endl;
    }

    if (failures == 0) {
        std::cout << "\nTodos los checks pasaron." << std::endl;
        return 0;
    }
    std::cerr << "\n" << failures << " check(s) fallaron." << std::endl;
    return 1;
}
