#include "CodeLib.h"

uint32_t HashDjb2 (void *pxData, uint32_t ulSize) {
    /* init: 5381
       hash(i) = hash(i-1)*33 + data(i) */
    uint32_t hash = 5381;
    uint8_t *data = pxData;
    while (ulSize--)
        hash = ((hash << 5) + hash) + *data++;
    return hash;
}

uint32_t HashSdbm (void *pxData, uint32_t ulSize) {
    /* init: 0
       hash(i) = hash(i-1)*65599 + data(i) */
    uint32_t hash = 0;
    uint8_t *data = pxData;
    while (ulSize--)
        hash = ((hash << 6) + (hash << 16) - hash) + *data++;
    return hash;
}

uint32_t HashKnuth (void *pxData, uint32_t ulSize) {
    /* init: 0
       hash(i) = hash(i) * (√5 − 1) / 2 + data(i) 
       (√5 − 1) / 2 ≈ 0.6180339887
    */
    const uint32_t GOLDEN_RATIO = 2654435761U;
    uint32_t hash = 0;
    uint8_t *data = pxData;
    while (ulSize--)
        hash = (hash * GOLDEN_RATIO) + *data++;
    return hash;
}

/*!
  Snake notation
*/

uint32_t hash_dbj2 (void *data, uint32_t size)  __attribute__ ((alias ("HashDjb2")));
uint32_t hash_sdbm (void *data, uint32_t size)  __attribute__ ((alias ("HashSdbm")));
uint32_t hash_knuth (void *data, uint32_t size)  __attribute__ ((alias ("HashKnuth")));
