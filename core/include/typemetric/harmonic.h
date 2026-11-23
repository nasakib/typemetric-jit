#ifndef TYPEMETRIC_HARMONIC_H
#define TYPEMETRIC_HARMONIC_H

#include "typemetric/typemetric.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Dozenal (Base-12) Fixed-Point Phase Angle.
 * 1 full circle (2*pi) = 12 * 256 = 3072 dozenal units.
 * Eliminates recurring fractional entropy for fundamental harmonics:
 *   - 1/2 turn  (pi)     = 1536 units (exact integer)
 *   - 1/3 turn  (2*pi/3) = 1024 units (exact integer, vs 0.3333... in base-10)
 *   - 1/4 turn  (pi/2)   =  768 units (exact integer)
 *   - 1/6 turn  (pi/3)   =  512 units (exact integer)
 *   - 1/12 turn (pi/6)   =  256 units (exact integer)
 */
typedef uint16_t tm_dozenal_phase_t;

#define TM_DOZENAL_FULL_CIRCLE  3072
#define TM_DOZENAL_HALF_CIRCLE  1536
#define TM_DOZENAL_THIRD_CIRCLE 1024
#define TM_DOZENAL_QUARTER      768
#define TM_DOZENAL_SIXTH        512
#define TM_DOZENAL_TWELFTH      256

/**
 * Convert continuous floating-point radian angle to exact dozenal fixed-point.
 */
tm_dozenal_phase_t tm_dozenal_from_radians(double radians);

/**
 * Convert dozenal fixed-point phase to radians.
 */
double tm_dozenal_to_radians(tm_dozenal_phase_t phase);

/**
 * Add two dozenal phases with exact modular wrap-around (modulo 3072).
 */
static inline tm_dozenal_phase_t tm_dozenal_add(tm_dozenal_phase_t a, tm_dozenal_phase_t b) {
    uint32_t sum = (uint32_t)a + (uint32_t)b;
    return (tm_dozenal_phase_t)(sum % TM_DOZENAL_FULL_CIRCLE);
}

/**
 * K-Space Harmonic Locality Sensitive Hashing (K-Space LSH):
 * Adapted from BabelForge K-space wave mechanics (babel-core).
 * Computes deterministic phase offsets using spatial wave mixtures in K-space.
 * Nearby type vectors map to identical or harmonic resonance nodes,
 * drastically reducing hash table probe lengths in JIT polymorphic dispatch.
 */
uint32_t tm_kspace_lsh_hash(const tm_type_vector_t *vec, uint64_t seed);

/**
 * Balanced Ternary Representation {-1, 0, +1}:
 * Adapted from ClothOS Polymorphic Radix Engine.
 * Encodes 8 trits into 16 bits (3^8 = 6561 <= 65536).
 * Aligns natively with Walsh-Hadamard character evaluations {-1, +1}
 * and inactive variables {0}, completely eliminating two's complement sign-bit bias.
 */
typedef int8_t tm_trit_t; /* -1, 0, or +1 */

typedef struct {
    tm_trit_t trits[TM_MAX_PARAMS];
    uint8_t arity;
} tm_trit_vector_t;

/**
 * Convert type vector to balanced ternary character signature.
 */
void tm_type_to_balanced_trits(const tm_type_vector_t *vec, tm_trit_vector_t *out_trits);

/**
 * Fast balanced ternary dot product (inner product without carry-propagation stalls).
 */
int32_t tm_trit_dot_product(const tm_trit_vector_t *a, const tm_trit_vector_t *b);

#ifdef __cplusplus
}
#endif

#endif /* TYPEMETRIC_HARMONIC_H */
