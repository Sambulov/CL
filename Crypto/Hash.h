#ifndef HASH_H_INCLUDED
#define HASH_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

uint32_t HashDjb2 (void *pxData, uint32_t ulSize);
uint32_t HashSdbm (void *pxData, uint32_t ulSize);
uint32_t HashKnuth (void *pxData, uint32_t ulSize);

/*!
  Snake notation
*/

uint32_t hash_dbj2 (void *data, uint32_t size);
uint32_t hash_sdbm (void *data, uint32_t size);
uint32_t hash_knuth (void *data, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif /* HASH_H_INCLUDED */
