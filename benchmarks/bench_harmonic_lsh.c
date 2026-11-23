#include "typemetric/harmonic.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_VECTORS 1000000
#define NUM_BUCKETS 1024

/* Standard FNV-1a Hash */
static uint32_t fnv1a_hash(const tm_type_vector_t *vec) {
    uint32_t h = 2166136261u;
    for (int i = 0; i < vec->arity; i++) {
        h ^= vec->kinds[i];
        h *= 16777619u;
    }
    return h;
}

int main(void) {
    printf("======================================================================\n");
    printf(" HARMONIC K-SPACE LSH & BALANCED TERNARY BENCHMARK\n");
    printf(" Workload: %d polymorphic signatures | Buckets: %d\n", NUM_VECTORS, NUM_BUCKETS);
    printf("======================================================================\n");

    /* 1. Generate polymorphic type signatures */
    tm_type_vector_t *vectors = (tm_type_vector_t *)malloc(NUM_VECTORS * sizeof(tm_type_vector_t));
    for (size_t i = 0; i < NUM_VECTORS; i++) {
        tm_type_id_t kinds[6];
        kinds[0] = (tm_type_id_t)(i % 5);
        kinds[1] = (tm_type_id_t)((i / 5) % 5);
        kinds[2] = (tm_type_id_t)((i / 25) % 5);
        kinds[3] = (tm_type_id_t)((i / 125) % 5);
        kinds[4] = (tm_type_id_t)((i / 625) % 5);
        kinds[5] = (tm_type_id_t)((i / 3125) % 5);
        tm_vector_init(&vectors[i], kinds, 6, (uint32_t)i);
    }

    /* ------------------------------------------------------------- */
    /* Benchmark A: Standard FNV-1a Hashing                         */
    /* ------------------------------------------------------------- */
    clock_t t0 = clock();
    uint32_t fnv_buckets[NUM_BUCKETS] = {0};
    for (size_t i = 0; i < NUM_VECTORS; i++) {
        uint32_t h = fnv1a_hash(&vectors[i]);
        fnv_buckets[h % NUM_BUCKETS]++;
    }
    clock_t t1 = clock();
    double fnv_time = (double)(t1 - t0) / CLOCKS_PER_SEC;

    /* ------------------------------------------------------------- */
    /* Benchmark B: K-Space Harmonic LSH (BabelForge K-Space Waves)  */
    /* ------------------------------------------------------------- */
    clock_t t2 = clock();
    uint32_t kspace_buckets[NUM_BUCKETS] = {0};
    for (size_t i = 0; i < NUM_VECTORS; i++) {
        uint32_t h = tm_kspace_lsh_hash(&vectors[i], 0x12345678ULL);
        kspace_buckets[h % NUM_BUCKETS]++;
    }
    clock_t t3 = clock();
    double kspace_time = (double)(t3 - t2) / CLOCKS_PER_SEC;

    /* Measure collision max load */
    uint32_t fnv_max = 0;
    uint32_t kspace_max = 0;
    for (int b = 0; b < NUM_BUCKETS; b++) {
        if (fnv_buckets[b] > fnv_max) fnv_max = fnv_buckets[b];
        if (kspace_buckets[b] > kspace_max) kspace_max = kspace_buckets[b];
    }

    /* ------------------------------------------------------------- */
    /* Benchmark C: Balanced Ternary Dot Product vs Standard         */
    /* ------------------------------------------------------------- */
    tm_trit_vector_t trit_a, trit_b;
    tm_type_to_balanced_trits(&vectors[0], &trit_a);
    tm_type_to_balanced_trits(&vectors[1], &trit_b);

    clock_t t4 = clock();
    int32_t trit_sum = 0;
    for (size_t i = 0; i < NUM_VECTORS; i++) {
        trit_sum += tm_trit_dot_product(&trit_a, &trit_b);
    }
    clock_t t5 = clock();
    double trit_time = (double)(t5 - t4) / CLOCKS_PER_SEC;

    printf("\nHashing Results:\n");
    printf("  Standard FNV-1a Hash:          %.4f s (Max bucket load: %u)\n", fnv_time, fnv_max);
    printf("  BabelForge K-Space LSH:        %.4f s (Max bucket load: %u)\n", kspace_time, kspace_max);
    printf("  Balanced Ternary Dot Product:  %.4f s (%d ops)\n", trit_time, NUM_VECTORS);

    /* Dozenal Phase exactness test */
    tm_dozenal_phase_t third = tm_dozenal_from_radians(2.0 * 3.14159265358979323846 / 3.0);
    printf("\nDozenal Harmonic Invariant:\n");
    printf("  2*pi / 3 in Dozenal:           %u units (Exact: %s)\n",
           third, (third == TM_DOZENAL_THIRD_CIRCLE) ? "YES (1024 / 3072, zero drift)" : "NO");
    printf("======================================================================\n");

    free(vectors);
    return 0;
}
