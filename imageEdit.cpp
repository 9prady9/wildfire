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

array threshold(const array &in, float thresholdValue)
{
    int channels = in.dims(2);
    array ret_val= in.copy();
    if (channels>1)
        ret_val = colorspace(in,af_gray,af_rgb);
    ret_val = (ret_val<thresholdValue)*0.0f + 255.0f*(ret_val>thresholdValue);
    return ret_val;
}

/**
 * Note:
 * suffix B indicates subset of all graylevels before current gray level
 * suffix F indicates subset of all graylevels after current gray level
 */
array otsu(const array& in)
{
    array gray;
    int channels = in.dims(2);
    if (channels>1)
        gray  = colorspace(in,af_gray,af_rgb);
    else
        gray  = in;
    unsigned total = gray.elements();
    array hist  = histogram(gray,256,0.0f,255.0f);
    array cond  = hist>0.0f;    // use this condition to limit computation
    array wts   = array(seq(256));

    array wtB   = accum(hist);
    array wtF   = total - wtB;
    array sumB  = accum(wts*hist);
    array meanB = sumB/wtB;
    array meanF = (sumB(255)-sumB) / wtF;
    array mDiff = meanB-meanF;

    array interClsVar= wtB * wtF * mDiff * mDiff;
    float max        = af::max<float>(interClsVar);
    float threshold2 = where(interClsVar==max).scalar<float>();
    array threshIdx  = where(interClsVar>=max);
    float threshold1 = threshIdx.elements()>0 ? threshIdx.scalar<float>() : 0.0f;

    return threshold(gray,(threshold1+threshold2)/2.0f);
}

inline int getWeight(int start, int end, int* hist)
{
    int sum = 0;
    for(int k=start;k<end;++k)
        sum += hist[k];
    return sum;
}

typedef enum {
    MEAN=0,
    MEDIAN,
    MINMAX_AVG
} LocalThresholdType;

array adaptiveThreshold(const array &in, LocalThresholdType kind, int window_size, int constnt)
{
    int wr = window_size;
    array ret_val = colorspace(in,af_gray,af_rgb);
    if (kind==MEAN) {
        array wind = constant(1,wr,wr)/(wr*wr);
        array mean = convolve(ret_val,wind);
        array diff = mean - ret_val;
        ret_val    = (diff<constnt)*0.f + 255.f*(diff>constnt);
    } else if (kind==MEDIAN) {
        array medf = medfilt(ret_val,wr,wr);
        array diff = medf - ret_val;
        ret_val    = (diff<constnt)*0.f + 255.f*(diff>constnt);
    } else if (kind==MINMAX_AVG) {
        array minf = minfilt(ret_val,wr,wr);
        array maxf = maxfilt(ret_val,wr,wr);
        array mean = (minf+maxf)/2.0f;
        array diff = mean - ret_val;
        ret_val    = (diff<constnt)*0.f + 255.f*(diff>constnt);
    }
    ret_val = 255.f - ret_val;
    return ret_val;
}

array iterativeThreshold(const array &in)
{
    array ret_val = colorspace(in,af_gray,af_rgb);
    float T = mean<float>(ret_val);
    bool isContinue = true;
    while(isContinue) {
        array region1 = (ret_val > T)*ret_val;
        array region2 = (ret_val <= T)*ret_val;
        float r1_avg  = mean<float>(region1);
        float r2_avg  = mean<float>(region2);
        float tempT = (r1_avg+r2_avg)/2.0f;
        if (abs(tempT-T)<0.01f) {
            break;
        }
        T = tempT;
    }
    return threshold(ret_val,T);
}

/**
 * azimuth range is [0-360]
 * elevation range is [0-180]
 * depth range is [1-100]
 * Note: this function has been tailored after
 * the emboss implementation in GIMP editor
 **/
array emboss(const array &input, float azimuth, float elevation, float depth)
{
    if (depth<1 || depth>100) {
        printf("Depth should be in the range of 1-100");
        return input;
    }
    static float x[3] = {-1,0,1};
    static array hg(3,x);
    static array vg = hg.T();

    array in = input;
    if (in.dims(2)>1)
        in = colorspace(input,af_gray,af_rgb);
    else
        in = input;

    // convert angles to radians
    float phi   = elevation*af::Pi/180.0f;
    float theta = azimuth*af::Pi/180.0f;

    // compute light pos in cartesian coordinates
    // and scale with maximum intensity
    // phi will effect the amount of we intend to put
    // on a pixel
    float pos[3];
    pos[0] = 255.99f * cos(phi)*cos(theta);
    pos[1] = 255.99f * cos(phi)*sin(theta);
    pos[2] = 255.99f * sin(phi);

    // compute gradient vector
    array gx = filter(in,vg);
    array gy = filter(in,hg);

    float pxlz  = (6*255.0f)/depth;
    array zdepth= constant(pxlz,gx.dims());
    array vdot  = gx*pos[0] + gy*pos[1] + pxlz*pos[2];
    array outwd = vdot < 0.0f;
    array norm  = vdot/sqrt(gx*gx+gy*gy+zdepth*zdepth);

    array color = outwd * 0.0f + (1-outwd) * norm;
    return color;
}

int main(int argc, char **argv)
{
    try {
        int device = argc > 1 ? atoi(argv[1]) : 0;
        af::deviceset(device);
        af::info();

        array man       = loadimage("/home/pradeep/gitroot/trailsground/blog_posts/imageediting/part1/man.jpg",true);
        array fight     = loadimage("/home/pradeep/gitroot/trailsground/blog_posts/imageediting/part1/fight.jpg",true);
        array nature    = loadimage("/home/pradeep/gitroot/trailsground/blog_posts/imageediting/part1/nature.jpg",true);
        array lena      = loadimage("/home/pradeep/gitroot/trailsground/blog_posts/imageediting/part2/lena512x512.jpg",true);
        array sudoku    = loadimage("/home/pradeep/gitroot/trailsground/blog_posts/imageediting/part3/sudoku.jpg",true);
        array bimodal   = loadimage("/home/pradeep/gitroot/trailsground/blog_posts/imageediting/part3/bimodal.jpg",true);
        array spider    = loadimage("/home/pradeep/gitroot/trailsground/blog_posts/imageediting/part3/spider.jpg",true);
        array arrow     = loadimage("/home/pradeep/gitroot/trailsground/blog_posts/imageediting/part3/arrow.jpg",true);

        array intensity = colorspace(fight,af_gray,af_rgb);
        array mask      = clamp(intensity,10.0f,255.0f)>0.0f;
        array blend     = alphaBlend(fight,nature,mask);
        array highcon   = changeContrast(man,0.3);
        array highbright= changeBrightness(man,0.2);
        array translated= translate(man,100,100,200,126);
        array sharp     = usm(man,3,1.2);
        array zoom      = digZoom(man,28,10,192,192);

        fig("sub",3,3,1); image(man/255);           fig("title","Input");
        fig("sub",3,3,2); image(highcon/255);       fig("title","High Contrast");
        fig("sub",3,3,3); image(highbright/255);    fig("title","High Brightness");
        fig("sub",3,3,4); image(translated/255);    fig("title","Translation");
        fig("sub",3,3,5); image(sharp/255);         fig("title","Unsharp Masking");
        fig("sub",3,3,6); image(zoom/255);          fig("title","Digital Zoom");
        fig("sub",3,3,7); image(nature/255);        fig("title","Background for blend");
        fig("sub",3,3,8); image(fight/255);         fig("title","Foreground for blend");
        fig("sub",3,3,9); image(blend/255);         fig("title","Alpha blend");
        getchar();

        array prew_mag, prew_dir;
        array sob_mag, sob_dir;
        array lena1ch = colorspace(lena,af_gray,af_rgb);
        prewitt(prew_mag,prew_dir,lena1ch);
        sobel(sob_mag,sob_dir,lena1ch);
        array sprd  = spread(lena,3,3);
        array hrl   = hurl(lena,10,1);
        array pckng = pick(lena,40,2);
        array difog = dog(lena,1,2);
        array bil   = bilateral(hrl,3.0f,40.0f);
        array mf    = medianfilter(hrl,5,5);
        array gb    = gaussianblur(hrl,3,3,0.8);

        fig("sub",3,3,1); image(hrl/255);           fig("title","Hurl noise");
        fig("sub",3,3,2); image(gb/255);            fig("title","Gaussian blur");
        fig("sub",3,3,3); image(bil/255);           fig("title","Bilateral filter on hurl noise");
        fig("sub",3,3,4); image(mf/255);            fig("title","Median filter on hurl noise");
        fig("sub",3,3,5); image(prew_mag/255);      fig("title","Prewitt edge filter");
        fig("sub",3,3,6); image(sob_mag/255);       fig("title","Sobel edge filter");
        fig("sub",3,3,7); image(sprd/255);          fig("title","Spread filter");
        fig("sub",3,3,8); image(pckng/255);         fig("title","Pick filter");
        fig("sub",3,3,9); image(difog/255);         fig("title","Difference of gaussians(3x3 and 5x5)");
        getchar();

        array morph_mask= constant(1,3,3);
        array bdry      = boundary(lena,morph_mask);
        fig("sub",1,1,1); image(bdry/255);          fig("title","Boundary extraction using morph ops");
        getchar();

        array bt = threshold(bimodal,180.0f);
        array ot = otsu(bimodal);
        array mnt= adaptiveThreshold(sudoku,MEAN,37,10);
        array mdt= adaptiveThreshold(sudoku,MEDIAN,7,4);
        array mmt= adaptiveThreshold(sudoku,MINMAX_AVG,11,4);
        array itt= 255.0f-iterativeThreshold(sudoku);
        array emb= emboss(arrow,45,20,10);

        fig("sub",2,3,1); image(sudoku/255);        fig("title","Input");
        fig("sub",2,3,2); image(mnt);               fig("title","Adap. Threshold(Mean)");
        fig("sub",2,3,3); image(mdt);               fig("title","Adap. Threshold(Median)");
        fig("sub",2,3,4); image(mmt);               fig("title","Adap. Threshold(Avg. Min,Max)");
        fig("sub",2,3,5); image(itt);               fig("title","Iterative Threshold");
        getchar();

        fig("sub",3,1,1); image(bimodal/255);       fig("title","Input");
        fig("sub",3,1,2); image(bt);                fig("title","Simple Binary threshold");
        fig("sub",3,1,3); image(ot);                fig("title","Otsu's Threshold");
        getchar();

        fig("sub",2,1,1); image(arrow/255);         fig("title","Input");
        fig("sub",2,1,2); image(emb);               fig("title","Emboss effect");
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
