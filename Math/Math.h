#ifndef MATH_H_INCLUDED
#define MATH_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint32_t num;                   /* numerator  */
  uint32_t denom;                 /* denominator */
} decimal32_t;

int32_t lGcd(int32_t lA, int32_t lB);
uint32_t ulArrayMedian(uint32_t aulArr[], uint32_t ulLen);
void vDecimal32Mull(decimal32_t *pdX, uint32_t ulFactor);
void vDecimal32Div(decimal32_t *pdX, uint32_t ulFactor);

int32_t gcd(int32_t a, int32_t b);
uint32_t array_median(uint32_t arr[], uint32_t len);
void decimal32_mull(decimal32_t *x, uint32_t factor);
void decimal32_div(decimal32_t *x, uint32_t factor);

#ifdef __cplusplus
}
#endif

#endif /* MATH_H_INCLUDED */
