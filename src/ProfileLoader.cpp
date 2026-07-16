#include "ProfileLoader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace LensProfile {

Profile LoadProfileFromFile(const std::string& jsonPath)
{
    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo abrir el perfil: " + jsonPath);
    }

    nlohmann::json data;
    try {
        file >> data;
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("JSON invalido en " + jsonPath + ": " + e.what());
    }

    Profile profile;

    // Campos generales. Si faltan, usamos valores por defecto razonables
    // en vez de fallar -- un perfil incompleto no debe tumbar Resolve.
    profile.profileId = data.value("profile_id", std::string("unknown"));
    profile.displayName = data.value("display_name", std::string("Perfil sin nombre"));

    if (!data.contains("chromatic_aberration")) {
        throw std::runtime_error("El perfil " + jsonPath +
                                  " no tiene el bloque 'chromatic_aberration'");
    }

    const auto& ca = data.at("chromatic_aberration");

    auto readChannelK1 = [&ca](const char* channelName) -> double {
        if (!ca.contains(channelName)) return 0.0;
        const auto& channel = ca.at(channelName);
        return channel.value("k1", 0.0);
    };

    profile.chromaticAberration.k1_red = readChannelK1("red");
    profile.chromaticAberration.k1_green = readChannelK1("green");
    profile.chromaticAberration.k1_blue = readChannelK1("blue");

    return profile;
}

}  // namespace LensProfile
