#include "CodeLib.h"

typedef struct {
  FsmStateHandler_t pfCurrentState;
  const FsmTransition_t **apxTransitions;
  void* pxContext;
#ifdef DEBUG
  FsmTransitionLog_t *pxLog;
#endif
} _FiniteStateMachine_t;

LIB_ASSERRT_STRUCTURE_CAST(_FiniteStateMachine_t, FiniteStateMachine_t, CL_FSM_PRIVATE_SIZE, MachineState.h);

#ifdef DEBUG 
#define fsm_log(fsm, event) if(fsm->pxLog && fsm->pxLog->axTransitions){\
    FsmTransitionLogEntry_t *log = &fsm->pxLog->axTransitions[fsm->pxLog->usLogPtr];\
    log->eEvent = event;\
    log->pfFrom = fsm->pfCurrentState;\
    if(log->ulCycles < 0xFFFFFFFF) log->ulCycles++;\
}

#define fsm_log_push(fsm) if(fsm->pxLog) {\
    fsm->pxLog->ulTransitionCount++;\
    fsm->pxLog->usLogPtr++;\
    if(fsm->pxLog->usLogPtr >= fsm->pxLog->usLogSize)\
        fsm->pxLog->usLogPtr = 0;\
    fsm->pxLog->axTransitions[fsm->pxLog->usLogPtr].ulCycles = 0;\
}

uint8_t bFsmSetTransitionLog(FiniteStateMachine_t *pxFsm, FsmTransitionLog_t *pxLog) {
    _FiniteStateMachine_t *fsm = (_FiniteStateMachine_t *)pxFsm;
    if((pxFsm == libNULL) || (pxLog == libNULL) || (pxLog->axTransitions == libNULL) || (pxLog->usLogSize == 0))
        return CL_FALSE;
    pxLog->usLogPtr = 0;
    pxLog->axTransitions[0].ulCycles = 0;
    fsm->pxLog = pxLog;
    return CL_TRUE;
}
#else
#define fsm_log(fsm, event, state)
#define fsm_log_push(fsm)
#endif

uint8_t bFsmReset(FiniteStateMachine_t* pxFsm) {
    _FiniteStateMachine_t *fsm = (_FiniteStateMachine_t *)pxFsm;
    if((pxFsm == libNULL) || (fsm->pfCurrentState != libNULL) || (fsm->apxTransitions == libNULL))
        return CL_FALSE;
    const FsmTransition_t *t = fsm->apxTransitions[0];
    while((t != libNULL) && (t->eEvent != EVENT_FSM_ENTER))
        t++;
    if (t != libNULL)
        fsm->pfCurrentState = t->pfToState;
    return (fsm->pfCurrentState != libNULL);
}

uint8_t bFsmInit(FiniteStateMachine_t *pxFsm, const FsmTransition_t *apxTransitions[], void *pxContext) {
    _FiniteStateMachine_t *fsm = (_FiniteStateMachine_t *)pxFsm;
    if(pxFsm != libNULL) {
        if(apxTransitions != libNULL) {
            fsm->pfCurrentState = libNULL;
            fsm->apxTransitions = apxTransitions;
            fsm->pxContext = pxContext;
            fsm->pxLog = libNULL;
            bFsmReset(pxFsm);
            return CL_TRUE;
        }
    }
    return CL_FALSE;
}

static uint8_t _bFsmCoroutine(coroutine_t *pxThis, uint8_t bCancel, void *pxArg) {
  (void)pxThis;
  return !bFsmProcess((FiniteStateMachine_t *)pxArg) || bCancel;
}

uint8_t bFsmCoroutineInit(Coroutine_t *pxCoR, FiniteStateMachine_t *pxFsm, const FsmTransition_t *apxTransitions[], void *pxContext) {
    if(bFsmInit(pxFsm, apxTransitions, pxContext)) {
        vCoroutineAdd(pxCoR, &_bFsmCoroutine, pxFsm);
        return CL_TRUE;
    }
    return CL_FALSE;
}

uint8_t bFsmValid(FiniteStateMachine_t *pxFsm) {
    _FiniteStateMachine_t *fsm = (_FiniteStateMachine_t *)pxFsm;
    return (pxFsm != libNULL) && (fsm->apxTransitions != libNULL);
}

uint8_t bFsmRun(FiniteStateMachine_t *pxFsm, Coroutine_t *pxCoR) {
    if(bFsmValid(pxFsm) && pxCoR) {
        vCoroutineSetContext(pxCoR, pxFsm);
        vCoroutineSetHandler(pxCoR, &_bFsmCoroutine);
        return CL_TRUE;
    }
    return CL_FALSE;
}


uint8_t bFsmProcess(FiniteStateMachine_t* pxFsm) {
    _FiniteStateMachine_t *fsm = (_FiniteStateMachine_t *)pxFsm;
    if (!pxFsm || (fsm->pfCurrentState == FSM_STATE_EXIT))
        return CL_FALSE;
    
    FsmEvent_t generated_event = fsm->pfCurrentState(fsm->pxContext);

    fsm_log(fsm, generated_event);

    if (generated_event != EVENT_NONE) {
        const FsmTransition_t *t = fsm->apxTransitions[0];
        while(t != libNULL) {
            if ((t->pfFromState == fsm->pfCurrentState) && (t->eEvent == generated_event)) {
                fsm_log_push(fsm);
                fsm->pfCurrentState = t->pfToState;
                break;
            }
            t++;
        }
    }

    return CL_TRUE;
}

#ifdef DEBUG
uint8_t fsm_set_transition_log(finite_state_machine_t *fsm, fsm_transition_log_t *log) 
                                                        __attribute__ ((alias ("bFsmSetTransitionLog")));
#endif
uint8_t fsm_init(finite_state_machine_t* fsm, const fsm_transition_t **transitions, void *context) 
                                                        __attribute__ ((alias ("bFsmInit")));
uint8_t fsm_run(finite_state_machine_t *fsm, coroutine_t *cor)
                                                        __attribute__ ((alias ("bFsmRun")));

uint8_t fsm_coroutine_init(coroutine_t *cor, finite_state_machine_t* fsm, const fsm_transition_t *transitions[], void *context)
                                                        __attribute__ ((alias ("bFsmCoroutineInit")));

uint8_t fsm_valid(finite_state_machine_t* fsm)          __attribute__ ((alias ("bFsmValid")));
uint8_t fsm_reset(finite_state_machine_t* fsm)          __attribute__ ((alias ("bFsmReset")));
uint8_t fsm_process(finite_state_machine_t* fsm)        __attribute__ ((alias ("bFsmProcess")));
