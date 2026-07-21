#ifndef MATH_H_INCLUDED
#define MATH_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int32_t num;                   /* numerator  */
  uint32_t denom;                /* denominator */
} decimal32_t;

#define DECIMAL32(N, D)  (decimal32_t){.num = N, .denom = D}
#define DECIMAL32_NAN  DECIMAL32(0, 0)

int32_t lGcd(int32_t lA, int32_t lB);
uint32_t ulArrayMedian(uint32_t aulArr[], uint32_t ulLen);

decimal32_t xDecimal32FromInt(int32_t ulX);
static inline decimal32_t xDecimal32(int32_t lNum, uint32_t ulDenom) {return DECIMAL32(lNum, ulDenom);}
static inline decimal32_t xDecimal32NaN() {return DECIMAL32(0, 0);}
uint8_t bDecimal32Valid(decimal32_t dX);
int32_t lDecimal32Cmp(decimal32_t dX, decimal32_t dY);
decimal32_t xDecimal32Reduce(decimal32_t dX);
int32_t lDecimal32Round(decimal32_t dX, uint32_t ulDegree, int32_t lDefault);
decimal32_t xDecimal32Add(decimal32_t dX, decimal32_t dY);
decimal32_t xDecimal32Sub(decimal32_t dX, decimal32_t dY);
decimal32_t xDecimal32Mul(decimal32_t dX, decimal32_t dY);
decimal32_t vDecimal32Div(decimal32_t dX, decimal32_t dY);

/*!
  Snake notation
*/

int32_t gcd(int32_t, int32_t);
uint32_t array_median(uint32_t [], uint32_t);

decimal32_t decimal32_from_int(int32_t);
static inline decimal32_t decimal32(int32_t num, uint32_t denom) {return DECIMAL32(num, denom);}
static inline decimal32_t decimal32_nan() {return DECIMAL32(0, 0);}
uint8_t decimal32_valid(decimal32_t);
int32_t decimal32_cmp(decimal32_t, decimal32_t);
decimal32_t decimal32_reduce(decimal32_t);
int32_t decimal32_round(decimal32_t, uint32_t, int32_t);
decimal32_t decimal32_add(decimal32_t, decimal32_t);
decimal32_t decimal32_sub(decimal32_t, decimal32_t);
decimal32_t decimal32_mul(decimal32_t, decimal32_t);
decimal32_t decimal32_div(decimal32_t, decimal32_t);

#ifdef __cplusplus
}
#endif

#endif /* MATH_H_INCLUDED */
