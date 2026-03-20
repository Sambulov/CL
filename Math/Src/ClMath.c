#include "CodeLib.h"

int32_t lGcd(int32_t a, int32_t b) {
  while(a && b) if (a < b) b %= a; else a %= b;
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

void vDecimal32Mull(decimal32_t *pdX, int32_t ulFactor) {
  if(pdX->denom && ulFactor) {
    uint32_t y = gcd(pdX->denom, ulFactor);
    if(y > 1) {
      pdX->denom /= y;
      ulFactor /= y;
    }
  }
  pdX->num *= ulFactor;
}

void vDecimal32Div(decimal32_t *pdX, int32_t ulFactor) {
  if(pdX->num && ulFactor) {
    uint32_t y = gcd(pdX->num, ulFactor);
    pdX->num /= y;
    pdX->denom *= (ulFactor / y);
  }
}


int32_t gcd(int32_t, int32_t) __attribute__ ((alias ("lGcd")));
uint32_t array_median(uint32_t [], uint32_t) __attribute__ ((alias ("ulArrayMedian")));
void decimal32_mull(decimal32_t *, int32_t) __attribute__ ((alias ("vDecimal32Mull")));
void decimal32_div(decimal32_t *, int32_t) __attribute__ ((alias ("vDecimal32Div")));

