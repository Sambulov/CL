#include "CodeLib.h"

typedef struct {
    uint8_t *storage;
    uint16_t size;
    uint16_t length    : 15;
    uint16_t head_open : 1;
    uint16_t tail;
    uint16_t open_len  : 15;
    uint16_t open_lost : 1;
} seg_ring_private_t;

LIB_ASSERRT_STRUCTURE_CAST(seg_ring_private_t, seg_ring_t, SEG_RING_PRIVATE_SIZE, SegmentedRing.h);

#define SEG_RING_MARK_SIZE        2
#define SEG_LEN_MAX               0x7fffu
#define SEG_BREAK                 0x8000u

static uint16_t seg_free(const seg_ring_private_t *p) {
  return (p->size - p->length);
}

static uint16_t seg_index_move(seg_ring_private_t *p, uint16_t index, int16_t offset) {
  int32_t ind = index + offset;
  while(ind < 0)
    ind += p->size;
  while(ind >= p->size)
    ind -= p->size;
  return (uint16_t)ind;
}

static void seg_mark_put(seg_ring_private_t *p, uint16_t at, int16_t mark) {
  p->storage[at++] = (uint16_t)mark;
  if(at >= p->size) 
    at = 0;
  p->storage[at] = ((uint16_t)mark) >> 8;
}

static int16_t seg_mark_get(seg_ring_private_t *p, uint16_t at) {
  uint16_t mark = p->storage[at++];
  if(at >= p->size) 
    at = 0;
  mark |= ((uint16_t)p->storage[at]) << 8;
  return (int16_t)mark;
}

static uint8_t *seg_get_linear_buf(seg_ring_private_t *p, uint16_t *size) {
  if(!p->head_open || (p->length >= p->size))
    return libNULL;
  uint16_t start_at = seg_index_move(p, p->tail, p->length);
  uint16_t sz = p->size - start_at;
  if(p->tail > start_at)
    sz = p->tail - start_at;
  *size = sz;
  return &p->storage[start_at];
}

static void seg_open(seg_ring_private_t *p) {
  if(seg_free(p) <= SEG_RING_MARK_SIZE)
    return;
  uint16_t head = seg_index_move(p, p->tail, p->length);
  seg_mark_put(p, head, 0);
  p->length += SEG_RING_MARK_SIZE;
  p->head_open = 1;
}

static void seg_close(seg_ring_private_t *p) {
  if(!p->head_open)
    return;
  int16_t mark = (int16_t)(p->open_len | (p->open_lost? SEG_BREAK: 0));
  uint16_t at = seg_index_move(p, p->tail, p->length - (p->open_len + SEG_RING_MARK_SIZE));
  seg_mark_put(p, at, mark);
  p->open_len = 0;
  p->open_lost = 0;
  p->head_open = 0;
}

uint8_t seg_ring_init(seg_ring_t *desc, uint8_t *storage, uint16_t size) {
  seg_ring_private_t *p = (seg_ring_private_t *)desc;
  if ((p == libNULL) || (storage == libNULL) || (size < SEG_RING_MARK_SIZE))
    return CL_FALSE;
  if(size > SEG_LEN_MAX)
    size = SEG_LEN_MAX;
  mem_set(p, 0, sizeof(seg_ring_private_t));
  p->storage = storage;
  p->size    = size;
  return CL_TRUE;
}

uint16_t seg_ring_write(seg_ring_t *desc, const uint8_t *data, uint16_t len, uint8_t brk) {
  seg_ring_private_t *p = (seg_ring_private_t *)desc;
  if (p == libNULL || p->storage == libNULL || (data == libNULL && len > 0))
      return 0;
  if(!p->head_open)
    seg_open(p);
  if(!p->head_open)
    return 0;
  uint16_t done = 0;
  while (!p->open_lost && (len > done)) {
    uint16_t bl_len = 0;
    uint8_t *blk = seg_get_linear_buf(p, &bl_len);
    if(!blk)
      break;
    uint16_t wr = CL_MIN(bl_len, len - done);
    mem_cpy(blk, &data[done], wr);
    p->length += wr;
    done += wr;
  }
  p->open_len += done;
  p->open_lost = p->open_lost || (len > done);
  if (brk)
    seg_close(p);
  return done;
}

uint8_t  seg_ring_read(seg_ring_t *desc, uint8_t *buf, uint16_t size, seg_ring_chunk_t *out_chunk) {
  seg_ring_private_t *p = (seg_ring_private_t *)desc;
  if (p == libNULL || p->storage == libNULL)
    return 0;
  int16_t mark = 0;
  uint16_t done = 0;
  uint8_t is_open = 1;
  uint16_t available = 0;
  if(p->length) {
    mark = (int16_t)seg_mark_get(p, p->tail);
    is_open = (p->length == (p->open_len + SEG_RING_MARK_SIZE)) && p->head_open;
    if(is_open)
      mark = p->open_len;
    available = (((uint16_t)mark) & ~SEG_BREAK);
    uint16_t read_at = seg_index_move(p, p->tail, SEG_RING_MARK_SIZE);
    uint16_t amount = 0;
    do {
      amount = p->size - read_at;
      amount = CL_MIN(amount, available);
      if(size)
        amount = CL_MIN(amount, size);
      if(buf && amount)
        mem_cpy(&buf[done], &p->storage[read_at], amount);
      done += amount;
      available -= amount;
      read_at = seg_index_move(p, read_at, amount);
    } while (amount && (done < size));
  }

  if(out_chunk) {
    out_chunk->len = done;
    out_chunk->end = !is_open && (!available);
    out_chunk->lost = out_chunk->end && ((uint16_t)mark & SEG_BREAK);
  }

  if(size) {
    if(!is_open && !available)
      done += SEG_RING_MARK_SIZE;

    p->tail = seg_index_move(p, p->tail, done);
    p->length -= done;

    if(is_open) {
      p->open_len = available;
      available = 0;
    }

    if(is_open || available)
      seg_mark_put(p, p->tail, available | ((uint16_t)mark & SEG_BREAK));
  }
  return 1;
}

uint16_t seg_ring_free(seg_ring_t *desc) {
  const seg_ring_private_t *p = (const seg_ring_private_t *)desc;
  if((p == libNULL) || (p->storage == libNULL)) 
    return 0;
  return seg_free(p);
}


uint8_t bSegRingInit(SegRing_t *pxDesc, uint8_t *ucStorage, uint16_t usSize) __attribute__ ((alias ("seg_ring_init")));
uint16_t usSegRingWrite(SegRing_t *pxDesc, const uint8_t *ucData, uint16_t usLen, uint8_t bBreak) __attribute__ ((alias ("seg_ring_write")));
uint8_t  bSegRingRead(SegRing_t *pxDesc, uint8_t *ucBuf, uint16_t usSize, SegRingChunk_t *pxOutChunk) __attribute__ ((alias ("seg_ring_read")));
uint16_t usSegRingFree(SegRing_t *pxDesc) __attribute__ ((alias ("seg_ring_free")));
