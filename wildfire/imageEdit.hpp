#ifndef IMAGEEDIT_HPP
#define IMAGEEDIT_HPP

#include <arrayfire.h>

/**
 * contrast value should be in the rnage [-1,1]
 * */
af::array changeContrast(const af::array &in, const float contrast);

/**
 * brightness value should be in the rnage [0,1]
 * */
af::array changeBrightness(const af::array &in, const float brightness, const float channelMax=255.0f);

/**
 * radius effects the level of details that will effected during sharpening process
 * amount value should be in the range [0,1] or [1,]
 * note: value of 1.0 for amount results unsharp masking
 *       values > 1.0 results in highboost filter effect
 * */
af::array usm(const af::array &in, int radius, float amount);

af::array digZoom(const af::array &in, int x, int y, int width, int height);

af::array alphaBlend(const af::array &a, const af::array &b, const af::array &mask);

#endif // IMAGEEDIT_HPP
