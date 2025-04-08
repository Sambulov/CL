/*!
   ModBusHelpers.h

    Created on: 25.04.2023
        Author: Sambulov Dmitry
        e-mail: dmitry.sambulov@gmail.com
 */

#ifndef MODBUS_HELPERS_H_INCLUDED
#define MODBUS_HELPERS_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "ModBus.h"

/*!
	@brief Init frame structure for read discrete outputs, modbus coils, function 0x01
	@param[out] pxOutFrame			Frame buffer
	@param[in] pxContext            Callback context
	@param[in] pfCb	                Callback function
	@param[in] ucDevAddr            Modbus device address
	@param[in] usRegAddr            Modbus register address
	@param[in] usAmount             Amount to read
*/
static inline void vModbusInitFrameReadOutputs(ModbusFrame_t *pxOutFrame, uint8_t ucDevAddr, uint16_t usRegAddr, uint16_t usAmount) {
	if((pxOutFrame == libNULL) || (usAmount == 0)) return;
	pxOutFrame->ucAddr = ucDevAddr;
	pxOutFrame->ucFunc = MB_FUNC_READ_OUTPUTS;
	pxOutFrame->usRegAddr = usRegAddr;
	pxOutFrame->usRegValueCount = usAmount;
}

/*!
	@brief Init frame structure for read discrete inputs, function 0x02
	@param[out] pxOutFrame			Frame buffer
	@param[in] pxContext            Callback context
	@param[in] pfCb	                Callback function
	@param[in] ucDevAddr            Modbus device address
	@param[in] usRegAddr            Modbus register address
	@param[in] usAmount             Amount to read
*/
static inline void vModbusInitFrameReadInputs(ModbusFrame_t *pxOutFrame, uint8_t ucDevAddr, uint16_t usRegAddr, uint16_t usAmount) {
	if((pxOutFrame == libNULL) || (usAmount == 0)) return;
	vModbusInitFrameReadOutputs(pxOutFrame, ucDevAddr, usRegAddr, usAmount);
	pxOutFrame->ucFunc = MB_FUNC_READ_INPUTS;
}

/*!
	@brief Init frame structure for read analog outputs, modbus holding registers, function 0x03
	@param[out] pxOutFrame			Frame buffer
	@param[in] pxContext            Callback context
	@param[in] pfCb	                Callback function
	@param[in] ucDevAddr            Modbus device address
	@param[in] usRegAddr            Modbus register address
	@param[in] usAmount             Amount to read
*/
static inline void vModbusInitFrameReadHoldings(ModbusFrame_t *pxOutFrame, uint8_t ucDevAddr, uint16_t usRegAddr, uint16_t usAmount) {
	if((pxOutFrame == libNULL) || (usAmount == 0)) return;
	vModbusInitFrameReadOutputs(pxOutFrame, ucDevAddr, usRegAddr, usAmount);
	pxOutFrame->ucFunc = MB_FUNC_READ_HOLDINGS;
}

/*!
	@brief Init frame structure for read analog inputs, modbus input registers, function 0x04
	@param[out] pxOutFrame			Frame buffer
	@param[in] pxContext            Callback context
	@param[in] pfCb	                Callback function
	@param[in] ucDevAddr            Modbus device address
	@param[in] usRegAddr            Modbus register address
	@param[in] usAmount             Amount to read
*/
static inline void vModbusInitFrameReadInputRegisters(ModbusFrame_t *pxOutFrame, uint8_t ucDevAddr, uint16_t usRegAddr, uint16_t usAmount) {
	if((pxOutFrame == libNULL) || (usAmount == 0)) return;
	vModbusInitFrameReadOutputs(pxOutFrame, ucDevAddr, usRegAddr, usAmount);
	pxOutFrame->ucFunc = MB_FUNC_READ_INPUT_REGISTERS;
}

/*!
	@brief Init frame structure for write descrete output, modbus coil, function 0x05
	@param[out] pxOutFrame			Frame buffer
	@param[in] pxContext            Callback context
	@param[in] pfCb	                Callback function
	@param[in] ucDevAddr            Modbus device address
	@param[in] usRegAddr            Modbus register address
	@param[in] bState               Value: 0-reset, !0- set
*/
static inline void vModbusInitFrameWriteOutput(ModbusFrame_t *pxOutFrame, uint8_t ucDevAddr, uint16_t usRegAddr, uint8_t bState) {
	if(pxOutFrame == libNULL) return;
	vModbusInitFrameReadOutputs(pxOutFrame, ucDevAddr, usRegAddr, bState? 0xFF00 :0x0000);
	pxOutFrame->ucFunc = MB_FUNC_WRITE_OUTPUT;
}

/*!
	@brief Init frame structure for write analog output, modbus holding register, function 0x06
	@param[out] pxOutFrame			Frame buffer
	@param[in] pxContext            Callback context
	@param[in] pfCb	                Callback function
	@param[in] ucDevAddr            Modbus device address
	@param[in] usRegAddr            Modbus register address
	@param[in] usValue              Register value
*/
static inline void vModbusInitFrameWriteHolding(ModbusFrame_t *pxOutFrame, uint8_t ucDevAddr, uint16_t usRegAddr, uint16_t usValue) {
	if(pxOutFrame == libNULL) return;
	vModbusInitFrameReadOutputs(pxOutFrame, ucDevAddr, usRegAddr, usValue);
	pxOutFrame->ucFunc = MB_FUNC_WRITE_OUTPUT;
}

/*!
	@brief Init frame structure for write multiple descrete outputs, modbus coils, function 0x0F
	@param[out] pxOutFrame			Frame buffer
	@param[in] pxContext            Callback context
	@param[in] pfCb	                Callback function
	@param[in] ucDevAddr            Modbus device address
	@param[in] usRegAddr            Modbus register address
	@param[in] usAmount             Amount to write
	@param[in] pucValues            Coils states
*/
static inline void vModbusInitFrameWriteCoils(ModbusFrame_t *pxOutFrame, uint8_t ucDevAddr, uint16_t usRegAddr, uint16_t usAmount, uint8_t *pucValues) {
	if((pxOutFrame == libNULL) || (usAmount == 0) || (usAmount > 2040)) return;
	vModbusInitFrameReadOutputs(pxOutFrame, ucDevAddr, usRegAddr, usAmount);
	pxOutFrame->ucFunc = MB_FUNC_WRITE_COILS;
	pxOutFrame->pucData = pucValues;
	pxOutFrame->ucLengthCode = (usAmount >> 3);
}

/*!
	@brief Init frame structure for write multiple analog outputs, modbus holding registres, function 0x10
	@param[out] pxOutFrame			Frame buffer
	@param[in] pxContext            Callback context
	@param[in] pfCb	                Callback function
	@param[in] ucDevAddr            Modbus device address
	@param[in] usRegAddr            Modbus register address
	@param[in] usAmount             Amount to write
	@param[in] pusValues            Registers values
*/
static inline void vModbusInitFrameWriteHoldings(ModbusFrame_t *pxOutFrame, uint8_t ucDevAddr, uint16_t usRegAddr, uint16_t usAmount, uint16_t *pusValues) {
	if((pxOutFrame == libNULL) || (usAmount == 0) || (usAmount > 127)) return;
	vModbusInitFrameReadOutputs(pxOutFrame, ucDevAddr, usRegAddr, usAmount);
	pxOutFrame->ucFunc = MB_FUNC_WRITE_HOLDINGS;
	pxOutFrame->pucData = (uint8_t *)pusValues;
	pxOutFrame->ucLengthCode = (usAmount * 2);
}

/*!
	@brief Init frame structure for error response
	@param[out] pxOutFrame			Frame buffer
	@param[in] ucDevAddr            Modbus device address
	@param[in] ucFunc               Modbus function
	@param[in] eErr                 Error code
*/
static inline void vModbusInitFrameError(ModbusFrame_t *pxOutFrame, uint8_t ucDevAddr, uint8_t ucFunc, ModbusErrorCode_t eErr) {
	if(pxOutFrame == libNULL) return;
	pxOutFrame->ucFunc = ucFunc | MODBUS_ERROR_FLAG;
	pxOutFrame->ucAddr = ucDevAddr;
    pxOutFrame->ucLengthCode = eErr;
}



//typedef uint8_t (*ReadDiscreteHandler_t)(uint16_t usAddr);
//typedef uint16_t (*ReadRegisterHandler_t)(uint16_t usAddr);
//typedef void (*WriteRegisterHandler_t)(uint16_t usAddr, uint16_t usValue);
//typedef uint8_t (*AssertHandler_t)(uint16_t usAddr, uint16_t pusLengthValue);


/*!
  Snake notation
*/

static inline void modbus_init_frame_read_outputs(modbus_frame_t *, uint8_t, uint16_t, uint16_t)\
    __attribute__ ((alias ("vModbusInitFrameReadOutputs")));
static inline void modbus_init_frame_read_inputs(modbus_frame_t *, uint8_t, uint16_t, uint16_t)\
    __attribute__ ((alias ("vModbusInitFrameReadInputs")));
static inline void modbus_init_frame_read_holdings(modbus_frame_t *, uint8_t, uint16_t, uint16_t)\
    __attribute__ ((alias ("vModbusInitFrameReadHoldings")));
static inline void modbus_init_frame_read_input_registers(modbus_frame_t *, uint8_t, uint16_t, uint16_t)\
    __attribute__ ((alias ("vModbusInitFrameReadInputRegisters")));
static inline void modbus_init_frame_write_output(modbus_frame_t *, uint8_t, uint16_t, uint8_t)\
    __attribute__ ((alias ("vModbusInitFrameWriteOutput")));
static inline void modbus_init_frame_write_holding(modbus_frame_t *, uint8_t, uint16_t, uint16_t)\
    __attribute__ ((alias ("vModbusInitFrameWriteHolding")));
static inline void modbus_init_frame_write_coils(modbus_frame_t *, uint8_t, uint16_t, uint16_t, uint8_t *)\
    __attribute__ ((alias ("vModbusInitFrameWriteCoils")));
static inline void modbus_init_frame_write_holdings(modbus_frame_t *, uint8_t, uint16_t, uint16_t, uint16_t *)\
    __attribute__ ((alias ("vModbusInitFrameWriteHoldings")));
static inline void modbus_init_frame_error(ModbusFrame_t *, uint8_t, uint8_t, ModbusErrorCode_t)
    __attribute__ ((alias ("vModbusInitFrameError")));

#ifdef __cplusplus
}
#endif

#endif /* MODBUS_HELPERS_H_INCLUDED */
