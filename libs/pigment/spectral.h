#ifndef SPECTRAL_H_
#define SPECTRAL_H_

#include "kritapigment_export.h"

    /**
     * Conversion of linear sRGB to ksWeight.
     */
KRITAPIGMENT_EXPORT float linearToWeight(float r, float g, float b, float a);

    /**
     * Conversion of linear sRGB to KS with ksWeight.
     */
KRITAPIGMENT_EXPORT void linearToKs(float r, float g, float b, float w, float* KS);

    /**
     * Adding the KS of two linear sRGB colours.
     */
KRITAPIGMENT_EXPORT void addKs(float* srcKs, float* dstKs);

    /**
     * Converting KS to linear sRGB.
     * @param w is the total ksWeight of all KS
     * @param offset is only used in the convolution worker, normally it should be 0
     */
KRITAPIGMENT_EXPORT void ksToLinear(float* KS, float* r, float* g, float* b, float w = 1.0, float offset = 0.0);

#endif
