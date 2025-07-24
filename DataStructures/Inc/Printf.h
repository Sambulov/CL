/*!
   Printf.h
        Author: Sambulov Dmitry
        e-mail: dmitry.sambulov@gmail.com
 */
#ifndef PRINTF_H_INCLUDED
#define PRINTF_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t (*PrintfWriter_t)(void *, uint8_t *, uint32_t);

int32_t lClVPrintf(PrintfWriter_t pfWriter, void *pxWrContext, const uint8_t *pcFormat, va_list xArgs);

static inline int32_t lClPrintf(PrintfWriter_t pfWriter, void *pxWrContext, const uint8_t* pcFormat, ...) {
	va_list args;
	va_start(args, pcFormat);
	int32_t streamed = lClVPrintf(pfWriter, pxWrContext, pcFormat, args);
	va_end(args);
	return streamed;
}

/*!
  Snake notation
*/

typedef PrintfWriter_t printf_writer_t;

int32_t cl_vprintf(printf_writer_t writer, void *wr_context, const uint8_t *format, va_list args);
int32_t cl_snprintf(uint8_t *buf, uint32_t size, const uint8_t *format, ...);
static inline int32_t cl_printf(printf_writer_t writer, void *wr_context, const uint8_t* format, ...)  __attribute__ ((alias ("lClPrintf")));

#ifdef __cplusplus
}
#endif

#endif /* PRINTF_H_INCLUDED */
