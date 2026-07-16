#pragma once

#include "ofxsImageEffect.h"
#include "LensProfile.h"
#include "ChromaticAberrationProcessor.h"

// Nombre interno del único parámetro de esta Fase 1: el slider de
// "carácter global" (0-150%) que escala los coeficientes del perfil.
static const char* const kParamAmount = "amount";

class ChromaticAberrationPlugin : public OFX::ImageEffect {
public:
    explicit ChromaticAberrationPlugin(OfxImageEffectHandle handle);

    virtual void render(const OFX::RenderArguments& args) override;

private:
    void setupAndProcess(ChromaticAberrationProcessor& processor, const OFX::RenderArguments& args);

    OFX::Clip* dstClip_;
    OFX::Clip* srcClip_;
    OFX::DoubleParam* amount_;

    LensProfile::Profile profile_;
};

// Factory: OFX descubre plugins a través de esta clase (macro definida en
// ofxsImageEffect.h -- ver mDeclarePluginFactory).
mDeclarePluginFactory(ChromaticAberrationPluginFactory, {}, {});
