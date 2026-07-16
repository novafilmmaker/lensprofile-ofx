#pragma once

// Puente C++ <-> Metal. Esta función está implementada en MetalKernel.mm
// (Objective-C++) y la llama ChromaticAberrationProcessor::processImagesMetal().
//
// commandQueue es en realidad un id<MTLCommandQueue> que Resolve nos entrega
// vía OFX::RenderArguments::pMetalCmdQ -- lo pasamos como void* para que este
// header lo puedan incluir archivos .cpp puros (sin saber nada de Objective-C).
//
// input/output son punteros a buffers RGBA float32 interleaved del mismo
// tamaño (width * height * 4 floats), tal como los entrega OFX::Image::getPixelData().
void RunChromaticAberrationMetalKernel(void* commandQueue,
                                        int width,
                                        int height,
                                        float k1Red,
                                        float k1Green,
                                        float k1Blue,
                                        const float* input,
                                        float* output);
