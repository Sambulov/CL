/*!
   Modbus.h

    Created on: 25.04.2023
        Author: Sambulov Dmitry
        e-mail: dmitry.sambulov@gmail.com
 */

#ifndef MODBUS_H_INCLUDED
#define MODBUS_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define MODBUS_ERROR_FLAG                   0x80
#define MODBUS_DESCRIPTOR_SIZE              56

typedef struct {
	CL_PRIVATE(MODBUS_DESCRIPTOR_SIZE);
} Modbus_t;

typedef enum {
	/*! The function code received in the query is not an allowable action for the slave.
	  This may be because the function code is only applicable to newer devices, and was
	  not implemented in the unit selected.  It could also indicate that the slave is in
	  the wrong state to process a request of this type, for example because it is
	  unconfigured and is being asked to return register values. If a Poll Program
	  Complete command was issued, this code indicates that no program function preceded
	  it. */
	ModbusIllegalFunction = 0x01,

	/*! The data address received in the query is not an allowable address for the slave.
	  More specifically, the combination of reference number and transfer length is
	  invalid. For a controller with 100 registers, a request with offset 96 and length 4
	  would succeed, a request with offset 96 and length 5 will generate exception 02. */
	ModbusIllegalDataAddress = 0x02,

	/*! A value contained in the query data field is not an allowable value for the slave.
	  This indicates a fault in the structure of remainder of a complex request, such as
	  that the implied length is incorrect. It specifically does NOT mean that a data item
	  submitted for storage in a register has a value outside the expectation of the
	  application program, since the MODBUS protocol is unaware of the significance of any
	  particular value of any particular register. */
	ModbusIllegalDataValue = 0x03,

	/* An unrecoverable error occurred while the slave was attempting to perform the
	  requested action. */
	ModbusSlaveDeviceFailure = 0x04,

	/* Specialized use in conjunction with programming commands.
	  The slave has accepted the request and is processing it, but a long duration of time
	  will be required to do so.  This response is returned to prevent a timeout error
	  from occurring in the master. The master can next issue a Poll Program Complete
	  message to determine if processing is completed. */
	ModbusAcknowledge = 0x05,

	/* Specialized use in conjunction with programming commands.
	  The slave is engaged in processing a long-duration program command.  The master
	  should retransmit the message later when the slave is free.. */
	ModbusSlaveDeviceBusy = 0x06,

	/*! The slave cannot perform the program function received in the query. This code is
	  returned for an unsuccessful programming request using function code 13 or 14
	  decimal. The master should request diagnostic or error information from the slave. */
	ModbusNegativeAcknowledge = 0x07,

	/*! Specialized use in conjunction with function codes 20 and 21 and reference type 6,
	  to indicate that the extended file area failed to pass a consistency check.
      The slave attempted to read extended memory or record file, but detected a parity
      error in memory. The master can retry the request, but service may be required on
      the slave device. */
	ModbusMemoryParityError = 0x08,

	/*! Specialized use in conjunction with gateways, indicates that the gateway was
	  unable to allocate an internal communication path from the input port to the output
	  port for processing the request. Usually means the gateway is misconfigured or
	  overloaded. */
	ModbusGatewayPathUnavailable = 0x0A,

	/*! Specialized use in conjunction with gateways, indicates that no response was
	  obtained from the target device. Usually means that the device is not present on the
	  network. */
	ModbusGatewayTargetDeviceFailedtoRespond = 0x0B,
} ModbusErrorCode_t;

typedef enum {
	MB_FUNC_READ_OUTPUTS         = 0x01, /* Read discrete outputs, modbus coils */
	MB_FUNC_READ_INPUTS          = 0x02, /* Read discrete inputs */
	MB_FUNC_READ_HOLDINGS        = 0x03, /* Read analog outputs, modbus holding registers */
	MB_FUNC_READ_INPUT_REGISTERS = 0x04, /* Read analog input, modbus input registers */
	MB_FUNC_WRITE_OUTPUT         = 0x05, /* Write single discrete output, modbus coil */
	MB_FUNC_WRITE_HOLDING        = 0x06, /* Write single analog output, modbus holding register */
	MB_FUNC_WRITE_COILS          = 0x0f, /* Write multiple discrete outputs, modbus coils */
	MB_FUNC_WRITE_HOLDINGS       = 0x10  /* Write multiple analog outputs, modbus holding registers */
} ModbusFunctionCode_t;

typedef enum {
    MB_STATE_UNDEF,
    MB_STATE_IDLE,      /* client: no active request */
    MB_STATE_SENDING,   /* client + server: iterative write via pfWrite */
    MB_STATE_LISTENING, /* client + server: waiting for the first byte */
    MB_STATE_RECEIVING, /* client + server: bytes arriving */
    MB_STATE_RECEIVED,  /* client + server: bEnd received, ready to parse */
    MB_STATE_HANDLING,  /* server: handler called, waiting for ulModbusResponse */
} ModbusState_t;

typedef struct {
	uint8_t ucAddr;           /* Device address on bus */
	uint8_t ucFunc;           /* Function number */
	uint16_t usRegAddr;       /* Register address */
	uint16_t usRegValueCount; /* Register value or amount */
	uint8_t ucLengthCode;     /* Data length next or function code response or error */
	uint8_t *pucData;         /* Frame payload */
} ModbusFrame_t;

typedef int32_t (*ModbusIfaceWrite_t)(void *pxPhy, const uint8_t *pucBuf, uint16_t ulSize);
typedef uint32_t (*ModbusIfaceTimer_t)(const void *pxTimerPhy);
typedef void (*ModbusCb_t)(ModbusFrame_t *pxFrame, Modbus_t *pxMb, void *pxContext);

typedef struct {
	ModbusIfaceWrite_t pfWrite;                /* Method to send data */
	ModbusIfaceTimer_t pfTimer;                /* Method to get timestamp */
} ModbusIface_t;

typedef enum {
    MB_MODE_PDU     = 0x00,
    MB_MODE_RTU     = 0x01,
    MB_MODE_ASCII   = 0x02,
    MB_MODE_TCP_UDP = 0x03
} ModbusMode_t;

typedef struct {
    const ModbusIface_t *pxIface;    /* Modbus extern interface  */
    void *pxTxContext;               /* Context for pfWrite */
    void *pxTimerContext;            /* Context for pfTimer */
    void *pxRequestContext;          /* Context for pfOnRequest */
    uint8_t *pucPayLoadBuffer;       /* Modbus payload buffer */
    uint16_t ucPayLoadBufferSize;    /* Modbus payload buffer size */
    uint16_t usTxTimeout;             /* Transmit timeout in interface timer time units */
    uint16_t usRxTimeout;             /* Receiving timeout for server capability */
    ModbusMode_t eMode;              /* Modbus parser mode */
    uint8_t bIsServer;
	ModbusCb_t pfOnRequest;          /* Modbus server on request callback */
} ModbusConfig_t;

/*!
	@brief Init modbus entity
	@param[in] pxMb        Modbus descriptor buffer
	@param[in] pxConfig    Modbus config
	@return !0 if ok
*/
uint8_t bModbusInit(Modbus_t *pxMb, const ModbusConfig_t *pxConfig);

/*!
	@brief Modbus working loop
	@param[in] pxMb        Modbus descriptor
*/
void vModbusWork(Modbus_t *pxMb);

/*!
	@brief Modbus client send request
	@param[in] pxMb        Modbus descriptor
	@param[in] pxFrame     Modbus frame to request. Could be destroyed after call.
	@param[in] pfCallback  On request complete callback.
	@param[in] pxCbContext Callback user context.
	@param[in] ulTimeout   Responce timeout.
	@return transfer ID if ok, 0 if fault
*/
uint16_t usModbusRequest(Modbus_t *pxMb, ModbusFrame_t *pxFrame, ModbusCb_t pfCallback, void *pxCbContext, uint16_t ulTimeout);

/*!
	@brief Modbus server send response
	@param[in] pxMb        Modbus descriptor
	@param[in] pxFrame     Modbus frame to request. Could be destroyed after call.
	@return transfer ID if ok, 0 if fault
*/
uint16_t usModbusResponse(Modbus_t *pxMb, ModbusFrame_t *pxFrame);

/*!
	@brief Modbus reset state
	@param[in] pxMb         Modbus descriptor
	@return !0 if reseted
*/
uint8_t bModbusReset(Modbus_t *pxMb);

/*!
	@brief Check if modbus busy
	@param[in] pxMb        Modbus descriptor
	@return !0 if busy
*/
uint8_t bModbusBusy(Modbus_t *pxMb);

/*!
	@brief Push data to Modbus stack
	@param[in] pxMb        Modbus descriptor
	@param[in] pucData     Frame fragment data
	@param[in] usLen       Fragment length
	@param[in] bEnd        Is last fragment
*/
void vModbusReceiveData(Modbus_t *pxMb, const uint8_t *pucData, uint16_t usLen, uint8_t bEnd);

/*!
	@brief Get current modbus FSM state
	@param[in] pxMb        Modbus descriptor
	@return current modbus state
*/
ModbusState_t vModbusGetState(Modbus_t *pxMb);

static inline uint8_t bModbusIsErrorFrame(ModbusFrame_t *pxFrame) {
    return ((pxFrame == libNULL) || ((pxFrame->ucFunc & MODBUS_ERROR_FLAG) != 0));
}

/*!
  Snake notation
*/

typedef Modbus_t modbus_t;
typedef ModbusConfig_t modbus_config_t;
typedef ModbusFrame_t modbus_frame_t;
typedef ModbusCb_t modbus_cb_t;
typedef ModbusErrorCode_t modbus_error_code_t;
typedef ModbusIface_t modbus_iface_t;
typedef ModbusIfaceWrite_t modbus_iface_write_t;
typedef ModbusIfaceTimer_t modbus_iface_timer_t;
typedef ModbusMode_t modbus_mode_t;
typedef ModbusState_t modbus_state_t;

uint8_t modbus_init(modbus_t *, const modbus_config_t *);
void modbus_work(modbus_t *);
uint16_t modbus_response(modbus_t *, modbus_frame_t *);
uint16_t modbus_request(modbus_t *, modbus_frame_t *, modbus_cb_t, void *, uint16_t);
uint8_t modbus_reset(modbus_t *);
uint8_t modbus_busy(modbus_t *);
uint8_t *modbus_frame_data(modbus_frame_t *, uint8_t *, uint8_t *, uint8_t *);
void modbus_receive_data(modbus_t *, const uint8_t *, uint16_t, uint8_t);
modbus_state_t modbus_get_state(modbus_t *);

static inline uint8_t modbus_is_error_frame(modbus_frame_t *)  __attribute__ ((alias ("bModbusIsErrorFrame")));

#ifdef __cplusplus
}
#endif

#endif /* MODBUS_H_INCLUDED */
