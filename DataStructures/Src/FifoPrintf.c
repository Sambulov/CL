#include "CodeLib.h"

static inline int32_t _lFifoPrintfWriter(void *pxDesc, uint8_t *ucBuf, uint32_t ulLen) {
	return lFifoWrite((Fifo_t *)pxDesc, ucBuf, ulLen);
}

inline int32_t lFifoVPrintf(Fifo_t *pxFifo, const char* pcFormat, va_list xArgs) {
	return lClVPrintf(&_lFifoPrintfWriter, pxFifo, pcFormat, xArgs);
}

inline int32_t lFifoPrintInteger(Fifo_t *pxFifo, uint64_t ullValue, FifoPrintIntegerFlags_t eFlags) {
	return lClPrintInteger(&_lFifoPrintfWriter, pxFifo, ullValue, eFlags);
}

inline int32_t lFifoPrintFloat(Fifo_t *pxFifo, float fpValue) {
	return lClPrintFloat(&_lFifoPrintfWriter, pxFifo, fpValue);
}

int32_t fifo_print_float(Fifo_t *, float)  __attribute__ ((alias ("lFifoPrintFloat")));
int32_t fifo_print_integer(fifo_t *, uint64_t, fifo_print_integer_flags_t)  __attribute__ ((alias ("lFifoPrintInteger")));
int32_t fifo_vprintf(fifo_t *, const char*, va_list)   __attribute__ ((alias ("lFifoVPrintf")));
