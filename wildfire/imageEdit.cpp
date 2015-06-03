#include <stdio.h>
#include <cmath>
#include "imageEdit.hpp"

using namespace af;

array changeContrast(const array &in, const float contrast)
{
    double scale     = std::tan((contrast+1)*Pi/4);
    return (((in/255.0f - 0.5f) * scale + 0.5f) * 255.0f);
}

array changeBrightness(const array &in, const float brightness, const float channelMax)
{
    float factor = brightness*channelMax;
    return (in + factor);
}

array usm(const array &in, int radius, float amount)
{
    int gKernelLen  = 2*radius + 1;
    array blurKernel= gaussianKernel(gKernelLen,gKernelLen);
    array blur  = convolve(in,blurKernel);
    return (in + amount*(in - blur));
}

array digZoom(const array &in, int x, int y, int width, int height)
{
    array cropped = in(seq(x, width-1),seq(y,height-1),span);
    return resize(cropped, in.dims(0),in.dims(1));
}

array alphaBlend(const array &a, const array &b, const array &mask)
{
    array tiledMask;
    if (mask.dims(2)!=a.dims(2))
        tiledMask = tile(mask, 1, 1, (unsigned)a.dims(2))/255.0f;
    return a*tiledMask+(1.0f-tiledMask)*b;
}
