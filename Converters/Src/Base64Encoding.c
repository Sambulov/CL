#include "CodeLib.h"

static const int8_t encodeTable[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const int8_t decodeTable[80] = {
                                                62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, 00, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 
};
static const uint8_t encShift[4] = {2, 4, 6, 8};
static const uint8_t dataShift[4] = {8, 4, 2, 0};

int32_t lBase64Encode(uint8_t *pucOutBase64, uint32_t ulSize, const uint8_t *pucData, uint32_t ulLength) {
	if((ulSize < (uint32_t)lBase64EncodeBufferRequired(ulLength)) || (pucData == libNULL))
	    return -1;
	int32_t bufIndex = 0;
	uint8_t encode;
	uint8_t left = 0;
	for(uint32_t i = 0; i < ulLength; i++) {
		encode = left | pucData[i] >> encShift[bufIndex & 3];
		pucOutBase64[bufIndex] = encodeTable[encode & 0x3f];
		bufIndex++;
		left = pucData[i] << dataShift[bufIndex & 3];
		if((bufIndex & 3) == 3)
			i--;
	}
	if (bufIndex & 3) pucOutBase64[bufIndex++] = encodeTable[left & 0x3f];
	if (bufIndex & 3) pucOutBase64[bufIndex++] = '=';
	if (bufIndex & 3) pucOutBase64[bufIndex++] = '=';
	return bufIndex;
}

int32_t lBase64Decode(uint8_t *pucOutData, uint32_t ulSize, const uint8_t *pucBase64, uint32_t ulLength) {
	if ((ulSize < (uint32_t)lBase64DecodeBufferRequired(pucBase64, ulLength)) || (pucOutData == libNULL) || (ulLength & 3)) 
		return -1;
	int32_t bufIndex = 0;
	uint8_t left = 0;
	int8_t sextet;
	for (uint32_t i = 0; i < ulLength; i++) {
	    sextet = pucBase64[i];
		if (((i & 3) >= 2) && pucOutData[i] == '=')
			break;
		if ((sextet < 43) || (sextet > 122) || ((sextet = decodeTable[sextet - 43]) < 0))
			return -1;
		pucOutData[bufIndex] = left;
		pucOutData[bufIndex] |= sextet >> dataShift[i & 3];
		left = sextet << encShift[i & 3];
		if(i & 3)
		    bufIndex++;
	}
	return bufIndex;
}

int32_t lBase64DecodeBufferRequired(const uint8_t *pucBase64, uint32_t ulLength) {
    if(ulLength & 3) return -1;
    if(!ulLength) return 0;
    uint32_t size = (ulLength * 3) >> 2;
    if(pucBase64[--ulLength] == '=') size--;
    if(pucBase64[--ulLength] == '=') size--;
    return size;
}

int32_t base64_encode(uint8_t *out_base64, uint32_t size, const uint8_t *data, uint32_t length) __attribute__((alias ("lBase64Encode")));
int32_t base64_decode(uint8_t *out_data, uint32_t size, const uint8_t *base64, uint32_t length) __attribute__((alias ("lBase64Decode")));
int32_t base64_decode_buffer_required(const uint8_t *base64, uint32_t length) __attribute__((alias ("lBase64DecodeBufferRequired")));
