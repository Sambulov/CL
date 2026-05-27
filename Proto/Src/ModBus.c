#include "CodeLib.h"

typedef enum {
    eModbusPacketNone        = 0,
    eModbusPacketBase        = 1,    /* [ADDR][FUNC][REG_ADDR_HI][REG_ADDR_LO][REG_COUNT_HI][REG_COUNT_LO] */
    eModbusPacketVariableLen = 2,    /* [ADDR][FUNC][LEN][DATA...] */
    eModbusPacketFull        = 3,    /* [ADDR][FUNC][REG_ADDR_HI][REG_ADDR_LO][REG_COUNT_HI][REG_COUNT_LO][LEN][DATA...] */
    eModbusPacketCode        = 4,    /* [ADDR][FUNC][CODE] */
    eModbusPacketCall        = 8,    /* [ADDR][FUNC] */
} ModbusPacketType_t;

static ModbusPacketType_t _eModbusFuncToPacketType(uint8_t ucFunc, uint8_t bIsRequest) {
    if (ucFunc & MODBUS_ERROR_FLAG) return eModbusPacketCode;
    switch (ucFunc) {
        case 0x01: case 0x02: case 0x03: case 0x04: /* read */
            return bIsRequest ? eModbusPacketBase: eModbusPacketVariableLen;
        case 0x05: case 0x06: /* write single */
            return eModbusPacketBase;
        case 0x07: /* read exception status, event counter, slave ID */
            return bIsRequest ? eModbusPacketCall: eModbusPacketCode;
        case 0x0B /* Get Com Event Counter */:
            return bIsRequest ? eModbusPacketCall: eModbusPacketBase;
        case 0x0C: case 0x11: /* Get Com Event Log, Report Slave ID */
            return bIsRequest ? eModbusPacketCall: eModbusPacketVariableLen;
        case 0x0F: case 0x10: /* write multiple */
            return bIsRequest ? eModbusPacketFull: eModbusPacketBase;
        case 0x14: case 0x15: /* Read File Record, Write File Record */
            return bIsRequest ? eModbusPacketVariableLen: eModbusPacketVariableLen;
        default:
            return eModbusPacketNone;
    }
}

typedef struct {
    const ModbusIface_t *pxIface;
    ModbusCb_t pfCallback;
    void *pxTxContext;
    void *pxTimerContext;
    void *pxCbContext;
    ModbusFrame_t xFrame;

    uint32_t ulTimer;

    uint16_t usTxTimeout;
    uint16_t usRxTimeout;

    uint8_t eState           :3;          /* MbClientState_t */
    uint8_t eMode            :3;
    uint8_t bIsServer        :1;
    uint8_t bAsciiHasNibble  :1;

    uint16_t ucPayLoadBufferSize;    
    uint8_t *pucPayLoadBuffer;

    uint16_t usExpectedTransId;
    uint16_t usTransactionId;


    uint16_t usTxCnt;
    uint16_t usFrameLen;

} _prModbus_t;

LIB_ASSERRT_STRUCTURE_CAST(_prModbus_t, Modbus_t, MODBUS_DESCRIPTOR_SIZE, "ModBus.h");

static const uint16_t crc16_table[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

uint16_t _usCrc16(uint8_t *pucData, uint32_t ulLen) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < ulLen; i++) {
        uint8_t index = (crc ^ pucData[i]);
        crc = (crc >> 8) ^ crc16_table[index];
    }
    return crc;
}

// static uint16_t _usCrc16(const uint8_t *pucData, uint16_t ulLen) {
//     uint16_t crc = 0xFFFF;
//     for (uint16_t i = 0; i < ulLen; i++) {
//         crc ^= pucData[i];
//         for (int j = 0; j < 8; j++) {
//             if (crc & 1) crc = (crc >> 1) ^ 0xA001;
//             else crc >>= 1;
//         }
//     }
//     return crc;
// }

static uint8_t _ucLrc(const uint8_t *pucData, uint16_t ulLen) {
    uint8_t lrc = 0;
    for (uint16_t i = 0; i < ulLen; i++) 
        lrc += pucData[i];
    return (uint8_t)(-lrc);
}

static void _vByte2AsciiHex(uint8_t ucByte, uint8_t *pucOut, uint8_t *pucOutLrc) {
    uint8_t h = (ucByte >> 4) & 0x0F;
    uint8_t l = ucByte & 0x0F;
    pucOut[0] = (h < 10) ? ('0' + h) : ('A' + h - 10);
    pucOut[1] = (l < 10) ? ('0' + l) : ('A' + l - 10);
    *pucOutLrc += ucByte;
}

static inline uint32_t _ulNow(_prModbus_t *pxMb) {
    return pxMb->pxIface->pfTimer(pxMb->pxTimerContext);
}

static uint8_t _ucFrame2Raw(_prModbus_t *pxMb, ModbusFrame_t *pxFrame, uint8_t isResponse) {
    uint16_t len = 0;
    uint8_t ucFunc = pxFrame->ucFunc;
    ModbusPacketType_t eType = _eModbusFuncToPacketType(ucFunc, !isResponse);
    if (eType == eModbusPacketNone) 
        return 0;
    if (pxMb->eMode == MB_MODE_ASCII) {
        if(pxMb->ucPayLoadBufferSize < 9)
            return 0;
        pxMb->pucPayLoadBuffer[len++] = ':';
        uint8_t lrc = 0;
        _vByte2AsciiHex(pxFrame->ucAddr, &pxMb->pucPayLoadBuffer[len++], &lrc); len++;
        _vByte2AsciiHex(pxFrame->ucFunc, &pxMb->pucPayLoadBuffer[len++], &lrc); len++;
        if ((eType == eModbusPacketBase) || (eType == eModbusPacketFull)) {
            if(pxMb->ucPayLoadBufferSize < 17)
                return 0;
            _vByte2AsciiHex((uint8_t)(pxFrame->usRegAddr >> 8), &pxMb->pucPayLoadBuffer[len++], &lrc); len++;
            _vByte2AsciiHex((uint8_t)(pxFrame->usRegAddr), &pxMb->pucPayLoadBuffer[len++], &lrc); len++;
            _vByte2AsciiHex((uint8_t)(pxFrame->usRegValueCount >> 8), &pxMb->pucPayLoadBuffer[len++], &lrc); len++;
            _vByte2AsciiHex((uint8_t)(pxFrame->usRegValueCount), &pxMb->pucPayLoadBuffer[len++], &lrc); len++;
        }
        if ((eType == eModbusPacketVariableLen) || (eType == eModbusPacketFull)) {
            if(pxMb->ucPayLoadBufferSize < (len + 6 + (pxFrame->ucLengthCode * 2)))
                return 0;
            _vByte2AsciiHex(pxFrame->ucLengthCode, &pxMb->pucPayLoadBuffer[len++], &lrc); len++;
            for (uint8_t _i = 0; _i < pxFrame->ucLengthCode; _i++) {
                _vByte2AsciiHex(pxFrame->pucData[_i], &pxMb->pucPayLoadBuffer[len++], &lrc); len++;
            }
        }
        if (eType == eModbusPacketCode) {
            if(pxMb->ucPayLoadBufferSize < 11)
                return 0;
            _vByte2AsciiHex(pxFrame->ucLengthCode, &pxMb->pucPayLoadBuffer[len++], &lrc); len++;
        }
        _vByte2AsciiHex(-lrc, &pxMb->pucPayLoadBuffer[len++], &lrc); len++;
        pxMb->pucPayLoadBuffer[len++] = '\r';
        pxMb->pucPayLoadBuffer[len++] = '\n';
    }
    else {
        /* ----- RTU PDU TCP UDP ----- */
        uint16_t pdu_len = 1;
        if      (eType == eModbusPacketBase)        pdu_len += 4;
        else if (eType == eModbusPacketVariableLen) pdu_len += 1 + pxFrame->ucLengthCode;
        else if (eType == eModbusPacketFull)        pdu_len += 5 + pxFrame->ucLengthCode;
        else if (eType == eModbusPacketCode)        pdu_len += 1;
        uint16_t frame_len = pdu_len;
        if(pxMb->eMode == MB_MODE_RTU)
            frame_len += 3;
        else if(pxMb->eMode == MB_MODE_TCP_UDP)
            frame_len += 7;
        if(pxMb->ucPayLoadBufferSize < frame_len)
            return 0;
        if(pxMb->eMode == MB_MODE_TCP_UDP) {
            pxMb->pucPayLoadBuffer[len++] = pxMb->usTransactionId >> 8;
            pxMb->pucPayLoadBuffer[len++] = pxMb->usTransactionId;
            pxMb->pucPayLoadBuffer[len++] = 0; 
            pxMb->pucPayLoadBuffer[len++] = 0;
            pxMb->pucPayLoadBuffer[len++] = (++pdu_len) >> 8; /* UNIT_ID + PDU */
            pxMb->pucPayLoadBuffer[len++] = pdu_len;
        }
        pxMb->pucPayLoadBuffer[len++] = pxFrame->ucAddr;
        pxMb->pucPayLoadBuffer[len++] = ucFunc;
        if ((eType == eModbusPacketBase) || (eType == eModbusPacketFull)) {
            pxMb->pucPayLoadBuffer[len++] = pxFrame->usRegAddr >> 8;
            pxMb->pucPayLoadBuffer[len++] = pxFrame->usRegAddr;
            pxMb->pucPayLoadBuffer[len++] = pxFrame->usRegValueCount >> 8;
            pxMb->pucPayLoadBuffer[len++] = pxFrame->usRegValueCount;
        }
        if ((eType == eModbusPacketVariableLen) || (eType == eModbusPacketFull)) {
            pxMb->pucPayLoadBuffer[len++] = pxFrame->ucLengthCode;
            mem_cpy(&pxMb->pucPayLoadBuffer[len], pxFrame->pucData, pxFrame->ucLengthCode);
            len += pxFrame->ucLengthCode;
        }
        if (eType == eModbusPacketCode)
            pxMb->pucPayLoadBuffer[len++] = pxFrame->ucLengthCode;
        if(pxMb->eMode == MB_MODE_RTU) {
            uint16_t crc = _usCrc16(pxMb->pucPayLoadBuffer, len);
            pxMb->pucPayLoadBuffer[len++] = crc;
            pxMb->pucPayLoadBuffer[len++] = crc >> 8;
        }
    }
    pxMb->usFrameLen = len;
    return 1;
}

/* Parse binary frame fields using packet type derived from function code.
   bin     - pointer to [ADDR][FUNC][...] binary data
   bin_len - number of bytes available (excluding any checksum) */
static uint16_t _usParsePdu(_prModbus_t *pxMb, uint8_t *bin, uint16_t bin_len, uint8_t bIsRequest) {
    if (bin_len < 1) return 0;
    uint16_t offset = 0;
    pxMb->xFrame.ucFunc          = bin[offset++];
    pxMb->xFrame.ucLengthCode    = 0;
    pxMb->xFrame.usRegAddr       = 0;
    pxMb->xFrame.usRegValueCount = 0;
    pxMb->xFrame.pucData         = libNULL;
    ModbusPacketType_t eType = _eModbusFuncToPacketType(pxMb->xFrame.ucFunc, bIsRequest);
    if(eType == eModbusPacketNone) { /* unknown */
        /* todo: extern parser */
        return 0;
    }
    if((eType == eModbusPacketBase) || (eType == eModbusPacketFull)) {
        if (bin_len < (offset + 4)) return 0;
        pxMb->xFrame.usRegAddr        = ((uint16_t)bin[offset++]) << 8;
        pxMb->xFrame.usRegAddr       |= bin[offset++];
        pxMb->xFrame.usRegValueCount  = ((uint16_t)bin[offset++]) << 8;
        pxMb->xFrame.usRegValueCount |= bin[offset++];
    }
    if((eType == eModbusPacketVariableLen) || (eType == eModbusPacketFull)) {
        pxMb->xFrame.ucLengthCode = bin[offset++];
        pxMb->xFrame.pucData      = &bin[offset];
    }
    if(eType == eModbusPacketCode) {
        pxMb->xFrame.ucLengthCode = bin[offset++];
    }
    return offset;
}

static uint8_t _bParseRtu(_prModbus_t *pxMb) {
    uint16_t len = pxMb->usFrameLen;
    if (len < 4) return 0; /* ADDR + FUNC + CRC */
    uint16_t crc_calc = _usCrc16(pxMb->pucPayLoadBuffer, len - 2);
    uint16_t crc_recv = (uint16_t)pxMb->pucPayLoadBuffer[len - 2] | ((uint16_t)pxMb->pucPayLoadBuffer[len - 1] << 8);
    if (crc_calc != crc_recv) return 0;
    pxMb->xFrame.ucAddr = pxMb->pucPayLoadBuffer[0];
    return _usParsePdu(pxMb, &pxMb->pucPayLoadBuffer[1], len - 3, pxMb->bIsServer) > 0;
}

/* ASCII: data already decoded byte-by-byte in vModbusReceiveData.
   Buffer contains [ADDR][FUNC][...][LRC], usFrameLen = total binary bytes. */
static uint8_t _bParseAscii(_prModbus_t *pxMb) {
    uint16_t len = pxMb->usFrameLen;
    if (len < 3) return 0; /* ADDR + FUNC + LRC */    
    if (_ucLrc(pxMb->pucPayLoadBuffer, len)) 
        return 0;
    pxMb->xFrame.ucAddr = pxMb->pucPayLoadBuffer[0];
    return _usParsePdu(pxMb, &pxMb->pucPayLoadBuffer[1], len - 2, pxMb->bIsServer) > 0;
}

static uint8_t _bParseTcp(_prModbus_t *pxMb) {
    uint16_t len = pxMb->usFrameLen;
    if (len < 8) return 0; /* 6 MBAP + UNIT_ID + FUNC */
    uint16_t declared_len = ((uint16_t)pxMb->pucPayLoadBuffer[4] << 8) | pxMb->pucPayLoadBuffer[5]; /* UNIT_ID + PDU */
    if (declared_len < 2) return 0; /* at minimum UNIT_ID + FUNC */
    if (len < 6 + declared_len) return 0;
    uint16_t proto = ((uint16_t)pxMb->pucPayLoadBuffer[2] << 8) | pxMb->pucPayLoadBuffer[3];
    if (proto != 0) return 0;
    pxMb->usTransactionId = ((uint16_t)pxMb->pucPayLoadBuffer[0] << 8) | pxMb->pucPayLoadBuffer[1];
    pxMb->xFrame.ucAddr = pxMb->pucPayLoadBuffer[6]; /* buf[6] points to [UNIT_ID] */
    /* buf + 7 points to PDU [FUNC][...], (declared_len - 1) covers exactly this region */
    return _usParsePdu(pxMb, &pxMb->pucPayLoadBuffer[7], declared_len - 1, pxMb->bIsServer) > 0;
}

static void _server_handle_frame(_prModbus_t *pxMb) {
    if(pxMb->pfCallback)
      pxMb->pfCallback(&pxMb->xFrame, (Modbus_t*)pxMb, pxMb->pxCbContext);
    else {
        pxMb->xFrame.ucFunc |= MODBUS_ERROR_FLAG;
        pxMb->xFrame.ucLengthCode = ModbusIllegalFunction;
        usModbusResponse((Modbus_t*)pxMb, &pxMb->xFrame);
    }
}

static inline void _vRxReset(_prModbus_t *pxMb) {
    pxMb->usFrameLen = 0;
    pxMb->bAsciiHasNibble = 0;
}

uint8_t bModbusInit(Modbus_t *pxMb, const ModbusConfig_t *pxConfig) {
    if (!pxMb || !pxConfig || !pxConfig->pucPayLoadBuffer || pxConfig->ucPayLoadBufferSize == 0) return 0;
    if (!pxConfig->pxIface || !pxConfig->pxIface->pfWrite || !pxConfig->pxIface->pfTimer) return 0;
    _prModbus_t *mb = (_prModbus_t*)pxMb;
    mem_set(pxMb, 0, sizeof(*pxMb));
    mb->pfCallback = pxConfig->pfOnRequest,
    mb->pxCbContext = pxConfig->pxRequestContext,
    mb->pxIface = pxConfig->pxIface;
    mb->pucPayLoadBuffer = pxConfig->pucPayLoadBuffer;
    mb->ucPayLoadBufferSize = pxConfig->ucPayLoadBufferSize;
    mb->pxTxContext = pxConfig->pxTxContext;
    mb->pxTimerContext = pxConfig->pxTimerContext;
    mb->usTxTimeout = pxConfig->usTxTimeout;
    mb->usRxTimeout = pxConfig->usRxTimeout;
    mb->eMode = pxConfig->eMode;
    mb->bIsServer = pxConfig->bIsServer;
    mb->eState = pxConfig->bIsServer? MB_STATE_LISTENING: MB_STATE_IDLE;
    _vRxReset(mb);
    return 1;
}

uint8_t bModbusDeinit(Modbus_t *pxMb) {
    if (pxMb) mem_set(pxMb, 0, MODBUS_DESCRIPTOR_SIZE);
    return 1;
}

void vModbusReceiveData(Modbus_t *pxMb, const uint8_t *pucData, uint16_t usLen, uint8_t bEnd) {
    _prModbus_t *mb = (_prModbus_t*)pxMb;
    if (!mb) return;
    /* Block receive when not in a receivable state */
    if (mb->eState == MB_STATE_SENDING || mb->eState == MB_STATE_HANDLING) return;
    if (mb->eState == MB_STATE_RECEIVED) return;
    if (mb->eState == MB_STATE_IDLE) return;
    if (mb->eMode == MB_MODE_ASCII) {
        for (uint16_t i = 0; i < usLen; i++) {
            uint8_t c = pucData[i];
            if (c == ':') {
                _vRxReset(mb);
                mb->eState = MB_STATE_RECEIVING;
                continue;
            }
            if (mb->eState != MB_STATE_RECEIVING)
                continue;
            if (c == '\r' || c == '\n') {
                if (c == '\n' || bEnd)
                    mb->eState = MB_STATE_RECEIVED;
                continue;
            }
            if (mb->usFrameLen >= mb->ucPayLoadBufferSize) {
                mb->eState = MB_STATE_LISTENING;
                _vRxReset(mb);
                continue;
            }
            uint8_t nib;
            if      (c >= '0' && c <= '9') nib = (uint8_t)(c - '0');
            else if (c >= 'A' && c <= 'F') nib = (uint8_t)(c - 'A' + 10);
            else if (c >= 'a' && c <= 'f') nib = (uint8_t)(c - 'a' + 10);
            else {
                mb->eState = MB_STATE_LISTENING;
                _vRxReset(mb);
                continue;
            }
            if (mb->bAsciiHasNibble)
                mb->pucPayLoadBuffer[mb->usFrameLen++] |= nib;
            else
                mb->pucPayLoadBuffer[mb->usFrameLen] = nib << 4;
            mb->bAsciiHasNibble = !mb->bAsciiHasNibble;
        }
        if (bEnd && mb->usFrameLen > 0) mb->eState = MB_STATE_RECEIVED;
    } else {
        for (uint16_t i = 0; i < usLen && mb->usFrameLen < mb->ucPayLoadBufferSize; i++)
            mb->pucPayLoadBuffer[mb->usFrameLen++] = pucData[i];
        if (mb->usFrameLen > 0) mb->eState = MB_STATE_RECEIVING;
        if (bEnd) mb->eState = MB_STATE_RECEIVED;
    }
}

static void _vModbusServerWork(_prModbus_t *pxMb) {
    uint32_t now = _ulNow(pxMb);
    switch (pxMb->eState) {
        case MB_STATE_SENDING:
            {
                int32_t tx = pxMb->pxIface->pfWrite(pxMb->pxTxContext, &pxMb->pucPayLoadBuffer[pxMb->usTxCnt], pxMb->usFrameLen - pxMb->usTxCnt);
                if (tx >= 0) pxMb->usTxCnt += tx;
                if ((tx < 0) || CL_TIME_ELAPSED(pxMb->ulTimer, pxMb->usTxTimeout, now))
                    pxMb->eState = MB_STATE_LISTENING; /* send error: go back to listen */
                else if (pxMb->usTxCnt >= pxMb->usFrameLen) {
                    _vRxReset(pxMb);
                    pxMb->eState = MB_STATE_LISTENING;
                }
            }
            break;
        case MB_STATE_HANDLING:
            if (CL_TIME_ELAPSED(pxMb->ulTimer, pxMb->usTxTimeout, now)) {
                _vRxReset(pxMb);
                pxMb->eState = MB_STATE_LISTENING;
            }
            break;
        case MB_STATE_RECEIVED: {
                uint8_t ok = 0;
                if (pxMb->eMode == MB_MODE_TCP_UDP) ok = _bParseTcp(pxMb);
                else if (pxMb->eMode == MB_MODE_RTU) ok = _bParseRtu(pxMb);
                else if (pxMb->eMode == MB_MODE_ASCII) ok = _bParseAscii(pxMb);
                else if (pxMb->eMode == MB_MODE_PDU) ok = (_usParsePdu(pxMb, &pxMb->pucPayLoadBuffer[0], pxMb->usFrameLen, pxMb->bIsServer) > 0);
                _vRxReset(pxMb);
                if (ok) {
                    pxMb->eState = MB_STATE_HANDLING;
                    pxMb->ulTimer = now;
                    _server_handle_frame(pxMb);
                } else {
                    pxMb->eState = MB_STATE_LISTENING;
                }
            }
            break;
        default: break;
    }
}

static void _vModbusClientWork(_prModbus_t *pxMb) {
    uint32_t now = _ulNow(pxMb);
    switch (pxMb->eState) {
        case MB_STATE_IDLE:
            break;
        case MB_STATE_SENDING: {
                int32_t tx = pxMb->pxIface->pfWrite(pxMb->pxTxContext, &pxMb->pucPayLoadBuffer[pxMb->usTxCnt], pxMb->usFrameLen - pxMb->usTxCnt);
                if (tx >= 0) 
                    pxMb->usTxCnt += tx;
                if ((tx < 0) || CL_TIME_ELAPSED(pxMb->ulTimer, pxMb->usTxTimeout, now)) {
                    pxMb->xFrame.ucFunc |= MODBUS_ERROR_FLAG;
                    pxMb->xFrame.ucLengthCode = ModbusGatewayPathUnavailable;
                    if (pxMb->pfCallback) 
                        pxMb->pfCallback(&pxMb->xFrame, (Modbus_t*)pxMb, pxMb->pxCbContext);
                    pxMb->eState = MB_STATE_IDLE;
                } else if (pxMb->usTxCnt >= pxMb->usFrameLen) {
                    _vRxReset(pxMb);
                    pxMb->eState = MB_STATE_LISTENING;
                    pxMb->ulTimer = now;
                }
            }
            break;
        case MB_STATE_LISTENING:
        case MB_STATE_RECEIVING:
            if (CL_TIME_ELAPSED(pxMb->ulTimer, pxMb->usRxTimeout, now)) {
                pxMb->xFrame.ucFunc |= MODBUS_ERROR_FLAG;
                pxMb->xFrame.ucLengthCode = ModbusGatewayTargetDeviceFailedtoRespond;
                if (pxMb->pfCallback) pxMb->pfCallback(&pxMb->xFrame, (Modbus_t*)pxMb, pxMb->pxCbContext);
                pxMb->eState = MB_STATE_IDLE;
            }
            break;
        case MB_STATE_RECEIVED: {
                uint8_t ok = 0;
                if (pxMb->eMode == MB_MODE_TCP_UDP) {
                    ok = _bParseTcp(pxMb);
                    if (ok && pxMb->usTransactionId != pxMb->usExpectedTransId) {
                        _vRxReset(pxMb);
                        pxMb->eState = MB_STATE_LISTENING;
                        ok = 0;
                    }
                } 
                else if (pxMb->eMode == MB_MODE_RTU) ok = _bParseRtu(pxMb);
                else if (pxMb->eMode == MB_MODE_ASCII) ok = _bParseAscii(pxMb);
                else if (pxMb->eMode == MB_MODE_PDU) ok = (_usParsePdu(pxMb, &pxMb->pucPayLoadBuffer[0], pxMb->usFrameLen, pxMb->bIsServer) > 0);
                if (ok) {
                    if (pxMb->pfCallback) pxMb->pfCallback(&pxMb->xFrame, (Modbus_t*)pxMb, pxMb->pxCbContext);
                    pxMb->eState = MB_STATE_IDLE;
                } else if (pxMb->eState != MB_STATE_LISTENING) {
                    _vRxReset(pxMb);
                    pxMb->eState = MB_STATE_LISTENING;
                }
            }
            break;
        default: 
            break;
    }
}

void vModbusWork(Modbus_t *pxMb) {
    _prModbus_t *mb = (_prModbus_t*)pxMb;
    if (!mb) 
        return;
    if (mb->bIsServer)
        _vModbusServerWork(mb);
    else
        _vModbusClientWork(mb);
}

uint16_t usModbusRequest(Modbus_t *pxMb, ModbusFrame_t *pxFrame, ModbusCb_t pfCallback, void *pxCbContext, uint16_t ulTimeout) {
    static uint16_t transId = 1;
    _prModbus_t *mb = (_prModbus_t*)pxMb;
    if (!pxMb || mb->eState != MB_STATE_IDLE || mb->bIsServer)
        return 0;
    if (!_ucFrame2Raw(mb, pxFrame, 0))
        return 0;
    mb->xFrame = *pxFrame;
    mb->pfCallback = pfCallback;
    mb->pxCbContext = pxCbContext;
    mb->usRxTimeout = ulTimeout;
    uint16_t currentId = transId++;
    if (!transId) transId = 1;
    if (mb->eMode == MB_MODE_TCP_UDP) {
        mb->usTransactionId = (uint16_t)currentId;
        mb->usExpectedTransId = mb->usTransactionId;
    }
    mb->usTxCnt = 0;
    mb->eState = MB_STATE_SENDING;
    mb->ulTimer = _ulNow(mb);
    return currentId;
}

uint16_t usModbusResponse(Modbus_t *pxMb, ModbusFrame_t *pxFrame) {
    _prModbus_t *mb = (_prModbus_t*)pxMb;
    if (!mb || !mb->bIsServer || mb->eState != MB_STATE_HANDLING)
        return 0;
    if (!_ucFrame2Raw(mb, pxFrame, 1))
        return 0;
    mb->usTxCnt = 0;
    mb->eState = MB_STATE_SENDING;
    mb->ulTimer = _ulNow(mb);
    return (mb->eMode == MB_MODE_TCP_UDP) ? mb->usTransactionId : 1;
}

uint8_t bModbusReset(Modbus_t *pxMb) {
    _prModbus_t *mb = (_prModbus_t*)pxMb;
    if (!mb) return 0;
    if (mb->bIsServer)
        mb->eState = MB_STATE_LISTENING;
    else
        mb->eState = MB_STATE_IDLE;
    _vRxReset(mb);
    return 1;
}

uint8_t bModbusBusy(Modbus_t *pxMb) {
    _prModbus_t *mb = (_prModbus_t*)pxMb;
    return !((mb->bIsServer && (mb->eState == MB_STATE_LISTENING)) || (!mb->bIsServer && (mb->eState == MB_STATE_IDLE)));
}

uint8_t *pucModbusExtructFrameData(ModbusFrame_t *pxFrame, uint8_t *pucOutCode, uint8_t *pucOutAmount, uint8_t *pucOutSize) {
    *pucOutAmount = 0;
    *pucOutSize = 0;
    *pucOutCode = 0;
    if (!pxFrame) return libNULL;
    uint8_t func = pxFrame->ucFunc & ~MODBUS_ERROR_FLAG;
    switch (func) {
        case 0x01: case 0x02: *pucOutSize = 1; break;
        default: *pucOutSize = 2; break;
    }
    if (pxFrame->ucFunc & MODBUS_ERROR_FLAG) {
        *pucOutCode = pxFrame->ucLengthCode;
        return libNULL;
    }
    if (func == 0x03 || func == 0x04) {
        *pucOutAmount = pxFrame->ucLengthCode / 2;
        return pxFrame->pucData;
    } else if (func == 0x01 || func == 0x02) {
        *pucOutAmount = pxFrame->ucLengthCode * 8;
        return pxFrame->pucData;
    } else if (func == 0x05 || func == 0x06) {
        *pucOutAmount = 1;
        return (uint8_t*)&pxFrame->usRegValueCount;
    }
    return libNULL;
}

ModbusState_t vModbusGetState(Modbus_t *pxMb) {
    _prModbus_t *mb = (_prModbus_t*)pxMb;
    if (!mb) return MB_STATE_UNDEF;
    return mb->eState;
}

/* snake */
uint8_t modbus_init(modbus_t *, const modbus_config_t *) __attribute__ ((alias ("bModbusInit")));
uint8_t modbus_deinit(modbus_t *) __attribute__ ((alias ("bModbusDeinit")));
void modbus_work(modbus_t *) __attribute__ ((alias ("vModbusWork")));
uint16_t modbus_request(modbus_t *, modbus_frame_t *, modbus_cb_t, void *, uint16_t) __attribute__ ((alias ("usModbusRequest")));
uint16_t modbus_response(modbus_t *, modbus_frame_t *) __attribute__ ((alias ("usModbusResponse")));
uint8_t modbus_reset(modbus_t *) __attribute__ ((alias ("bModbusReset")));
uint8_t modbus_busy(modbus_t *) __attribute__ ((alias ("bModbusBusy")));
void modbus_receive_data(modbus_t *, const uint8_t *, uint16_t, uint8_t) __attribute__ ((alias ("vModbusReceiveData")));
ModbusState_t modbus_get_state(modbus_t *) __attribute__ ((alias ("vModbusGetState")));

uint8_t *modbus_extruct_frame_data(modbus_frame_t *, uint8_t *, uint8_t *, uint8_t *) __attribute__ ((alias ("pucModbusExtructFrameData")));
