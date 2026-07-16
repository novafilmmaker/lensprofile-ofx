#include "ChromaticAberrationProcessor.h"
#include "MetalKernel.h"

#include <algorithm>
#include <cmath>

ChromaticAberrationProcessor::ChromaticAberrationProcessor(OFX::ImageEffect& instance)
    : OFX::ImageProcessor(instance)
    , _srcImg(nullptr)
    , _k1Red(0.0)
    , _k1Green(0.0)
    , _k1Blue(0.0)
{
}

void ChromaticAberrationProcessor::setParams(double k1Red, double k1Green, double k1Blue)
{
    _k1Red = k1Red;
    _k1Green = k1Green;
    _k1Blue = k1Blue;
}

// ---------------------------------------------------------------------------
// Camino GPU (Metal)
// ---------------------------------------------------------------------------
void ChromaticAberrationProcessor::processImagesMetal()
{
    const OfxRectI& bounds = _dstImg->getBounds();
    const int width = bounds.x2 - bounds.x1;
    const int height = bounds.y2 - bounds.y1;

    const float* srcData = static_cast<const float*>(_srcImg->getPixelData());
    float* dstData = static_cast<float*>(_dstImg->getPixelData());

    RunChromaticAberrationMetalKernel(_pMetalCmdQ,
                                       width, height,
                                       static_cast<float>(_k1Red),
                                       static_cast<float>(_k1Green),
                                       static_cast<float>(_k1Blue),
                                       srcData, dstData);
}

// ---------------------------------------------------------------------------
// Camino CPU (respaldo)
// ---------------------------------------------------------------------------
float ChromaticAberrationProcessor::SampleChannelBilinear(int channel, double px, double py) const
{
    const OfxRectI& bounds = _srcImg->getBounds();
    const int width = bounds.x2 - bounds.x1;
    const int height = bounds.y2 - bounds.y1;

    // clamp a los bordes de la imagen (evita leer fuera del buffer)
    px = std::min(std::max(px, 0.0), static_cast<double>(width - 1));
    py = std::min(std::max(py, 0.0), static_cast<double>(height - 1));

    int x0 = static_cast<int>(std::floor(px));
    int y0 = static_cast<int>(std::floor(py));
    int x1 = std::min(x0 + 1, width - 1);
    int y1 = std::min(y0 + 1, height - 1);

    double fx = px - x0;
    double fy = py - y0;

    const float* p00 = static_cast<const float*>(_srcImg->getPixelAddress(bounds.x1 + x0, bounds.y1 + y0));
    const float* p10 = static_cast<const float*>(_srcImg->getPixelAddress(bounds.x1 + x1, bounds.y1 + y0));
    const float* p01 = static_cast<const float*>(_srcImg->getPixelAddress(bounds.x1 + x0, bounds.y1 + y1));
    const float* p11 = static_cast<const float*>(_srcImg->getPixelAddress(bounds.x1 + x1, bounds.y1 + y1));

    if (!p00 || !p10 || !p01 || !p11) return 0.0f;

    double top = p00[channel] * (1.0 - fx) + p10[channel] * fx;
    double bottom = p01[channel] * (1.0 - fx) + p11[channel] * fx;
    return static_cast<float>(top * (1.0 - fy) + bottom * fy);
}

void ChromaticAberrationProcessor::multiThreadProcessImages(OfxRectI procWindow)
{
    const OfxRectI& bounds = _dstImg->getBounds();
    const double width = static_cast<double>(bounds.x2 - bounds.x1);
    const double height = static_cast<double>(bounds.y2 - bounds.y1);
    const double centerX = width * 0.5;
    const double centerY = height * 0.5;

    for (int y = procWindow.y1; y < procWindow.y2; ++y) {
        if (_effect.abort()) break;

        float* dstRow = static_cast<float*>(_dstImg->getPixelAddress(procWindow.x1, y));

        for (int x = procWindow.x1; x < procWindow.x2; ++x) {
            // Coordenadas centradas y normalizadas (aprox -1..1 en el eje corto)
            double dx = (x - bounds.x1) - centerX;
            double dy = (y - bounds.y1) - centerY;
            double nx = dx / centerX;
            double ny = dy / centerY;
            double r2 = nx * nx + ny * ny;

            double scaleR = 1.0 + _k1Red * r2;
            double scaleG = 1.0 + _k1Green * r2;
            double scaleB = 1.0 + _k1Blue * r2;

            double srcXr = centerX + dx * scaleR;
            double srcYr = centerY + dy * scaleR;
            double srcXg = centerX + dx * scaleG;
            double srcYg = centerY + dy * scaleG;
            double srcXb = centerX + dx * scaleB;
            double srcYb = centerY + dy * scaleB;

            float* dstPix = dstRow + (x - procWindow.x1) * 4;
            dstPix[0] = SampleChannelBilinear(0, srcXr, srcYr);
            dstPix[1] = SampleChannelBilinear(1, srcXg, srcYg);
            dstPix[2] = SampleChannelBilinear(2, srcXb, srcYb);
            dstPix[3] = SampleChannelBilinear(3, (x - bounds.x1), (y - bounds.y1));  // alpha sin desplazar
        }
    }
}
