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

typedef enum {
	UNSIGNED_INT  = 0,
	SIGNED_INT    = 1,
	BIN_INT       = 2,
	OCT_INT       = 3,
	HEX_INT       = 4,
	INT_FLAG_MASK = 7,
	NO_PREFIX     = 8
} PrintIntegerFlags_t;

typedef int32_t (*PrintfWriter_t)(void *, uint8_t *, uint32_t);

int32_t lClVPrintf(PrintfWriter_t pfWriter, void *pxWrContext, const char *pcFormat, va_list xArgs);
static inline int32_t lClPrintf(PrintfWriter_t pfWriter, void *pxWrContext, const char* pcFormat, ...) {
	va_list args;
	va_start(args, pcFormat);
	int32_t streamed = lClVPrintf(pfWriter, pxWrContext, pcFormat, args);
	va_end(args);
	return streamed;
}
int32_t lClSnprintf(uint8_t *ucBuf, uint32_t ulSize, const char *ucFormat, ...);

int32_t lClPrintInteger(PrintfWriter_t pfWriter, void *pxWrContext, uint64_t ullValue, PrintIntegerFlags_t eFlags);
int32_t lClSnPrintInteger(uint8_t *ucBuf, uint32_t ulSize, uint64_t ullValue, PrintIntegerFlags_t eFlags);
int32_t lClPrintFloat(PrintfWriter_t pfWriter, void *pxWrContext, float fpValue);
int32_t lClSnPrintFloat(uint8_t *ucBuf, uint32_t ulSize, float fpValue);

/*!
  Snake notation
*/

typedef PrintfWriter_t printf_writer_t;
typedef PrintIntegerFlags_t print_integer_flags_t;

int32_t cl_vprintf(printf_writer_t writer, void *wr_context, const char *format, va_list args);
int32_t cl_snprintf(uint8_t *buf, uint32_t size, const char *format, ...);
static inline int32_t cl_printf(printf_writer_t writer, void *wr_context, const char* format, ...)  __attribute__ ((alias ("lClPrintf")));

int32_t cl_print_integer(printf_writer_t pfWriter, void *pxWrContext, uint64_t ullValue, print_integer_flags_t eFlags);
int32_t cl_snprint_integer(uint8_t *ucBuf, uint32_t ulSize, uint64_t ullValue, print_integer_flags_t eFlags);
int32_t cl_print_float(printf_writer_t pfWriter, void *pxWrContext, float fpValue);
int32_t cl_snprint_float(uint8_t *ucBuf, uint32_t ulSize, float fpValue);


#ifdef __cplusplus
}
#endif

#endif /* PRINTF_H_INCLUDED */
