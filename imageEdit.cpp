#include <stdio.h>
#include <cmath>
#include <arrayfire.h>
#include <af/utils.h>

using namespace af;

/**
 * contrast value should be in the rnage [-1,1]
 * */
array changeContrast(const array &in, const float contrast)
{
    float scale     = tan((contrast+1)*Pi/4);
    return (((in/255.0f - 0.5f) * scale + 0.5f) * 255.0f);
}

/**
 * brightness value should be in the rnage [0,1]
 * */
array changeBrightness(const array &in, const float brightness, const float channelMax=255.0f)
{
    float factor = brightness*channelMax;
    return (in + factor);
}

array clamp(const array &in, float min=0.0f, float max=255.0f)
{
    return ((in<min)*0.0f + (in>max)*255.0f + (in>=min && in <=max)*in);
}

/**
 * radius effects the level of details that will effected during sharpening process
 * amount value should be in the range [0,1] or [1,]
 * note: value of 1.0 for amount results unsharp masking
 *       values > 1.0 results in highboost filter effect
 * */
array usm(const array &in, float radius, float amount)
{
    int gKernelLen  = 2*radius + 1;
    array blurKernel= gaussiankernel(gKernelLen,gKernelLen);
    array blur  = convolve(in,blurKernel);
    return (in + amount*(in - blur));
}

array digZoom(const array &in, int x, int y, int width, int height)
{
    array cropped = in(seq(height)+x,seq(width)+y,span);
    return resize((unsigned)in.dims(0),(unsigned)in.dims(1),cropped);
}

array alphaBlend(const array &a, const array &b, const array &mask)
{
    array tiledMask;
    if (mask.dims(2)!=a.dims(2))
        tiledMask = tile(mask,1,1,a.dims(2));
    return a*tiledMask+(1.0f-tiledMask)*b;
}

int main(int argc, char **argv)
{
    try {
        int device = argc > 1 ? atoi(argv[1]) : 0;
        af::deviceset(device);
        af::info();

        array a = loadimage("../common/images/usm_man.jpg",true);
        array fight = loadimage("/home/pradeep/gitroot/trailsground/blog_posts/imageediting/fight.jpg",true);
        array nature = loadimage("/home/pradeep/gitroot/trailsground/blog_posts/imageediting/nature.jpg",true);

        array intensity = colorspace(fight,af_gray,af_rgb);
        array mask  = clamp(intensity,10.0f,255.0f)>0.0f;
        array blend = alphaBlend(fight,nature,mask);

        array highcon = changeContrast(a,0.3);
        array highbright = changeBrightness(a,0.2);
        array translated = translate(a,100,100,200,126);
        array sharp = usm(a,3,1.2);
        array zoom = digZoom(a,28,10,192,192);

        fig("sub",3,3,1); image(a/255); fig("title","Input");
        fig("sub",3,3,2); image(highcon/255); fig("title","High Contrast");
        fig("sub",3,3,3); image(highbright/255); fig("title","High Brightness");
        fig("sub",3,3,4); image(translated/255); fig("title","Translation");
        fig("sub",3,3,5); image(sharp/255); fig("title","Unsharp Masking");
        fig("sub",3,3,6); image(zoom/255); fig("title","Digital Zoom");
        fig("sub",3,3,7); image(nature/255); fig("title","Background for blend");
        fig("sub",3,3,8); image(fight/255); fig("title","Foreground for blend");
        fig("sub",3,3,9); image(blend/255); fig("title","Alpha blend");
        getchar();

    } catch (af::exception& e) {
        fprintf(stderr, "%s\n", e.what());
        throw;
    }

    #ifdef WIN32 // pause in Windows
    if (!(argc == 2 && argv[1][0] == '-')) {
        printf("hit [enter]...");
        fflush(stdout);
        getchar();
    }
    #endif
    return 0;
}
