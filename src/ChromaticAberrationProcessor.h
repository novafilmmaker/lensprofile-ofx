#pragma once

#include "ofxsImageEffect.h"
#include "ofxsProcessing.h"

// Procesador de aberración cromática: implementa tanto el camino CPU
// (multiThreadProcessImages, por si Resolve no puede darnos Metal) como el
// camino GPU real (processImagesMetal), que es el que se usa en la práctica
// dentro de Resolve en macOS.
class ChromaticAberrationProcessor : public OFX::ImageProcessor {
public:
    explicit ChromaticAberrationProcessor(OFX::ImageEffect& instance);

    void setSrcImg(OFX::Image* src) { _srcImg = src; }

    // k1Red/k1Green/k1Blue ya vienen escalados por el slider de "carácter".
    void setParams(double k1Red, double k1Green, double k1Blue);

    // Camino GPU (Metal) -- el que realmente se ejecuta en Resolve en macOS
    // cuando el host activa render Metal para este efecto.
    virtual void processImagesMetal() override;

    // Camino CPU multihilo -- respaldo si Metal no está disponible.
    virtual void multiThreadProcessImages(OfxRectI procWindow) override;

private:
    OFX::Image* _srcImg;
    double _k1Red;
    double _k1Green;
    double _k1Blue;

    // Muestreo bilineal de un canal (0=R,1=G,2=B,3=A) del buffer fuente en
    // una posición de pixel fraccionaria, con clamp a los bordes. Usado solo
    // por el camino CPU -- el kernel Metal hace lo mismo en GPU.
    float SampleChannelBilinear(int channel, double px, double py) const;
};
