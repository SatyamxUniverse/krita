#ifndef SPECTRAL_H_
#define SPECTRAL_H_

#include "kritapigment_export.h"

KRITAPIGMENT_EXPORT float concentration(float sr, float sg, float sb, float sw, float dr, float dg, float db);
KRITAPIGMENT_EXPORT void LinearToSpectrum(float r, float g, float b, float* spectrum);
KRITAPIGMENT_EXPORT void spectrumToLinear(float* spectrum, float* r, float* g, float* b);
KRITAPIGMENT_EXPORT void mixSpectrums(float* spectrum_s, float* spectrum_d, float sw);

#endif
