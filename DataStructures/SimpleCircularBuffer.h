/*!
    SimpleCircularBuffer.h
  
    Created on: 21.05.2024
        Author: Sambulov Dmitry
        e-mail: dmitry.sambulov@gmail.com
 */
#ifndef SIMPLE_CIRCULAR_BUFFER_H_INCLUDED
#define SIMPLE_CIRCULAR_BUFFER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint8_t *pucBuffer;
  uint16_t usTail;
  uint16_t usHead;
  uint16_t usMax;
} SimpleCircularBuffer_t;

/*!
  Note: buffer size must be power of 2
*/
static inline void vScbInit(SimpleCircularBuffer_t *pxBuf, uint8_t *pucBuffer, uint16_t usSize) {
  pxBuf->usHead = pxBuf->usTail = 0;
  pxBuf->usMax = usSize - 1;
  pxBuf->pucBuffer = pucBuffer;
}

static inline int32_t usScbAvailable(SimpleCircularBuffer_t *pxBuf) {
  return (pxBuf->usHead - pxBuf->usTail) & pxBuf->usMax;
}

static inline int32_t usScbAvailableFree(SimpleCircularBuffer_t *pxBuf) {
  return pxBuf->usMax + pxBuf->usTail - pxBuf->usHead;
}

static inline int32_t usScbNonSegmentedAvailable(SimpleCircularBuffer_t *pxBuf) {
  return (pxBuf->usHead >= pxBuf->usTail)? (pxBuf->usHead - pxBuf->usTail): (pxBuf->usMax - pxBuf->usTail);
}

static inline int32_t usScbNonSegmentedAvailableFree(SimpleCircularBuffer_t *pxBuf) {
  return (pxBuf->usHead >= pxBuf->usTail)? (pxBuf->usMax - pxBuf->usHead): (pxBuf->usTail - pxBuf->usHead);
}

static inline void vScbMoveTail(SimpleCircularBuffer_t *pxBuf, uint16_t usCount) {
  uint16_t cnt = usScbAvailable(pxBuf);
  cnt = CL_MIN(cnt, usCount);
  pxBuf->usTail = (pxBuf->usTail + cnt) & pxBuf->usMax;
}

static inline void vScbMoveHead(SimpleCircularBuffer_t *pxBuf, uint16_t usCount) {
  uint16_t cnt = usScbAvailableFree(pxBuf);
  cnt = CL_MIN(cnt, usCount);
  pxBuf->usHead = (pxBuf->usHead + cnt) & pxBuf->usMax;
}

static inline uint8_t *pucScbData(SimpleCircularBuffer_t *pxBuf) {
  return pxBuf->pucBuffer + pxBuf->usTail;
}

static inline uint8_t *pucScbBuffer(SimpleCircularBuffer_t *pxBuf) {
  return pxBuf->pucBuffer + pxBuf->usHead;
}

static inline uint8_t bScbPush(SimpleCircularBuffer_t *pxBuf, uint8_t ucData) {
  if(usScbAvailableFree(pxBuf)) {
    pxBuf->pucBuffer[pxBuf->usHead] = ucData;
    pxBuf->usHead = (pxBuf->usHead + 1) & pxBuf->usMax;
    return 1;
  }
  return 0;
}

static inline uint8_t bScbPop(SimpleCircularBuffer_t *pxBuf, uint8_t *pucOutData) {
  if(usScbAvailable(pxBuf)) {
    if(pucOutData) *pucOutData = pxBuf->pucBuffer[pxBuf->usTail];
    pxBuf->usTail = (pxBuf->usTail + 1) & pxBuf->usMax;
    return 1;
  }
  return 0;
}

static int32_t _lScbRead(SimpleCircularBuffer_t *pxBuf, uint8_t *pucBuf, uint16_t usSize) {
  int32_t i = usSize;
  while(i && bScbPop(pxBuf, pucBuf++))
    i--;
  return usSize - i;
}

static int32_t _lScbWrite(SimpleCircularBuffer_t *pxBuf, const uint8_t *pucData, uint16_t len) {
  int32_t i = len;
  while(i && bScbPush(pxBuf, *pucData++))
    i--;
  return len - i;
}


/*!
  Snake notation
*/

typedef SimpleCircularBuffer_t simple_circular_buffer_t;

static inline void scb_init(simple_circular_buffer_t *, uint8_t *, uint16_t)\
                                                      __attribute__ ((alias ("vScbInit")));

static inline int32_t scb_available(simple_circular_buffer_t *)\
                                                      __attribute__ ((alias ("usScbAvailable")));

static inline int32_t scb_non_segmented_available(simple_circular_buffer_t *)\
                                                      __attribute__ ((alias ("usScbNonSegmentedAvailable")));

static inline int32_t scb_available_free(simple_circular_buffer_t *)\
                                                      __attribute__ ((alias ("usScbAvailableFree")));

static inline int32_t scb_non_segmented_available_free(simple_circular_buffer_t *)\
                                                      __attribute__ ((alias ("usScbNonSegmentedAvailableFree")));

static inline void scb_move_tail(simple_circular_buffer_t *, uint16_t)\
                                                      __attribute__ ((alias ("vScbMoveTail")));

static inline void scb_move_head(simple_circular_buffer_t *, uint16_t)\
                                                      __attribute__ ((alias ("vScbMoveHead")));

static inline uint8_t *scb_data(simple_circular_buffer_t *)\
                                                      __attribute__ ((alias ("pucScbData")));

static inline uint8_t *scb_buffer(simple_circular_buffer_t *)\
                                                      __attribute__ ((alias ("pucScbBuffer")));

static inline uint8_t scb_push(simple_circular_buffer_t *, uint8_t)\
                                                      __attribute__ ((alias ("bScbPush")));

static inline uint8_t scb_pop(simple_circular_buffer_t *, uint8_t *)\
                                                      __attribute__ ((alias ("bScbPop")));

static int32_t scb_read(simple_circular_buffer_t *, uint8_t *, uint16_t)\
                                                      __attribute__ ((alias ("_lScbRead")));

static int32_t scb_write(simple_circular_buffer_t *, const uint8_t *, uint16_t)\
                                                      __attribute__ ((alias ("_lScbWrite")));

#ifdef __cplusplus
}
#endif

#endif /* SIMPLE_CIRCULAR_BUFFER_H_INCLUDED */
