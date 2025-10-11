#ifndef CL_EVENT_H_
#define CL_EVENT_H_

#define CL_DELEGATE_PRIVATE_SIZE    16
#define CL_EVENT_PRIVATE_SIZE        4

typedef void (*event_handler_t)(void *event_trigger, void *sender, void *context);

typedef struct {
  event_handler_t handler;
  void *context;
  CL_PRIVATE(CL_DELEGATE_PRIVATE_SIZE);
} delegate_t;

typedef void (*event_subscribe_t)(const void *, delegate_t *delegate);

typedef struct {
  CL_PRIVATE(CL_EVENT_PRIVATE_SIZE);
} event_t;

void event_subscribe(event_t *event, delegate_t *delegate);
void event_unsubscribe(delegate_t *delegate);
uint32_t event_raise(event_t *event, void *sender, void *event_trigger);

#endif // CL_EVENT_H_
