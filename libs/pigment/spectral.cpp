#include "spectral.h"
#include <cmath>
#include <xsimd_extensions/xsimd.hpp>

using float_v = xsimd::batch<float, xsimd::current_arch>;

static const size_t vectorized_count = 41 - (41 % float_v::size);

// Spectral RGB basis vectors sampled at 10 nm intervals from 380 nm to 780 nm.
// Assumes mixing will happen with the sRGB primaries and the d65 illuminant.
// https://github.com/geometrian/simple-spectral/tree/master
static const float SPD_R[41] = {0.327457414, 0.313439461, 0.239205681, 0.121746068, 0.044433159, 0.022316653, 0.014181107, 0.011986164, 0.010906066, 0.010637360, 0.011032712, 0.011154642, 0.008918582, 0.006705708, 0.005537257, 0.005025362, 0.005433200, 0.006400573, 0.008583636, 0.013565434, 0.032084071, 0.624393724, 0.949253030, 0.958187751, 0.955679061, 0.954101573, 0.938681328, 0.904606025, 0.847787873, 0.752531854, 0.618694571, 0.472964416, 0.405358046, 0.370983585, 0.348712800, 0.341917877, 0.337169504, 0.335167443, 0.334008760, 0.333818455, 0.333569513};
static const float SPD_G[41] = {0.331861713, 0.327860022, 0.294322584, 0.188894319, 0.078687060, 0.042288146, 0.029755948, 0.030988572, 0.034669962, 0.040684806, 0.080905287, 0.379679643, 0.876214748, 0.940655563, 0.961643280, 0.970989746, 0.973116594, 0.973351116, 0.973351022, 0.971061306, 0.954941968, 0.364348804, 0.041230434, 0.031924630, 0.032630370, 0.031561761, 0.041403005, 0.063434300, 0.099542743, 0.157590910, 0.231474475, 0.296029164, 0.317815883, 0.326353848, 0.330808727, 0.331984550, 0.332912009, 0.333027673, 0.333247031, 0.333275050, 0.333309425};
static const float SPD_B[41] = {0.340680792, 0.358700493, 0.466471731, 0.689359611, 0.876879781, 0.935395201, 0.956062945, 0.957025265, 0.954423973, 0.948677833, 0.908062000, 0.609165715, 0.114866670, 0.052638729, 0.032819463, 0.023984891, 0.021450205, 0.020248311, 0.018065342, 0.015373260, 0.012973962, 0.011257478, 0.009516535, 0.009887619, 0.011690569, 0.014336665, 0.019915666, 0.031959674, 0.052669382, 0.089877232, 0.149830947, 0.231006403, 0.276826039, 0.302662506, 0.320478325, 0.326097309, 0.329917976, 0.331803633, 0.332740781, 0.332901731, 0.333111083};

// The CIE-1931 color matching functions with the same wavelength samples.
// The data is normalized as (data * d65) / sum(CIE_CMF_Y * d65).
// https://gist.github.com/mattdesl/73d4e93c78042f3aec5fca5fcaad0dc5
static const float X_BAR[41] = {0.000064692, 0.000219409, 0.001120568, 0.003766593, 0.011880489, 0.023286316, 0.034559231, 0.037223589, 0.032418201, 0.021233091, 0.010490934, 0.003295820, 0.000507032, 0.000948669, 0.006273684, 0.016864533, 0.028689494, 0.042674592, 0.056254444, 0.069470022, 0.083052703, 0.086125631, 0.090465648, 0.085003405, 0.070906294, 0.050628618, 0.035473770, 0.021468094, 0.012516389, 0.006804545, 0.003464547, 0.001497602, 0.000769685, 0.000407342, 0.000168990, 0.000095226, 0.000049025, 0.000019978, 0.000007291, 0.000005247, 0.000002519};
static const float Y_BAR[41] = {0.000001844, 0.000006205, 0.000031009, 0.000104748, 0.000353639, 0.000951466, 0.002282251, 0.004207306, 0.006688762, 0.009888343, 0.015249369, 0.021418195, 0.033422749, 0.051309736, 0.070401703, 0.087838232, 0.094248544, 0.097956131, 0.094151677, 0.086780554, 0.078856107, 0.063526359, 0.053741126, 0.042645834, 0.031617178, 0.020885093, 0.013860035, 0.008102596, 0.004630077, 0.002491367, 0.001259297, 0.000541644, 0.000277951, 0.000147107, 0.000061032, 0.000034387, 0.000017692, 0.000007221, 0.000002635, 0.000001896, 0.000000900};
static const float Z_BAR[41] = {0.000305015, 0.001036801, 0.005313107, 0.017954296, 0.057077273, 0.113651004, 0.173357789, 0.196205515, 0.186081364, 0.139949719, 0.089174036, 0.047895952, 0.028145473, 0.016137575, 0.007759061, 0.004296125, 0.002005498, 0.000861467, 0.000369037, 0.000191428, 0.000149555, 0.000092310, 0.000068135, 0.000028826, 0.000015767, 0.000003941, 0.000001584, 0.000000000, 0.000000000, 0.000000000, 0.000000000, 0.000000000, 0.000000000, 0.000000000, 0.000000000, 0.000000000, 0.000000000, 0.000000000, 0.000000000, 0.000000000, 0.000000000};


static float sumVector(float_v x) {
    float xs[float_v::size];
    x.store_unaligned(xs);

    float sum = 0.0;
    for (size_t i = 0; i < float_v::size; ++i) {
        sum += xs[i];
    }

    return sum;
}

static float linearToKs(float r, float g, float b, float w, size_t i) {
    float spectrum = SPD_R[i] * r + SPD_G[i] * g + SPD_B[i] * b;
    float inv = (1.0 - spectrum);

    return w * (inv * inv / (2.0 * spectrum));
}

static float_v linearToKsV(float r, float g, float b, float w, size_t i) {
    float_v spectrum = float_v::load_unaligned(&SPD_R[i]) * float_v(r) + float_v::load_unaligned(&SPD_G[i]) * float_v(g) + float_v::load_unaligned(&SPD_B[i]) * float_v(b);
    float_v inv = float_v(1.0) - spectrum;

    return float_v(w) * (inv * inv / (float_v(2.0) * spectrum));
}

// https://github.com/rvanwijnen/spectral.js
float linearToWeight(float r, float g, float b, float a) {
    float l = 0.212648173 * r + 0.715179242 * g + 0.072172585 * b;
    return a * a * l;
}

void linearToKs(float r, float g, float b, float w, float* KS) {
    for (size_t i = 0; i < vectorized_count; i += float_v::size) {
        float_v ks = linearToKsV(r, g, b, w, i);
        ks.store_unaligned(&KS[i]);
    }

    for (size_t i = vectorized_count; i < 41; ++i) {
        KS[i] = linearToKs(r, g, b, w, i);
    }
}

void addKs(float* srcKs, float* dstKs) {
    for (size_t i = 0; i < vectorized_count; i += float_v::size) {
        float_v ks = float_v::load_unaligned(&srcKs[i]) + float_v::load_unaligned(&dstKs[i]);
        ks.store_unaligned(&dstKs[i]);
    }

    for (size_t i = vectorized_count; i < 41; ++i) {
        dstKs[i] = srcKs[i] + dstKs[i];
    }
}

void ksToLinear(float* KS, float* r, float* g, float* b, float w, float offset) {
    float invW = 1.0 / w;
    float_v x_v(0.0), y_v(0.0), z_v(0.0);
    for (size_t i = 0; i < vectorized_count; i += float_v::size) {
        float_v ks = float_v::load_unaligned(&KS[i]) * float_v(invW) + float_v(offset);
        float_v spectrum = float_v(1.0) + ks - sqrt(ks * ks + float_v(2.0) * ks);

        x_v += float_v::load_unaligned(&X_BAR[i]) * spectrum;
        y_v += float_v::load_unaligned(&Y_BAR[i]) * spectrum;
        z_v += float_v::load_unaligned(&Z_BAR[i]) * spectrum;
    }

    float x = sumVector(x_v);
    float y = sumVector(y_v);
    float z = sumVector(z_v);

    for (size_t i = vectorized_count; i < 41; ++i) {
        float ks = KS[i] * invW + offset;
        float spectrum = 1.0 + ks - sqrt(ks * ks + 2.0 * ks);

        x += X_BAR[i] * spectrum;
        y += Y_BAR[i] * spectrum;
        z += Z_BAR[i] * spectrum;
    }

    *r =  3.240830229f * x - 1.537316904f * y - 0.498589266f * z;
    *g = -0.969229321f * x + 1.875939794f * y + 0.041554444f * z;
    *b =  0.055645287f * x - 0.204032720f * y + 1.057260459f * z;
}
