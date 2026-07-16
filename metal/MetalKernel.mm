// MetalKernel.mm
//
// Puente Objective-C++ que compila y ejecuta el kernel de aberracion
// cromatica en la GPU via Metal. Este archivo se compila SIN ARC
// (-fno-objc-arc, ver CMakeLists.txt) para poder hacer casts directos del
// puntero de cola de comandos que nos entrega Resolve.
//
// Referencia de patron: esta estructura (crear MTLLibrary desde un string de
// fuente en runtime, cachear el pipeline, castear los buffers de pixeles de
// OFX directamente a id<MTLBuffer>) sigue el mismo enfoque que usan los
// plugins OFX/Metal de Resolve publicados (p.ej. reframe360XL, plugins de
// Blackmagic). Ver arquitectura-ofx-plugin-opticas.md seccion 4.1.

#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

#include "MetalKernel.h"

namespace {

// Debe coincidir EXACTAMENTE con el struct CAParams del lado Metal (mismo
// orden de campos, mismos tipos) -- no hay un header compartido porque el
// codigo fuente Metal vive como string embebido, asi que la sincronia es
// manual. Si agregas un campo, agregalo en ambos lados.
struct CAParams {
    int32_t width;
    int32_t height;
    float k1Red;
    float k1Green;
    float k1Blue;
};

const char* const kKernelSource = R"MSL(
#include <metal_stdlib>
using namespace metal;

struct CAParams {
    int width;
    int height;
    float k1Red;
    float k1Green;
    float k1Blue;
};

inline float4 sampleBilinear(const device float* buf, int width, int height, float2 pos)
{
    pos.x = clamp(pos.x, 0.0f, float(width - 1));
    pos.y = clamp(pos.y, 0.0f, float(height - 1));

    int x0 = int(floor(pos.x));
    int y0 = int(floor(pos.y));
    int x1 = min(x0 + 1, width - 1);
    int y1 = min(y0 + 1, height - 1);

    float fx = pos.x - float(x0);
    float fy = pos.y - float(y0);

    int i00 = (y0 * width + x0) * 4;
    int i10 = (y0 * width + x1) * 4;
    int i01 = (y1 * width + x0) * 4;
    int i11 = (y1 * width + x1) * 4;

    float4 c00 = float4(buf[i00], buf[i00+1], buf[i00+2], buf[i00+3]);
    float4 c10 = float4(buf[i10], buf[i10+1], buf[i10+2], buf[i10+3]);
    float4 c01 = float4(buf[i01], buf[i01+1], buf[i01+2], buf[i01+3]);
    float4 c11 = float4(buf[i11], buf[i11+1], buf[i11+2], buf[i11+3]);

    float4 top = mix(c00, c10, fx);
    float4 bottom = mix(c01, c11, fx);
    return mix(top, bottom, fy);
}

kernel void ChromaticAberrationKernel(const device float* inBuffer [[buffer(0)]],
                                       device float* outBuffer [[buffer(1)]],
                                       constant CAParams& params [[buffer(2)]],
                                       uint2 gid [[thread_position_in_grid]])
{
    if (gid.x >= uint(params.width) || gid.y >= uint(params.height)) return;

    float centerX = float(params.width) * 0.5;
    float centerY = float(params.height) * 0.5;

    float dx = float(gid.x) - centerX;
    float dy = float(gid.y) - centerY;
    float nx = dx / centerX;
    float ny = dy / centerY;
    float r2 = nx * nx + ny * ny;

    float scaleR = 1.0 + params.k1Red * r2;
    float scaleG = 1.0 + params.k1Green * r2;
    float scaleB = 1.0 + params.k1Blue * r2;

    float2 posR = float2(centerX + dx * scaleR, centerY + dy * scaleR);
    float2 posG = float2(centerX + dx * scaleG, centerY + dy * scaleG);
    float2 posB = float2(centerX + dx * scaleB, centerY + dy * scaleB);
    float2 posA = float2(float(gid.x), float(gid.y));

    float4 sampR = sampleBilinear(inBuffer, params.width, params.height, posR);
    float4 sampG = sampleBilinear(inBuffer, params.width, params.height, posG);
    float4 sampB = sampleBilinear(inBuffer, params.width, params.height, posB);
    float4 sampA = sampleBilinear(inBuffer, params.width, params.height, posA);

    int outIdx = (int(gid.y) * params.width + int(gid.x)) * 4;
    outBuffer[outIdx + 0] = sampR.x;
    outBuffer[outIdx + 1] = sampG.y;
    outBuffer[outIdx + 2] = sampB.z;
    outBuffer[outIdx + 3] = sampA.w;
}
)MSL";

// Compila (una sola vez, cacheado) el pipeline de compute para el kernel
// de arriba. Inicializacion estatica local en C++11+ es thread-safe.
id<MTLComputePipelineState> GetCachedPipelineState(id<MTLDevice> device)
{
    static id<MTLComputePipelineState> pipelineState = nil;
    if (pipelineState != nil) {
        return pipelineState;
    }

    NSError* error = nil;
    MTLCompileOptions* options = [MTLCompileOptions new];
    options.fastMathEnabled = YES;

    id<MTLLibrary> library = [device newLibraryWithSource:@(kKernelSource)
                                                    options:options
                                                      error:&error];
    [options release];

    if (library == nil) {
        NSLog(@"LensProfileOFX: fallo compilando el kernel Metal: %@", error);
        return nil;
    }

    id<MTLFunction> function = [library newFunctionWithName:@"ChromaticAberrationKernel"];
    if (function == nil) {
        NSLog(@"LensProfileOFX: no se encontro la funcion ChromaticAberrationKernel");
        [library release];
        return nil;
    }

    pipelineState = [device newComputePipelineStateWithFunction:function error:&error];
    [function release];
    [library release];

    if (pipelineState == nil) {
        NSLog(@"LensProfileOFX: fallo creando el pipeline de compute: %@", error);
    }

    return pipelineState;
}

}  // namespace

void RunChromaticAberrationMetalKernel(void* commandQueue,
                                        int width,
                                        int height,
                                        float k1Red,
                                        float k1Green,
                                        float k1Blue,
                                        const float* input,
                                        float* output)
{
    if (commandQueue == nullptr || input == nullptr || output == nullptr) {
        return;
    }

    id<MTLCommandQueue> queue = (id<MTLCommandQueue>)commandQueue;
    id<MTLDevice> device = queue.device;

    id<MTLComputePipelineState> pipelineState = GetCachedPipelineState(device);
    if (pipelineState == nil) {
        return;
    }

    // Resolve nos entrega los buffers de pixeles de OFX::Image ya como
    // memoria respaldada por Metal (el mismo patron que usan otros plugins
    // OFX/Metal para Resolve) -- por eso podemos reinterpretarlos
    // directamente como id<MTLBuffer> en lugar de copiar datos.
    id<MTLBuffer> inputBuffer = (id<MTLBuffer>)(const_cast<float*>(input));
    id<MTLBuffer> outputBuffer = (id<MTLBuffer>)(output);

    CAParams params;
    params.width = width;
    params.height = height;
    params.k1Red = k1Red;
    params.k1Green = k1Green;
    params.k1Blue = k1Blue;

    id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
    commandBuffer.label = @"ChromaticAberrationKernel";

    id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
    [encoder setComputePipelineState:pipelineState];
    [encoder setBuffer:inputBuffer offset:0 atIndex:0];
    [encoder setBuffer:outputBuffer offset:0 atIndex:1];
    [encoder setBytes:&params length:sizeof(CAParams) atIndex:2];

    NSUInteger execWidth = pipelineState.threadExecutionWidth;
    NSUInteger execHeight = pipelineState.maxTotalThreadsPerThreadgroup / execWidth;
    if (execHeight == 0) execHeight = 1;

    MTLSize threadsPerThreadgroup = MTLSizeMake(execWidth, execHeight, 1);
    MTLSize threadgroups = MTLSizeMake((width + execWidth - 1) / execWidth,
                                        (height + execHeight - 1) / execHeight,
                                        1);

    [encoder dispatchThreadgroups:threadgroups threadsPerThreadgroup:threadsPerThreadgroup];
    [encoder endEncoding];

    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
}
