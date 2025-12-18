/*!
    MachineState.h
  
    Created on: 12.11.2025
        Author: Sambulov Dmitry
        e-mail: dmitry.sambulov@gmail.com


    Usage exapmple:

    -- Define transitions graph

    typedef enum {
      EVENT_START,
      EVENT_COMPLETE,
      EVENT_ERROR,
      EVENT_RESET
    } SomeFsmEvents_t;

    const FsmTransition_t *apxFsmTransitions[] = {
        &(const FsmTransition_t) {idle,    EVENT_START,    running},
        &(const FsmTransition_t) {running, EVENT_COMPLETE, FSM_STATE_EXIT},
        &(const FsmTransition_t) {running, EVENT_ERROR,    error},
        &(const FsmTransition_t) {error,   EVENT_RESET,    idle},
        (const FsmTransition_t *) NULL
    };

    -- Init fsm

    FiniteStateMachine_t xSomeFsm;
    bFsmInit(&xSomeFsm, apxFsmTransitions, pxAppContext);

    -- Run in loop

    while(bFsmProcess(&fsm));

*/
#ifndef MACHINE_STATE_H_INCLUDED
#define MACHINE_STATE_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG
  #define CL_FSM_PRIVATE_SIZE   16
#else
  #define CL_FSM_PRIVATE_SIZE   12
#endif

typedef uint32_t FsmEvent_t;

#define EVENT_FSM_ENTER              0x00000000
#define EVENT_NONE                   0xFFFFFFFF

#define FSM_STATE_EXIT               ((FsmStateHandler_t)libNULL)

typedef FsmEvent_t (*FsmStateHandler_t)(void* context);

typedef struct {
  FsmStateHandler_t pfFromState;
  FsmEvent_t eEvent;
  FsmStateHandler_t pfToState;
} FsmTransition_t;

#ifdef DEBUG
typedef struct {
  FsmStateHandler_t pfFrom;
  FsmEvent_t eEvent;
  uint32_t ulCycles;
} FsmTransitionLogEntry_t;

typedef struct {
  uint32_t ulTransitionCount;
  FsmTransitionLogEntry_t *axTransitions;
  uint16_t usLogSize;
  uint16_t usLogPtr;
} FsmTransitionLog_t;
#endif

typedef struct {
  CL_PRIVATE(CL_FSM_PRIVATE_SIZE);
} FiniteStateMachine_t;

#ifdef DEBUG
uint8_t bFsmSetTransitionLog(FiniteStateMachine_t *pxFsm, FsmTransitionLog_t *pxLog);
#endif

#define FSM_TRANSITION(From, onEvent, To) &(const fsm_transition_t) {From,    onEvent,    To}

/*!
	@brief Initialize finite state machine
	@param[in] pxFsm            FSM descriptor buffer
	@param[in] apxTransitions   Transitions graph
	@param[in] pxContext        FSM handlers context
	@return !0 if init ok
*/
uint8_t bFsmInit(FiniteStateMachine_t* pxFsm, const FsmTransition_t **apxTransitions, void *pxContext);

/*!
	@brief Run finite state machine with in coriutine
	@param[in] pxFsm            FSM descriptor buffer
	@param[in] pxCoR            Runing coroutine descriptor
	@return !0 if init ok
*/
uint8_t bFsmRun(FiniteStateMachine_t* pxFsm, Coroutine_t *pxCoR);

/*!
	@brief Initialize finite state machine and run as coroutine
	@param[in] pxCoR            Coroutine descriptor buffer
	@param[in] pxFsm            FSM descriptor buffer
	@param[in] apxTransitions   Transitions graph
	@param[in] pxContext        FSM handlers context
	@return !0 if init ok
*/
uint8_t bFsmCoroutineInit(Coroutine_t *pxCoR, FiniteStateMachine_t *pxFsm, const FsmTransition_t *apxTransitions[], void *pxContext);

/*!
	@brief Restart finite state machine if it is finished
	@param[in] pxFsm            FSM descriptor buffer
	@return !0 if restarted
*/
uint8_t bFsmReset(FiniteStateMachine_t* pxFsm);

/*!
	@brief Check if FSM can be procrssed
	@param[in] pxFsm            FSM descriptor
	@return !0 if ok
*/
uint8_t bFsmValid(FiniteStateMachine_t* pxFsm);

/*!
	@brief Do FSM cycle
	@param[in] pxFsm            FSM descriptor
	@return !0 while process ongoing
*/
uint8_t bFsmProcess(FiniteStateMachine_t* pxFsm);

/*!
  Snake notation
*/

typedef FiniteStateMachine_t finite_state_machine_t;
typedef FsmTransition_t fsm_transition_t;
typedef FsmEvent_t fsm_event_t;
typedef FsmStateHandler_t fsm_state_handler_t;

#ifdef DEBUG
typedef FsmTransitionLog_t fsm_transition_log_t;

uint8_t fsm_set_transition_log(finite_state_machine_t *fsm, fsm_transition_log_t *log);
#endif

uint8_t fsm_init(finite_state_machine_t* fsm, const fsm_transition_t *transitions[], void *context);

uint8_t fsm_run(finite_state_machine_t* fsm, coroutine_t *cor);
uint8_t fsm_coroutine_init(coroutine_t *cor, finite_state_machine_t* fsm, const fsm_transition_t *transitions[], void *context);

uint8_t fsm_reset(finite_state_machine_t* fsm);
uint8_t fsm_valid(finite_state_machine_t* fsm);

uint8_t fsm_process(finite_state_machine_t* fsm);

#ifdef __cplusplus
}
#endif

#endif /* MACHINE_STATE_H_INCLUDED */
