#ifndef BASE64_ENCODING_H_INCLUDED
#define BASE64_ENCODING_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

int32_t lBase64Encode(uint8_t *pucOutBase64, uint32_t ulSize, const uint8_t *pucData, uint32_t ulLength);
int32_t lBase64Decode(uint8_t *pucOutData, uint32_t ulSize, const uint8_t *pucBase64, uint32_t ulLength);

static inline int32_t lBase64EncodeBufferRequired(uint32_t ulLength) {
    return CL_SIZE_ALIGN4(ulLength * 4 / 3);
}

int32_t lBase64DecodeBufferRequired(const uint8_t *pucBase64, uint32_t ulLength);

/*!
  Snake notation
*/

int32_t base64_encode(uint8_t *out_base64, uint32_t size, const uint8_t *data, uint32_t length);
int32_t base64_decode(uint8_t *out_data, uint32_t size, const uint8_t *base64, uint32_t length);

static inline int32_t base64_encode_buffer_required(uint32_t length) __attribute__((alias ("lBase64EncodeBufferRequired")));
int32_t base64_decode_buffer_required(const uint8_t *base64, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif /* BASE64_ENCODING_H_INCLUDED */
