#pragma once

#define SEG_RING_PRIVATE_SIZE     CL_SIZE_OF(1, 6)

typedef struct {
  CL_PRIVATE(SEG_RING_PRIVATE_SIZE);
} seg_ring_t;

typedef struct {
  uint16_t len;
  uint8_t  end;
  uint8_t  lost;
} seg_ring_chunk_t;

uint8_t seg_ring_init(seg_ring_t *desc, uint8_t *storage, uint16_t size);
uint16_t seg_ring_write(seg_ring_t *desc, const uint8_t *data, uint16_t len, uint8_t brk);
uint8_t  seg_ring_read(seg_ring_t *desc, uint8_t *buf, uint16_t size, seg_ring_chunk_t *out_chunk);
uint16_t seg_ring_free(seg_ring_t *desc);

/*!
  Camel notation
*/
typedef seg_ring_t SegRing_t;
typedef seg_ring_chunk_t SegRingChunk_t;

uint8_t bSegRingInit(SegRing_t *pxDesc, uint8_t *ucStorage, uint16_t usSize);
uint16_t usSegRingWrite(SegRing_t *pxDesc, const uint8_t *ucData, uint16_t usLen, uint8_t bBreak);
uint8_t  bSegRingRead(SegRing_t *pxDesc, uint8_t *ucBuf, uint16_t usSize, SegRingChunk_t *pxOutChunk);
uint16_t usSegRingFree(SegRing_t *pxDesc);
