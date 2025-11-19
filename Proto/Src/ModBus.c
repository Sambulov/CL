#include "CodeLib.h"

/* eRxPacketType/eTxPacketType definitions. */
typedef enum {
    eModbusPacketNone        = 0,    /* Undefined */
    eModbusPacketBase        = 1,    /* [ADDR][FUNC][MSB_REG_ADDR][LSB_REG_ADDR][MSB_COUNT][LSB_COUNT][MSB_CRC][LSB_CRC] */
    eModbusPacketVariableLen = 2,    /* [ADDR][FUNC][LENGTH][DATA...][MSB_CRC][LSB_CRC] */
    eModbusPacketFull        = 3,    /* [ADDR][FUNC][MSB_REG_ADDR][LSB_REG_ADDR][MSB_COUNT][LSB_COUNT][LENGTH][DATA...][MSB_CRC][LSB_CRC] */
    eModbusPacketCode        = 4,    /* [ADDR][FUNC][CODE][MSB_CRC][LSB_CRC] */
    eModbusPacketCall        = 8,    /* [ADDR][FUNC][MSB_CRC][LSB_CRC] */
    eModbusPacketTypesAmount
} ModbusPacketType_t;

typedef struct {
    const ModbusIface_t *pxIface;
	ModbusFrame_t xFrame;
    uint16_t usTimer;
    uint16_t rx_timeout;
    uint16_t tx_timeout;
    uint16_t usTmpBuffer;
    uint16_t ucCrc;
    uint8_t eTxType; /* ModbusPacketType_t */
    uint8_t eRxType; /* ModbusPacketType_t */
    uint8_t ucBlockCounter;
	int8_t cXferState;
    uint8_t bProcessing      :1,
            bTransmitPhase   :1,
            bRequestComplete :1,
            bPduMode         :1,
            bBlockComplete   :1,
            bAsciiIncomplete :1,
            bAsciiMode       :1,
            bSkeepFrame      :1;
	uint8_t ucPayLoadBufferSize;
    uint32_t req_id;
	uint8_t *pucPayLoadBuffer;
    ModbusCb_t pfOnComplete;
    void *pxTxContext;
	void *pxRxContext;
	void *pxTimerContext;
 	void *pxCbContext;
    const ModbusEndpoint_t *pxEndpoints;
} _prModbus_t;

LIB_ASSERRT_STRUCTURE_CAST(_prModbus_t, Modbus_t, MODBUS_DESCRIPTOR_SIZE, "ModBus.h");

#define bModbusServerMode(pxMb)    (pxMb->pxEndpoints != libNULL)

static void _eModbusDummyHandler(Modbus_t *pxMb, void *pxContext, ModbusFrame_t *pxFrame) {
    (void)pxMb; (void)pxContext;
    pxFrame->ucFunc |= MODBUS_ERROR_FLAG;
    pxFrame->ucLengthCode = ModbusIllegalFunction;
}

static ModbusPacketType_t _eModbusFuncToPacketType(uint8_t ucFunc, uint8_t bIsRequest) {
    if(ucFunc & MODBUS_ERROR_FLAG) return eModbusPacketCode;
	switch (ucFunc) {
	case 0x01 /* Read coil */:
		return bIsRequest? eModbusPacketBase: eModbusPacketVariableLen;
	case 0x02 /* Read input */:
		return bIsRequest? eModbusPacketBase: eModbusPacketVariableLen;
	case 0x03 /* Read holding registers */:
		return bIsRequest? eModbusPacketBase: eModbusPacketVariableLen;
	case 0x04 /* Read input registers */:
		return bIsRequest? eModbusPacketBase: eModbusPacketVariableLen;
	case 0x05 /* Write single coil */:
		return eModbusPacketBase;
	case 0x06 /* Write single register */:
		return eModbusPacketBase;
	case 0x07 /* Read Exception Status */:
		return bIsRequest? eModbusPacketCall: eModbusPacketCode;
	//	case 0x08 /* Diagnostic */:
	//		return ;
	case 0x0B /* Get Com Event Counter */:
        return bIsRequest? eModbusPacketCall: eModbusPacketBase;
	case 0x0C /* Get Com Event Log */:
        return bIsRequest? eModbusPacketCall: eModbusPacketVariableLen;
	case 0x0F /* Write multiple coils */:
		return bIsRequest? eModbusPacketFull: eModbusPacketBase;
	case 0x10 /* Write multiple registers */:
		return bIsRequest? eModbusPacketFull: eModbusPacketBase;
    case 0x11 /* Report Slave ID */:
        return bIsRequest? eModbusPacketCall: eModbusPacketVariableLen;
    case 0x14 /* Read File Record */:
        return bIsRequest? eModbusPacketVariableLen: eModbusPacketVariableLen;
    case 0x15 /* Write File Record */:
        return bIsRequest? eModbusPacketVariableLen: eModbusPacketVariableLen;
    //	case 0x16 /* Mask Write Register */:
    //		return ;
    //	case 0x18 /* Read FIFO Queue */:
    //		return ;
    //	case 0x2B /* Encapsulated Interface Transport */:
    //		return ;
	default:
		return eModbusPacketNone;
	}
}

static uint8_t _bModbusServerSetHandler(_prModbus_t *pxMb) {
	pxMb->pfOnComplete = &_eModbusDummyHandler;
    const ModbusEndpoint_t *currentEP = pxMb->pxEndpoints;
    while ((currentEP != libNULL) && (currentEP->ucAddress != (pxMb->xFrame.ucAddr & currentEP->ucAddressMask)))
        currentEP = currentEP->pxNext;
    if (currentEP == libNULL) return CL_FALSE;
    const ModbusHandler_t *hdesc = libNULL;
    for (int32_t i = 0; currentEP->paxHandlers[i] != libNULL; i++) {
        if (currentEP->paxHandlers[i]->ucFunctionNo == pxMb->xFrame.ucFunc) {
            hdesc = currentEP->paxHandlers[i];
            break;
        }
    }
    if (hdesc != libNULL) {
        pxMb->pfOnComplete = hdesc->pfOnRequest;
        pxMb->pxCbContext = hdesc->pxContext;
    }
    return CL_TRUE;
}

static uint16_t _usModbusDummyCrc(const uint8_t *pucData, uint16_t ulLen, const uint16_t *pusCrc) {
    (void)pucData; (void)ulLen; (void)pusCrc;
    return 0;
}

static uint16_t _usCrc16ModbusAscii(const uint8_t *pucData, uint16_t ulLen, const uint16_t *pusCrc) {
    uint8_t val = (pusCrc != libNULL)? *pusCrc: 0;
	while (ulLen--) val -= *pucData++;
	return val;
}

static inline int32_t _lRead(_prModbus_t *pxMb, uint8_t *pucData, uint16_t usSize) {
    return pxMb->pxIface->pfRead(pxMb->pxRxContext, pucData, usSize);
}

static inline int32_t _lWrite(_prModbus_t *pxMb, uint8_t *pucData, uint16_t usSize) {
    return pxMb->pxIface->pfWrite(pxMb->pxTxContext, pucData, usSize);
}

static int32_t _bReceiveAscii(_prModbus_t *pxMb, uint8_t *out) {
    int32_t len = 2 - pxMb->bAsciiIncomplete;
    int32_t res = _lRead(pxMb, ((uint8_t *)&pxMb->usTmpBuffer) + pxMb->bAsciiIncomplete, len);
    if(res <= 0) return CL_FALSE;
    if(res == len) {
        pxMb->bAsciiIncomplete = 0;
        if(!bConvertAsciiHexToByte(swap_bytes(pxMb->usTmpBuffer), out)) return CL_FALSE;
        return CL_TRUE;
    }
    pxMb->bAsciiIncomplete = 1;
    return CL_FALSE;
}

static int32_t _bTransmitAscii(_prModbus_t *pxMb, uint8_t ucData) {
    if(!pxMb->bAsciiIncomplete) {
        pxMb->usTmpBuffer = swap_bytes(usConvertByteToAsciiHex(ucData, CL_TRUE));
        pxMb->bAsciiIncomplete = 1;
    }
    uint16_t len = (pxMb->usTmpBuffer & 0xff00)? 2: 1;
    int32_t res = _lWrite(pxMb, (uint8_t *)&pxMb->usTmpBuffer, len);
    if(res > 0) pxMb->usTmpBuffer >>= 8 * res;
    if(!pxMb->usTmpBuffer) pxMb->bAsciiIncomplete = 0;
    return !pxMb->bAsciiIncomplete;
}

static int32_t _lReadAscii(_prModbus_t *pxMb, uint8_t *pucData, uint16_t usSize) {
    int32_t count = 0;
    while (usSize-- && _bReceiveAscii(pxMb, pucData++)) count++;
    return count;
}

static int32_t _lWriteAscii(_prModbus_t *pxMb, uint8_t *pucData, uint16_t usSize) {
    int32_t count = 0;
    while (usSize-- && _bTransmitAscii(pxMb, *pucData++)) count++;
    return count;
}

static uint8_t _bTransferBlock(_prModbus_t *pxMb, uint8_t *pucData, uint16_t usSize, int32_t (fWriter)(_prModbus_t *, uint8_t *, uint16_t)) {
    if(pxMb->bBlockComplete) {
        pxMb->bBlockComplete = 0;
        pxMb->ucBlockCounter = 0;
    }
    int32_t res = fWriter(pxMb, pucData + pxMb->ucBlockCounter, usSize - pxMb->ucBlockCounter);
    if(res > 0) {
        pxMb->ucBlockCounter += res; 
        if(pxMb->ucBlockCounter == usSize) 
            pxMb->bBlockComplete = 1;
    }
    return pxMb->bBlockComplete;
}

static inline uint8_t _bReadBlock(_prModbus_t *pxMb, uint8_t *pucData, uint16_t usSize) {
    return _bTransferBlock(pxMb, pucData, usSize, &_lRead);
}

static inline uint8_t _bReadBlockAscii(_prModbus_t *pxMb, uint8_t *pucData, uint16_t usSize) {
    return _bTransferBlock(pxMb, pucData, usSize, &_lReadAscii);
}

static inline uint8_t _bWriteBlock(_prModbus_t *pxMb, uint8_t *pucData, uint16_t usSize) {
    return _bTransferBlock(pxMb, pucData, usSize, &_lWrite);
}

static inline uint8_t _bWriteBlockAscii(_prModbus_t *pxMb, uint8_t *pucData, uint16_t usSize) {
    return _bTransferBlock(pxMb, pucData, usSize, &_lWriteAscii);
}

static void _TxFrame(_prModbus_t *pxMb) {
	if (pxMb->cXferState < 0) pxMb->cXferState = 0;
	if (pxMb->eTxType == eModbusPacketNone) {
        pxMb->cXferState = -1;
        return;
    }
    int32_t (*lDataWrite)(_prModbus_t *, uint8_t *, uint16_t) = &_lWrite;
    if(pxMb->bAsciiMode) lDataWrite = &_lWriteAscii;
    uint8_t (*bBlockWrite)(_prModbus_t *, uint8_t *, uint16_t) = &_bWriteBlock;
    if(pxMb->bAsciiMode) bBlockWrite = &_bWriteBlockAscii;
    uint16_t (*usCrcCalc)(const uint8_t *, uint16_t, const uint16_t *) = &usCrc16ModbusRtu;
    if(pxMb->bPduMode) usCrcCalc = &_usModbusDummyCrc;
    else if(pxMb->bAsciiMode) usCrcCalc = &_usCrc16ModbusAscii;
    ModbusFrame_t *frame = &pxMb->xFrame;
    switch (pxMb->cXferState) {
    case 0:
    case 1 /* Tx frame start */:
        pxMb->cXferState = 1;
        if ((pxMb->bAsciiMode) && (_lWrite(pxMb, (uint8_t *)":", 1) <= 0))
            break;
        pxMb->bBlockComplete = 1;
        pxMb->bAsciiIncomplete = 0;
    /* fall through */
    case 2 /* Tx addr */:
        pxMb->cXferState = 2;
        if(!pxMb->bPduMode) {
            if(lDataWrite(pxMb, &frame->ucAddr, 1) <= 0)
                break;
            pxMb->ucCrc = usCrcCalc(&frame->ucAddr, 1, libNULL);
        }
        /* fall through */
    case 3 /* Tx func */:
        pxMb->cXferState = 3;
        if (lDataWrite(pxMb, &frame->ucFunc, 1) <= 0 )
            break;
        pxMb->ucCrc = usCrcCalc(&frame->ucFunc, 1, &pxMb->ucCrc);
        /* fall through */
    case 4 /* Tx Reg addr */:
        pxMb->cXferState = 4;
        if (!(frame->ucFunc & MODBUS_ERROR_FLAG) && (pxMb->eTxType & eModbusPacketBase)) {
            uint16_t regAddr = swap_bytes(frame->usRegAddr);
            if (!bBlockWrite(pxMb, (uint8_t *)&regAddr, 2))
                break;
            pxMb->ucCrc = usCrcCalc((uint8_t *)&regAddr, 2, &pxMb->ucCrc);
        } /* fall through */
    case 5 /* Tx Reg count */:
        pxMb->cXferState = 5;
        if (!(frame->ucFunc & MODBUS_ERROR_FLAG) && (pxMb->eTxType & eModbusPacketBase)) {
            uint16_t regValCount = swap_bytes(frame->usRegValueCount);
            if (!bBlockWrite(pxMb, (uint8_t *)&regValCount, 2))
                break;
            pxMb->ucCrc = usCrcCalc((uint8_t *)&regValCount, 2, &pxMb->ucCrc);
        } /* fall through */
    case 6 /* Tx length */:
        pxMb->cXferState = 6;
        if (pxMb->eTxType & (eModbusPacketVariableLen | eModbusPacketCode)) {
            if (lDataWrite(pxMb, &frame->ucLengthCode, 1) <= 0)
                break;
            pxMb->ucCrc = usCrcCalc(&frame->ucLengthCode, 1, &pxMb->ucCrc);
        } /* fall through */
    case 7 /* Tx data */:
        pxMb->cXferState = 7;
        if (pxMb->eTxType & eModbusPacketVariableLen) {
            if(!bBlockWrite(pxMb, frame->pucData, frame->ucLengthCode))
                break;
            pxMb->ucCrc = usCrcCalc(frame->pucData, frame->ucLengthCode, &pxMb->ucCrc);			
        } /* fall through */
    case 8 /* Tx crc */: 
        pxMb->cXferState = 8;
        if (pxMb->bAsciiMode) {
            uint8_t crc = pxMb->ucCrc;
            if (lDataWrite(pxMb, &crc, 1) <= 0)
                break;
        }
        else if(!pxMb->bPduMode && !bBlockWrite(pxMb, (uint8_t *)&pxMb->ucCrc, 2))
            break;
        /* fall through */
    case 9: /* Tx frame terminator */
        pxMb->cXferState = 9;
        if (pxMb->bAsciiMode && (!_bWriteBlock(pxMb, (uint8_t *)"\r\n", 2)))
            break;
        /* fall through */
    default:
        pxMb->cXferState = 0;
        break;
    }
}

static void _RxFrame(_prModbus_t *pxMb) {
    int32_t (*lDataRead)(_prModbus_t *, uint8_t *, uint16_t) = &_lRead;
    if(pxMb->bAsciiMode) lDataRead = &_lReadAscii;
    uint8_t (*bBlockRead)(_prModbus_t *, uint8_t *, uint16_t) = &_bReadBlock;
    if(pxMb->bAsciiMode) bBlockRead = &_bReadBlockAscii;
    uint16_t (*usCrcCalc)(const uint8_t *, uint16_t, const uint16_t *) = &usCrc16ModbusRtu;
    if(pxMb->bPduMode) usCrcCalc = &_usModbusDummyCrc;
    else if (pxMb->bAsciiMode) usCrcCalc = &_usCrc16ModbusAscii;
    if (pxMb->cXferState <= 0) {
        pxMb->cXferState = 1;
        pxMb->usTimer = pxMb->pxIface->pfTimer(pxMb->pxTimerContext);
    }
    ModbusFrame_t *frame = &pxMb->xFrame;
    uint8_t *tmpBuffer = (uint8_t *)&pxMb->usTmpBuffer;
    switch (pxMb->cXferState) {
        case 1 /* Rx frame start */:
            if ((pxMb->bAsciiMode) && ((_lRead(pxMb, tmpBuffer, 1) <= 0) || (*tmpBuffer != ':')))
                break;
            pxMb->bSkeepFrame = 0;
            pxMb->bBlockComplete = 1;
            pxMb->bAsciiIncomplete = 0;
            frame->pucData = pxMb->pucPayLoadBuffer;
            frame->ucBufferSize = pxMb->ucPayLoadBufferSize;
            /* fall through */
        case 2 /* Rx addr */:
            if(!pxMb->bPduMode) {
                pxMb->cXferState = ((pxMb->bAsciiMode)? 2: 1);
                if (lDataRead(pxMb, tmpBuffer, 1) <= 0)
                    break;
                frame->ucAddr = *tmpBuffer;
                pxMb->ucCrc = usCrcCalc(tmpBuffer, 1, libNULL);
            }
            /* fall through */
        case 3 /* Rx func */:
            pxMb->cXferState = 3;
            if (lDataRead(pxMb, tmpBuffer, 1) <= 0)
                break;
            frame->ucFunc = *tmpBuffer;
            pxMb->ucCrc = usCrcCalc(tmpBuffer, 1, &pxMb->ucCrc);
            pxMb->eRxType = _eModbusFuncToPacketType(frame->ucFunc, bModbusServerMode(pxMb));
            /* fall through */
        case 4 /* Rx Reg addr */:
            pxMb->cXferState = 4;
            if (pxMb->eRxType & eModbusPacketBase) {
                if (!bBlockRead(pxMb, (uint8_t *)&frame->usRegAddr, 2))
                    break;
                pxMb->ucCrc = usCrcCalc((uint8_t *)&frame->usRegAddr, 2, &pxMb->ucCrc);
                frame->usRegAddr = swap_bytes(frame->usRegAddr);
            } /* fall through */
        case 5 /* Rx Reg value count */:
            pxMb->cXferState = 5;
            if (pxMb->eRxType & eModbusPacketBase) {
                if (!bBlockRead(pxMb, (uint8_t *)&frame->usRegValueCount, 2))
                    break;
                pxMb->ucCrc = usCrcCalc((uint8_t *)&frame->usRegValueCount, 2, &pxMb->ucCrc);
                frame->usRegValueCount = swap_bytes(frame->usRegValueCount);
            } /* fall through */
        case 6 /* Rx length | err */:
            pxMb->cXferState = 6;
            if ((pxMb->eRxType & eModbusPacketVariableLen) || (pxMb->eRxType == eModbusPacketCode)) {
                if (lDataRead(pxMb, &frame->ucLengthCode, 1) <= 0)
                    break;
                pxMb->ucCrc = usCrcCalc(&frame->ucLengthCode, 1, &pxMb->ucCrc);
            } /* fall through */
        case 7:
            if ((pxMb->eRxType & eModbusPacketVariableLen) && 
                ((frame->ucLengthCode > frame->ucBufferSize) || (frame->ucLengthCode == 0))) {
                frame->ucLengthCode = 0;
                pxMb->cXferState = ((pxMb->bAsciiMode || pxMb->bPduMode)? -1: 12);
                break;
            } /* fall through */
        case 8 /* Rx data */:
            pxMb->cXferState = 8;
            if (pxMb->eRxType & eModbusPacketVariableLen) {
                if(!bBlockRead(pxMb, frame->pucData, frame->ucLengthCode))
                    break;
                pxMb->ucCrc = usCrcCalc(frame->pucData, frame->ucLengthCode, &pxMb->ucCrc);			
            } /* fall through */
        case 9 /* Rx unknown */:
            if (pxMb->eRxType == eModbusPacketNone) {
                pxMb->cXferState = -1;
                if (!(pxMb->bAsciiMode || pxMb->bPduMode)) {
                    frame->ucLengthCode = 0;
                    pxMb->cXferState = 12;
                }
                break;
            } /* fall through */
        case 10 /* Rx crc */:
            if(pxMb->bAsciiMode) {
                uint8_t crc;
                if(lDataRead(pxMb, &crc, 1) <= 0) {
                    pxMb->cXferState = 10;
                    break;
                }
                if (((uint8_t)pxMb->ucCrc) != crc) {
                    pxMb->cXferState = -1;
                    break;
                }
            }
            else if(pxMb->bPduMode) {
                pxMb->cXferState = 0;
                break;
            }
            else {
                if (!bBlockRead(pxMb, (uint8_t *)&pxMb->usTmpBuffer, 2)) {
                    pxMb->cXferState = 10;
                    break;
                }
                pxMb->cXferState = (pxMb->ucCrc == pxMb->usTmpBuffer)? 0: -1;
                break;
            } /* fall through */
        case 11:
            if (pxMb->bAsciiMode) {
                uint8_t *buf = (uint8_t *)&pxMb->usTmpBuffer;
                if(!_bReadBlock(pxMb, buf, 2)) {
                    pxMb->cXferState = 11;
                    break;
                }
                pxMb->cXferState = (((buf[0] == '\r') && (buf[1] == '\n'))? 0: -1);
                break;
            } /* fall through */
        case 12:
        default: /* Will try find end of frame. Break if crc match or too many data received, or timeout occurred */
            while (_lRead(pxMb, &frame->pucData[1], 1) > 0) {
                if (frame->ucLengthCode > 0) {
                    uint16_t crc = (((uint16_t)frame->pucData[0]) << 8) | (frame->pucData[1]);
                    if ((crc == pxMb->ucCrc) || (frame->ucLengthCode >= 251)) {
                        pxMb->cXferState = -1;
                        break;
                    }
                    pxMb->ucCrc = usCrc16ModbusRtu(frame->pucData, 1, &pxMb->ucCrc);
                }
                frame->ucLengthCode++;
                frame->pucData[0] = frame->pucData[1];
            }
            pxMb->cXferState = 12;
            break;
    }
}

static void _vModbusServerWork(_prModbus_t *pxMb) {
    uint16_t now = pxMb->pxIface->pfTimer(pxMb->pxTimerContext);
    if (!pxMb->bTransmitPhase) { /* Awaite request frame */
        if(pxMb->bRequestComplete) {
            if (((uint16_t)(now - pxMb->usTimer) < pxMb->tx_timeout)) return;
            pxMb->bRequestComplete = 0;
        }
        _RxFrame(pxMb);
        if ((uint16_t)(now - pxMb->usTimer) >= pxMb->rx_timeout) pxMb->cXferState = 0;
        if (pxMb->cXferState == 0) { /* Frame received */
            if(_bModbusServerSetHandler(pxMb)) {
                pxMb->bRequestComplete = 1;
                pxMb->pfOnComplete((Modbus_t *)pxMb, pxMb->pxCbContext, &pxMb->xFrame);
            }
        }
    }
    else {
        _TxFrame(pxMb);
        if ((pxMb->cXferState <= 0) || ((uint16_t)(now - pxMb->usTimer) >= pxMb->tx_timeout)) pxMb->bTransmitPhase = 0;
    }
}

static void _vModbusClientWork(_prModbus_t *pxMb) {
    uint16_t now = pxMb->pxIface->pfTimer(pxMb->pxTimerContext);
    if (pxMb->bTransmitPhase) {
        if(pxMb->cXferState <= 0) pxMb->usTimer = now;
        _TxFrame(pxMb);
        if(pxMb->cXferState <= 0) {
            pxMb->bTransmitPhase = 0;
            pxMb->bRequestComplete = (pxMb->eRxType == eModbusPacketNone) || (pxMb->cXferState < 0);
            pxMb->usTimer = now;
        }
    }
    else { /* Awaite response */
        _RxFrame(pxMb);
        pxMb->bRequestComplete = (pxMb->cXferState <= 0) && !(pxMb->bSkeepFrame);
    }
    uint8_t bTimeout = ((uint16_t)(now - pxMb->usTimer) >= pxMb->tx_timeout);
    if(pxMb->bRequestComplete || bTimeout) {
        if (pxMb->pfOnComplete != libNULL) {
            ModbusErrorCode_t err = 0;
            if(pxMb->bTransmitPhase) {
                if(bTimeout || (pxMb->cXferState < 0)) err = ModbusGatewayPathUnavailable;
            }
            else { /* receive failed */
                if(bTimeout || (pxMb->cXferState < 0)) err = ModbusGatewayTargetDeviceFailedtoRespond;
            }
            if(err) {
                pxMb->xFrame.ucFunc |= MODBUS_ERROR_FLAG;
                pxMb->xFrame.ucLengthCode = err;
            }
            pxMb->pfOnComplete((Modbus_t *)pxMb, pxMb->pxCbContext, &pxMb->xFrame);
            pxMb->cXferState = -1;
        }
        pxMb->bProcessing = 0;
    }
}

void vModbusWork(Modbus_t *pxMb) {
    _prModbus_t *mb = (_prModbus_t *)pxMb;
    if((mb == libNULL) || (!mb->bProcessing)) return;
    if(mb->pxEndpoints != libNULL) _vModbusServerWork(mb);
    else _vModbusClientWork(mb);
}

static void _vModbusReset(_prModbus_t *pxMb) {
    pxMb->bProcessing = 0;
    pxMb->pxEndpoints = libNULL;
    pxMb->bAsciiIncomplete = 0;
    pxMb->bBlockComplete = 1;
    pxMb->bTransmitPhase = 0;
    pxMb->cXferState = 0;
    pxMb->bRequestComplete = 0;
}

uint8_t bModbusServerLinkEndpoints(Modbus_t *pxMb, const ModbusEndpoint_t *pxMbEp) {
    _prModbus_t *mb = (_prModbus_t*)pxMb;
    if(mb == libNULL) return CL_FALSE;
    _vModbusReset(mb);
    uint8_t check = 1;
    const ModbusEndpoint_t *current = pxMbEp;
    while (current != libNULL) {
        for (uint8_t i = 0; pxMbEp->paxHandlers[i] != libNULL; i++)
            check = check && (pxMbEp->paxHandlers[i]->pfOnRequest != libNULL);
        current = current->pxNext;
    }
    if (check) {
        mb->pxEndpoints = pxMbEp;
        mb->bProcessing = (pxMbEp != libNULL);
    }
    return check;
}

uint8_t bModbusInit(Modbus_t *pxMb, const ModbusConfig_t *pxConfig) {
	if ((pxMb == libNULL) || 
        (pxConfig == libNULL) || 
        (pxConfig->pucPayLoadBuffer == libNULL) || 
        (pxConfig->ucPayLoadBufferSize == 0)) return CL_FALSE;
    _prModbus_t *mb = (_prModbus_t*)pxMb;
    mb->pxIface = pxConfig->pxIface;
    mb->ucPayLoadBufferSize = pxConfig->ucPayLoadBufferSize;
    mb->pucPayLoadBuffer = pxConfig->pucPayLoadBuffer;
    mb->pxTimerContext = pxConfig->pxTimerContext;
    mb->pxTxContext = pxConfig->pxTxContext;
    mb->pxRxContext = pxConfig->pxRxContext;
    mb->rx_timeout = pxConfig->rx_timeout;
    mb->tx_timeout = pxConfig->tx_timeout;
    mb->bAsciiMode = pxConfig->bAsciiMode;
    mb->bPduMode = pxConfig->bPduMode && !mb->bAsciiMode;
    _vModbusReset(mb);
    return CL_TRUE;
}

uint8_t bModbusDeinit(Modbus_t *pxMb) {
	if (pxMb == libNULL) return CL_TRUE;
    _prModbus_t *mb = (_prModbus_t*)pxMb;
    _vModbusReset(mb);
    return CL_TRUE;
}

uint32_t ulModbusRequest(Modbus_t *pxMb, ModbusFrame_t *pxFrame, ModbusCb_t pfCallback, void *pxCbContext) {
    _prModbus_t *mb = (_prModbus_t *)pxMb;
	if ((pxMb == libNULL) || (mb->bProcessing) || /* server mode always in processing state */
        (pxFrame == libNULL)) return 0;
    mb->eTxType = _eModbusFuncToPacketType(pxFrame->ucFunc, CL_TRUE);
    mb->eRxType = _eModbusFuncToPacketType(pxFrame->ucFunc, CL_FALSE);
    if((mb->eTxType == eModbusPacketNone) ||
       ((mb->eTxType & eModbusPacketVariableLen) && 
         ((pxFrame->ucLengthCode == 0) || (pxFrame->pucData == libNULL)))) 
        return 0;
    mb->xFrame = *pxFrame;
    mb->pxCbContext = pxCbContext;
    mb->pfOnComplete = pfCallback;
    if(mb->eTxType & eModbusPacketVariableLen) {
        mb->xFrame.pucData = mb->pucPayLoadBuffer;
        mem_cpy(mb->xFrame.pucData, pxFrame->pucData, pxFrame->ucLengthCode);
    }
    uint8_t dummy[16];
    while(mb->pxIface->pfRead(mb->pxRxContext, dummy, 16) > 0); /* flush input */
    mb->cXferState = 0;
    mb->bTransmitPhase = 1;
    mb->bProcessing = 1;
    mb->bRequestComplete = 0;
    mb->req_id++;
    if(!mb->req_id) mb->req_id = 1;
    return mb->req_id;
}

uint32_t ulModbusResponse(Modbus_t *pxMb, ModbusFrame_t *pxFrame) {
    _prModbus_t *mb = (_prModbus_t *)pxMb;
	if ((pxMb == libNULL) || (mb->pxEndpoints == libNULL) || /* not server mode */
        (pxFrame == libNULL) || !mb->bRequestComplete) return 0;
    mb->eTxType = _eModbusFuncToPacketType(pxFrame->ucFunc, CL_FALSE);
    mb->eRxType = eModbusPacketNone;
    if((mb->eTxType == eModbusPacketNone) || ((mb->eTxType & eModbusPacketVariableLen) && 
      ((pxFrame->ucLengthCode == 0) || (pxFrame->pucData == libNULL)))) 
        return 0;
    mb->xFrame = *pxFrame;
    if(mb->eTxType & eModbusPacketVariableLen) {
        mb->xFrame.pucData = mb->pucPayLoadBuffer;
        mem_cpy(mb->xFrame.pucData, pxFrame->pucData, pxFrame->ucLengthCode);
    }
    mb->cXferState = 0;
    mb->bTransmitPhase = 1;
    mb->bProcessing = 1;
    mb->bRequestComplete = 0;
    mb->req_id++;
    if(!mb->req_id) mb->req_id = 1;
    return mb->req_id;
}

uint8_t bModbusCancelRequest(Modbus_t *pxMb, uint32_t ulRequestId) {
    _prModbus_t *mb = (_prModbus_t *)pxMb;
	if ((pxMb == libNULL) || (mb->req_id != ulRequestId) || (mb->pxEndpoints != libNULL)/* server mode */) 
        return CL_FALSE;
    mb->bProcessing = 0;
    return CL_TRUE;
}

uint8_t bModbusBusy(Modbus_t *pxMb) {
    _prModbus_t *mb = (_prModbus_t *)pxMb;
	return (pxMb != libNULL) && mb->bProcessing;
}

uint8_t *pucModbusResponseFrameData(ModbusFrame_t *pxFrame, uint8_t *pucCode, uint8_t *pucOutAmount, uint8_t *pucOutSize) {
	*pucOutAmount = 0;
	*pucOutSize = 0;
	if(pxFrame == libNULL) return libNULL;
    ModbusPacketType_t type = _eModbusFuncToPacketType(pxFrame->ucFunc, CL_FALSE);
    switch (pxFrame->ucFunc & ~MODBUS_ERROR_FLAG) {
        case MB_FUNC_READ_OUTPUTS:
        case MB_FUNC_READ_INPUTS:
        *pucOutSize = 1;
        break;
    default:
        *pucOutSize = 2;
        break;
    }
    *pucCode = 0;
    switch (type) {
        case eModbusPacketVariableLen:
        case eModbusPacketFull:
            if(*pucOutSize == 2) {
                for(uint32_t i = 0; i < pxFrame->ucLengthCode; i += 2)
                    *(uint16_t *)&pxFrame->pucData[i] = swap_bytes(*(uint16_t *)&pxFrame->pucData[i]);
                *pucOutAmount = pxFrame->ucLengthCode >> 1;
            }
            return pxFrame->pucData;
        case eModbusPacketBase:
            *pucOutAmount = 1;
            return (uint8_t *)&pxFrame->usRegValueCount;
        case eModbusPacketCode:
            *pucCode = pxFrame->ucLengthCode;
        default:
        break;
    }    
    return libNULL;
}

uint8_t modbus_init(modbus_t *, const modbus_config_t *) __attribute__ ((alias ("bModbusInit")));
void modbus_work(modbus_t *) __attribute__ ((alias ("vModbusWork")));
uint8_t modbus_server_link_endpoints(modbus_t *, const modbus_endpoint_t *) __attribute__ ((alias ("bModbusServerLinkEndpoints")));
uint32_t modbus_request(modbus_t *, modbus_frame_t *, modbus_cb_t, void *) __attribute__ ((alias ("ulModbusRequest")));
uint32_t modbus_response(modbus_t *, modbus_frame_t *) __attribute__ ((alias ("ulModbusResponse")));
uint8_t modbus_cancel_request(modbus_t *, uint32_t) __attribute__ ((alias ("bModbusCancelRequest")));
uint8_t modbus_busy(modbus_t *) __attribute__ ((alias ("bModbusBusy")));
uint8_t *modbus_frame_data(modbus_frame_t *, uint8_t *, uint8_t *, uint8_t *) __attribute__ ((alias ("pucModbusResponseFrameData")));
