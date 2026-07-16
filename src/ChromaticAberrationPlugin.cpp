#include "ChromaticAberrationPlugin.h"
#include "ProfileLoader.h"
#include "BundleResourcePath.h"

#include <memory>
#include <stdexcept>

#define kPluginName "Lens Profile - Aberracion Cromatica"
#define kPluginGrouping "Bitacora Films/Lens Profile"
#define kPluginDescription \
    "Fase 1 del motor Lens Profile: emula la aberracion cromatica radial " \
    "de una optica de cine a partir de un perfil JSON externo."
#define kPluginIdentifier "cl.bitacorafilms.LensProfileOFX.ChromaticAberration"
#define kPluginVersionMajor 0
#define kPluginVersionMinor 1

namespace {
const char* const kProfileFileName = "canon_k35_50mm.json";
}

// ---------------------------------------------------------------------------
// ChromaticAberrationPlugin
// ---------------------------------------------------------------------------

ChromaticAberrationPlugin::ChromaticAberrationPlugin(OfxImageEffectHandle handle)
    : ImageEffect(handle)
    , dstClip_(fetchClip(kOfxImageEffectOutputClipName))
    , srcClip_(fetchClip(kOfxImageEffectSimpleSourceClipName))
    , amount_(fetchDoubleParam(kParamAmount))
{
    const std::string profilesDir = LensProfile::GetProfilesDirectory();
    const std::string profilePath = profilesDir + kProfileFileName;

    try {
        profile_ = LensProfile::LoadProfileFromFile(profilePath);
    } catch (const std::exception&) {
        // No pudimos leer el perfil (bundle instalado a medias, permisos,
        // etc.). Preferimos degradar a un valor de respaldo hardcodeado
        // antes que tumbar Resolve -- ver nota en ProfileLoader sobre esto.
        profile_.profileId = "fallback";
        profile_.displayName = "Fallback (perfil no encontrado)";
        profile_.chromaticAberration = LensProfile::ChromaticAberration{0.0009, 0.0, -0.0007};
    }
}

void ChromaticAberrationPlugin::setupAndProcess(ChromaticAberrationProcessor& processor,
                                                 const OFX::RenderArguments& args)
{
    std::unique_ptr<OFX::Image> dst(dstClip_->fetchImage(args.time));
    std::unique_ptr<OFX::Image> src(srcClip_->fetchImage(args.time));

    if (!dst.get() || !src.get()) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    if (dst->getPixelDepth() != OFX::eBitDepthFloat ||
        dst->getPixelComponents() != OFX::ePixelComponentRGBA) {
        // Esta Fase 1 solo soporta float RGBA (procesamiento lineal / alta
        // profundidad de bits, como define la arquitectura). 8/16 bit se
        // agregan mas adelante si hace falta.
        OFX::throwSuiteStatusException(kOfxStatErrImageFormat);
        return;
    }

    const double amount = amount_->getValueAtTime(args.time) / 100.0;

    processor.setDstImg(dst.get());
    processor.setSrcImg(src.get());
    processor.setRenderWindow(args.renderWindow);
    processor.setParams(profile_.chromaticAberration.k1_red * amount,
                         profile_.chromaticAberration.k1_green * amount,
                         profile_.chromaticAberration.k1_blue * amount);
    processor.setGPURenderArgs(args);

    processor.process();
}

void ChromaticAberrationPlugin::render(const OFX::RenderArguments& args)
{
    if (dstClip_->getPixelDepth() != OFX::eBitDepthFloat ||
        dstClip_->getPixelComponents() != OFX::ePixelComponentRGBA) {
        OFX::throwSuiteStatusException(kOfxStatErrImageFormat);
        return;
    }

    ChromaticAberrationProcessor processor(*this);
    setupAndProcess(processor, args);
}

// ---------------------------------------------------------------------------
// ChromaticAberrationPluginFactory
// ---------------------------------------------------------------------------

void ChromaticAberrationPluginFactory::describe(OFX::ImageEffectDescriptor& desc)
{
    desc.setLabels(kPluginName, kPluginName, kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription(kPluginDescription);

    desc.addSupportedContext(OFX::eContextFilter);
    desc.addSupportedContext(OFX::eContextGeneral);

    // Solo float: procesamos en lineal / alta profundidad de bits.
    desc.addSupportedBitDepth(OFX::eBitDepthFloat);

    desc.setSingleInstance(false);
    desc.setHostFrameThreading(false);
    desc.setSupportsMultiResolution(false);
    desc.setSupportsTiles(false);  // el efecto lee desde el centro de la imagen completa
    desc.setTemporalClipAccess(false);
    desc.setRenderTwiceAlways(false);
    desc.setSupportsMultipleClipPARs(false);

    // La bandera clave para esta Fase 1: le decimos a Resolve que sabemos
    // procesar con Metal.
    desc.setSupportsMetalRender(true);
}

void ChromaticAberrationPluginFactory::describeInContext(OFX::ImageEffectDescriptor& desc,
                                                           OFX::ContextEnum /*context*/)
{
    OFX::ClipDescriptor* srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(false);

    OFX::ClipDescriptor* dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    dstClip->setSupportsTiles(false);

    OFX::PageParamDescriptor* page = desc.definePageParam("Controls");

    OFX::DoubleParamDescriptor* amount = desc.defineDoubleParam(kParamAmount);
    amount->setLabels("Caracter", "Caracter", "Caracter global de la optica");
    amount->setScriptName(kParamAmount);
    amount->setHint("Escala global de la aberracion cromatica del perfil (100% = valor del perfil tal cual)");
    amount->setDefault(100.0);
    amount->setRange(0.0, 150.0);
    amount->setDisplayRange(0.0, 150.0);
    amount->setIncrement(1.0);
    page->addChild(*amount);
}

OFX::ImageEffect* ChromaticAberrationPluginFactory::createInstance(OfxImageEffectHandle handle,
                                                                     OFX::ContextEnum /*context*/)
{
    return new ChromaticAberrationPlugin(handle);
}

// ---------------------------------------------------------------------------
// Punto de entrada que el SDK de OFX (Support Library) usa para registrar
// nuestro plugin. Esto es lo que finalmente conecta con los simbolos C
// OfxGetNumberOfPlugins / OfxGetPlugin exportados via osxSymbols.
// ---------------------------------------------------------------------------
namespace OFX {
namespace Plugin {

void getPluginIDs(OFX::PluginFactoryArray& ids)
{
    static ChromaticAberrationPluginFactory factory(kPluginIdentifier,
                                                      kPluginVersionMajor,
                                                      kPluginVersionMinor);
    ids.push_back(&factory);
}

}  // namespace Plugin
}  // namespace OFX
