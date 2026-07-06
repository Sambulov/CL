#ifndef MATH_H_INCLUDED
#define MATH_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int32_t num;                   /* numerator  */
  uint32_t denom;                /* denominator */
} decimal32_t;

int32_t lGcd(int32_t lA, int32_t lB);
uint32_t ulArrayMedian(uint32_t aulArr[], uint32_t ulLen);

decimal32_t dDecimal32FromInt(int32_t ulX);
int32_t lDecimal32Cmp(decimal32_t dX, decimal32_t dY);
decimal32_t dDecimal32Reduce(decimal32_t dX);
decimal32_t dDecimal32Add(decimal32_t dX, decimal32_t dY);
decimal32_t dDecimal32Sub(decimal32_t dX, decimal32_t dY);
decimal32_t dDecimal32Mul(decimal32_t dX, decimal32_t dY);
decimal32_t vDecimal32Div(decimal32_t dX, decimal32_t dY);

/*!
  Snake notation
*/

int32_t gcd(int32_t, int32_t);
uint32_t array_median(uint32_t [], uint32_t);

decimal32_t decimal32_from_int(int32_t);
int32_t decimal32_cmp(decimal32_t, decimal32_t);
decimal32_t decimal32_reduce(decimal32_t);
decimal32_t decimal32_add(decimal32_t, decimal32_t);
decimal32_t decimal32_sub(decimal32_t, decimal32_t);
decimal32_t decimal32_mul(decimal32_t, decimal32_t);
decimal32_t decimal32_div(decimal32_t, decimal32_t);

#ifdef __cplusplus
}
#endif

#endif /* MATH_H_INCLUDED */
