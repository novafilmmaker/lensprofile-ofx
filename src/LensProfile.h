#pragma once

#include <string>

// Estructuras de datos de un "perfil óptico".
//
// Esto es intencionalmente un subconjunto del esquema completo definido en
// arquitectura-ofx-plugin-opticas.md (sección 5). En esta Fase 1 solo leemos
// el bloque "chromatic_aberration" del JSON; el resto de bloques (distortion,
// vignette, bokeh, bloom, etc.) se agregan a este struct en fases futuras sin
// romper nada de lo que ya existe.

namespace LensProfile {

// Coeficientes de aberración cromática radial por canal.
// Modelo: offset_radial(canal) = posicion * (1 + k1 * r^2)
// donde r es el radio normalizado desde el centro de la imagen.
struct ChromaticAberration {
    double k1_red = 0.0;
    double k1_green = 0.0;
    double k1_blue = 0.0;
};

struct Profile {
    std::string profileId;
    std::string displayName;
    ChromaticAberration chromaticAberration;
};

}  // namespace LensProfile
