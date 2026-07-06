#include "CodeLib.h"

int32_t lGcd(int32_t a, int32_t b) {
  if(a < 0) a = -a;
  if(b < 0) b = -b;
  while(a && b) 
    if (a < b) 
      b %= a; 
    else 
      a %= b;
  return a + b;
}

uint32_t ulArrayMedian(uint32_t aulArr[], uint32_t ulLen) {
  uint32_t am = ulLen >> 1;
  if(ulLen && am) {
    for(uint32_t j = 0; j < ulLen; j++) {
      uint8_t bigger = 0;
      uint8_t lower = 0;
      uint32_t nb = -1;
      uint32_t nl = 0;            
      for(uint32_t i = 0; i < ulLen && (bigger <= am) && (lower <= am); i++) {
        if(aulArr[i] > aulArr[j]) {
          bigger++;
          if(nb > aulArr[i]) {
            nb = aulArr[i];
          }
        }
        else if((aulArr[i] != aulArr[j])) {
          lower++;
          if(nl < aulArr[i]) {
            nl = aulArr[i];
          }
        }
      }
      if((bigger <= am) && (lower <= am)) {
        if(!(ulLen & 1)) {
          if(bigger == am) {
            return (aulArr[j] + nb) >> 1;
          }
          if(lower == am) {
            return (aulArr[j] + nl) >> 1;
          }
        }
        return aulArr[j];
      }
    }
  }
  return aulArr[0];
}

decimal32_t dDecimal32Reduce(decimal32_t dX) {
  if (!dX.denom) {
    if(dX.num >= 0)
        dX.num = 1;
    else
        dX.num = -1;
    return dX;
  }
  if (!dX.num) {
    dX.denom = 1;
    return dX;
  }
  uint32_t g = lGcd((dX.num > 0 ? dX.num : -dX.num), (int32_t)dX.denom);
  return (decimal32_t){ .num = dX.num / (int32_t)g, .denom = dX.denom / g };
}

static decimal32_t dDecimal32Reduce64(int64_t llNom, uint64_t ullDenom) {
  uint8_t sign = llNom < 0;
  if(sign)
    llNom = -llNom;
  while (((llNom > __INT32_MAX__) || (ullDenom > __UINT32_MAX__))& (ullDenom > 1)) {
      llNom >>= 1;
      ullDenom >>= 1;
  }
  if(llNom == 0)
    ullDenom = 1;
  if(llNom > __INT32_MAX__)
    llNom = __INT32_MAX__;
  if(sign)
    llNom = -llNom;
  return dDecimal32Reduce(
    (decimal32_t){.num = (int32_t)llNom, .denom = (uint32_t)ullDenom}
  );
}

int32_t lDecimal32Cmp(decimal32_t dX, decimal32_t dY) {
  int64_t lhs = (int64_t)dX.num * (int64_t)dY.denom;
  int64_t rhs = (int64_t)dY.num * (int64_t)dX.denom;
  if (lhs < rhs) return -1;
  if (lhs > rhs) return 1;
  return 0;
}

decimal32_t dDecimal32Add(decimal32_t dX, decimal32_t dY) {
  return dDecimal32Reduce64(
    (int64_t)dX.num * (int64_t)dY.denom + (int64_t)dY.num * (int64_t)dX.denom,
    (uint64_t)dX.denom * (uint64_t)dY.denom
  );
}

decimal32_t dDecimal32Sub(decimal32_t dX, decimal32_t dY) {
  return dDecimal32Add(
    dX, 
    (decimal32_t){.num = -dY.num, .denom = dY.denom}
  );
}

decimal32_t dDecimal32Mul(decimal32_t dX, decimal32_t dY) {
  return dDecimal32Reduce64(
    (int64_t)dX.num * (int64_t)dY.num,
    (uint64_t)dX.denom * (uint64_t)dY.denom
  );
}

decimal32_t dDecimal32Div(decimal32_t dX, decimal32_t dY) {
  decimal32_t inv;
  if (dY.num >= 0) {
    inv.num = (int32_t)dY.denom;
    inv.denom = (uint32_t)dY.num;
  } else {
    inv.num = -(int32_t)dY.denom;
    inv.denom = (uint32_t)(-dY.num);
  }
  return dDecimal32Mul(dX, inv);
}

decimal32_t dDecimal32FromInt(int32_t ulX) {
  return (decimal32_t){.num = ulX, .denom = 1};
}


decimal32_t dDecimal32VectorMul(const decimal32_t *aV1, const decimal32_t *aV2, uint32_t lLen) {
  decimal32_t sum = dDecimal32FromInt(0);
  for (uint32_t i = 0; i < lLen; i++)
    sum = dDecimal32Add(sum, dDecimal32Mul(aV1[i], aV2[i]));
  return sum;
}

void vDecimal32VectorSum(decimal32_t *aOut, decimal32_t dAlpha, const decimal32_t *aV1, decimal32_t dBeta, const decimal32_t *aV2, uint32_t lLen) {
  for (uint32_t i = 0; i < lLen; i++)
    aOut[i] = dDecimal32Add(dDecimal32Mul(dAlpha, aV1[i]), dDecimal32Mul(dBeta, aV2[i]));
}

int32_t gcd(int32_t, int32_t) __attribute__ ((alias ("lGcd")));
uint32_t array_median(uint32_t [], uint32_t) __attribute__ ((alias ("ulArrayMedian")));

decimal32_t decimal32_from_int(int32_t) __attribute__ ((alias ("dDecimal32FromInt")));
int32_t decimal32_cmp(decimal32_t, decimal32_t) __attribute__ ((alias ("lDecimal32Cmp")));
decimal32_t decimal32_reduce(decimal32_t) __attribute__ ((alias ("dDecimal32Reduce")));
decimal32_t decimal32_add(decimal32_t, decimal32_t) __attribute__ ((alias ("dDecimal32Add")));
decimal32_t decimal32_sub(decimal32_t, decimal32_t) __attribute__ ((alias ("dDecimal32Sub")));
decimal32_t decimal32_mul(decimal32_t, decimal32_t) __attribute__ ((alias ("dDecimal32Mul")));
decimal32_t decimal32_div(decimal32_t, decimal32_t) __attribute__ ((alias ("dDecimal32Div")));


decimal32_t decimal32_vector_mul(const decimal32_t *, const decimal32_t *, uint32_t) __attribute__ ((alias ("dDecimal32VectorMul")));
