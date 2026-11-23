#include "typemetric/harmonic.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

tm_dozenal_phase_t tm_dozenal_from_radians(double radians) {
    double two_pi = 2.0 * M_PI;
    double normalized = fmod(radians, two_pi);
    if (normalized < 0.0) {
        normalized += two_pi;
    }
    double units = floor((normalized / two_pi) * (double)TM_DOZENAL_FULL_CIRCLE + 0.5);
    uint32_t int_units = (uint32_t)units;
    return (tm_dozenal_phase_t)(int_units % TM_DOZENAL_FULL_CIRCLE);
}

double tm_dozenal_to_radians(tm_dozenal_phase_t phase) {
    uint32_t p = (uint32_t)phase % TM_DOZENAL_FULL_CIRCLE;
    return ((double)p / (double)TM_DOZENAL_FULL_CIRCLE) * (2.0 * M_PI);
}

uint32_t tm_kspace_lsh_hash(const tm_type_vector_t *vec, uint64_t seed) {
    if (vec == NULL) return 0;

    /* Extract 3D K-space coordinates from packed parameter types */
    double u = (double)(vec->kinds[0] | ((uint32_t)vec->kinds[1] << 8));
    double v = (double)(vec->kinds[2] | ((uint32_t)vec->kinds[3] << 8));
    double w = (double)(vec->kinds[4] | ((uint32_t)vec->kinds[5] << 8));

    /* K-Space scale (1 / 32) */
    const double scale = 0.03125;
    double u_f = u * scale;
    double v_f = v * scale;
    double w_f = w * scale;

    /* Spatial wave mixtures (BabelForge K-space wave equation) */
    double wave1 = sin(sin(u_f) * cos(v_f));
    double wave2 = cos(sin(v_f) * cos(w_f));
    double wave3 = sin(sin(w_f) * cos(u_f));
    double phase = (wave1 + wave2 + wave3) * M_PI;

    /* Discretize to dozenal phase */
    tm_dozenal_phase_t d_phase = tm_dozenal_from_radians(phase);

    /* Mix with scalar key and seed */
    uint64_t h = vec->scalar_key ^ seed;
    h ^= (uint64_t)d_phase * UINT64_C(0x9e3779b97f4a7c15);
    h ^= h >> 16;
    h *= UINT64_C(0xbf58476d1ce4e5b9);
    h ^= h >> 32;

    return (uint32_t)h;
}

void tm_type_to_balanced_trits(const tm_type_vector_t *vec, tm_trit_vector_t *out_trits) {
    if (vec == NULL || out_trits == NULL) return;

    out_trits->arity = vec->arity;
    for (uint8_t i = 0; i < TM_MAX_PARAMS; i++) {
        if (i < vec->arity) {
            tm_type_id_t k = vec->kinds[i];
            if (k == 0) {
                out_trits->trits[i] = 0;   /* Nil / None -> 0 */
            } else if (k <= 3) {
                out_trits->trits[i] = 1;   /* Primitives (Bool, Int, Float) -> +1 */
            } else {
                out_trits->trits[i] = -1;  /* Heap pointers (Strings, Arrays, Objects) -> -1 */
            }
        } else {
            out_trits->trits[i] = 0;
        }
    }
}

int32_t tm_trit_dot_product(const tm_trit_vector_t *a, const tm_trit_vector_t *b) {
    if (a == NULL || b == NULL) return 0;

    int32_t sum = 0;
    uint8_t min_arity = (a->arity < b->arity) ? a->arity : b->arity;

    /* Branchless product in {-1, 0, +1}: product is simply a_i * b_i */
    for (uint8_t i = 0; i < min_arity; i++) {
        sum += ((int32_t)a->trits[i] * (int32_t)b->trits[i]);
    }

    return sum;
}
