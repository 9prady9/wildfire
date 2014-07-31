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

/**
 * x,y - starting position of zoom
 * width, height - dimensions of the rectangle to where we have to zoom in
 * */
array digZoom(const array &in, int x, int y, int width, int height)
{
    array cropped = in(seq(height)+x,seq(width)+y,span);
    return resize((unsigned)in.dims(0),(unsigned)in.dims(1),cropped);
}

/**
 * a - foregound image
 * b - background image
 * mask - mask map
 * */
array alphaBlend(const array &a, const array &b, const array &mask)
{
    array tiledMask;
    if (mask.dims(2)!=a.dims(2))
        tiledMask = tile(mask,1,1,a.dims(2));
    return a*tiledMask+(1.0f-tiledMask)*b;
}

/**
 * randomization - controls % of total number of pixels in the image
 * that will be effected by random noise
 * repeat - # of times the process is carried out on the previous steps output
 */
array hurl(const array &in, int randomization, int repeat)
{
    int w = in.dims(0);
    int h = in.dims(1);
    float f = randomization/100.0f;
    int dim = (int)(f*w*h);
    array ret_val = in.copy();
    array temp = moddims(ret_val,w*h,3);
    for(int i=0;i<repeat;++i) {
        array idxs = (w*h)  * randu(dim);
        array rndR = 255.0f * randu(dim);
        array rndG = 255.0f * randu(dim);
        array rndB = 255.0f * randu(dim);
        temp(idxs,0) = rndR;
        temp(idxs,1) = rndG;
        temp(idxs,2) = rndB;
    }
    ret_val = moddims(temp,in.dims());
    return ret_val;
}

/**
 * Retrieve a new image of same dimensions of the original image
 * where each original image's pixel is replaced by randomly picked
 * neighbor in the provided local neighborhood window
 */
array getRandomNeighbor(const array &in, int windW, int windH)
{
    array rnd = 2.0f*randu(in.dims(0), in.dims(1))-1.0f;
    array sx = seq(in.dims(0));
    array sy = seq(in.dims(1));
    array vx = tile(sx, 1, in.dims(1)) + floor(rnd*windW);
    array vy = tile(sy.T(), in.dims(0), 1) + floor(rnd*windH);
    array vxx = clamp(vx,0,in.dims(0));
    array vyy = clamp(vy,0,in.dims(1));
    array in2 = moddims(in, vx.elements(), 3);
    return moddims(in2(vyy*in.dims(0)+vxx, span),in.dims());
}

/**
 * randomly pick neighbor from given window size and replace the
 * current pixel with the randomly chosen color.
 * No new colors are introduced, unlike hurl.
 */
array spread(const array &in, int window_width, int window_height)
{
    return getRandomNeighbor(in,window_width,window_height);
}

/**
 * randomization - controls % of total number of pixels in the image
 * that will be effected by random noise
 * repeat - # of times the process is carried out on the previous steps output
 */
array pick(const array &in, int randomization, int repeat)
{
    int w = in.dims(0);
    int h = in.dims(1);
    float f = randomization/100.0f;
    int dim = (int)(f*w*h);
    array ret_val = in.copy();
    for(int i=0;i<repeat;++i) {
        array idxs  = (w*h)  * randu(dim);
        array rnd   = getRandomNeighbor(ret_val,1,1);
        array temp_src = moddims(rnd,w*h,3);
        array temp_dst = moddims(ret_val,w*h,3);
        temp_dst(idxs,span) = temp_src(idxs,span);
        ret_val = moddims(temp_dst,in.dims());
    }
    return ret_val;
}

void prewitt(array &mag, array &dir, const array &in)
{
    float h1[] = { 1, 1, 1};
    float h2[] = {-1, 0, 1};

    // Find the gradients
    array Gy = convolve(3, h2, 3, h1, in)/6;
    array Gx = convolve(3, h1, 3, h2, in)/6;

    // Find magnitude and direction
    mag = hypot(Gx, Gy);
    dir = atan2(Gy, Gx);
}

void sobel(array &mag, array &dir, const array &in)
{
    float h1[] = { 1, 2,  1};
    float h2[] = { 1, 0, -1};

    // Find the gradients
    array Gy = convolve(3, h2, 3, h1, in)/8;
    array Gx = convolve(3, h1, 3, h2, in)/8;

    // Find magnitude and direction
    mag = hypot(Gx, Gy);
    dir = atan2(Gy, Gx);
}

void normalizeImage(array &in)
{
    float min = af::min<float>(in);
    float max = af::max<float>(in);
    in = 255.0f*((in-min)/(max-min));
}

array dog(const array &in, int window_radius1, int window_radius2)
{
    array ret_val;
    int w1 = 2*window_radius1+1;
    int w2 = 2*window_radius2+1;
    array g1 = gaussiankernel(w1,w1);
    array g2 = gaussiankernel(w2,w2);
    ret_val = (convolve(in,g1)-convolve(in,g2));
    normalizeImage(ret_val);
    return ret_val;
}

array medianfilter(const array &in, int window_width, int window_height)
{
    array ret_val(in.dims());
    ret_val(span,span,0) = medfilt(in(span,span,0),window_width,window_height);
    ret_val(span,span,1) = medfilt(in(span,span,1),window_width,window_height);
    ret_val(span,span,2) = medfilt(in(span,span,2),window_width,window_height);
    return ret_val;
}

array gaussianblur(const array &in, int window_width, int window_height, int sigma)
{
    array g = gaussiankernel(window_width,window_height,sigma,sigma);
    return convolve(in,g);
}

/**
 * dimensions of the mask control the thickness of the boundary that
 * will be extracted by the following function
 */
array boundary(const array &in, const array &mask)
{
    array ret_val = in - erode(in,mask);
    normalizeImage(ret_val);
    return ret_val;
}

int main(int argc, char **argv)
{
    try {
        int device = argc > 1 ? atoi(argv[1]) : 0;
        af::deviceset(device);
        af::info();

        array a = loadimage("/home/pradeep/gitroot/trailsground/blog_posts/imageediting/lena512x512.jpg",true);
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

        array prew_mag, prew_dir;
        array sob_mag, sob_dir;
        array lena1ch = colorspace(a,af_gray,af_rgb);
        prewitt(prew_mag,prew_dir,lena1ch);
        sobel(sob_mag,sob_dir,lena1ch);
        array sprd  = spread(a,3,3);
        array hrl   = hurl(a,10,1);
        array pckng = pick(a,40,2);
        array difog = dog(a,1,2);
        array bil   = bilateral(hrl,3.0f,40.0f);
        array mf    = medianfilter(hrl,5,5);
        array gb    = gaussianblur(hrl,3,3,0.8);

        fig("sub",3,3,1); image(hrl/255); fig("title","Hurl noise");
        fig("sub",3,3,2); image(gb/255); fig("title","Gaussian blur");
        fig("sub",3,3,3); image(bil/255); fig("title","Bilateral filter on hurl noise");
        fig("sub",3,3,4); image(mf/255); fig("title","Median filter on hurl noise");
        fig("sub",3,3,5); image(prew_mag/255); fig("title","Prewitt edge filter");
        fig("sub",3,3,6); image(sob_mag/255); fig("title","Sobel edge filter");
        fig("sub",3,3,7); image(sprd/255); fig("title","Spread filter");
        fig("sub",3,3,8); image(pckng/255); fig("title","Pick filter");
        fig("sub",3,3,9); image(difog/255); fig("title","Difference of gaussians(3x3 and 5x5)");
        getchar();

        array morph_mask= constant(1,3,3);
        array bdry      = boundary(a,morph_mask);
        fig("sub",1,1,1); image(bdry/255); fig("title","Boundary extraction using morph ops");
        getchar();

        normalizeImage(prew_mag);
        normalizeImage(sob_mag);
        saveimage("/home/pradeep/gitroot/trailsground/blog_posts/imageediting/hurl.jpg",hrl);
        saveimage("/home/pradeep/gitroot/trailsground/blog_posts/imageediting/gblur.jpg",gb);
        saveimage("/home/pradeep/gitroot/trailsground/blog_posts/imageediting/bilateral.jpg",bil);
        saveimage("/home/pradeep/gitroot/trailsground/blog_posts/imageediting/medfilt.jpg",mf);
        saveimage("/home/pradeep/gitroot/trailsground/blog_posts/imageediting/prewitt.jpg",prew_mag);
        saveimage("/home/pradeep/gitroot/trailsground/blog_posts/imageediting/sobel.jpg",sob_mag);
        saveimage("/home/pradeep/gitroot/trailsground/blog_posts/imageediting/spread.jpg",sprd);
        saveimage("/home/pradeep/gitroot/trailsground/blog_posts/imageediting/pick.jpg",pckng);
        saveimage("/home/pradeep/gitroot/trailsground/blog_posts/imageediting/dog.jpg",difog);
        saveimage("/home/pradeep/gitroot/trailsground/blog_posts/imageediting/bdry.jpg",bdry);
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
