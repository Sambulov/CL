#include "CodeLib.h"

typedef struct {
  event_handler_t handler;
  void *context;
  __linked_list_object__;
} delegate_private_t;

typedef struct {
  linked_list_t delegates;
} event_private_t;

LIB_ASSERRT_STRUCTURE_CAST(delegate_private_t, delegate_t, CL_DELEGATE_PRIVATE_SIZE, Event.h);
LIB_ASSERRT_STRUCTURE_CAST(event_private_t, event_t, CL_EVENT_PRIVATE_SIZE, Event.h);

void event_subscribe(event_t *event, delegate_t *delegate) {
  event_private_t *evt = (event_private_t *)event;
  if((event != libNULL) && (delegate != libNULL))
    linked_list_insert(&(evt->delegates), linked_list_item((delegate_private_t*)delegate), libNULL);
}

void event_unsubscribe(delegate_t *delegate) {
  if(delegate != libNULL)
    linked_list_unlink(linked_list_item((delegate_private_t*)delegate));
}

void event_clear(event_t *event) {
  event_private_t *evt = (event_private_t *)event;
  linked_list_clear(&evt->delegates);
}

static void _call_delegate(linked_list_item_t *item, void *arg) {
  delegate_private_t *delegate = linked_list_get_object(delegate_private_t, item);
  cl_tuple_t args = (cl_tuple_t)arg;
  if(delegate->handler)
    delegate->handler(args[0], args[1], delegate->context);
}

uint32_t event_raise(event_t *event, void *sender, void *event_trigger) {
  event_private_t *evt = (event_private_t *)event;
  if(event != libNULL)
    return linked_list_do_foreach(evt->delegates, &_call_delegate, cl_tuple_make(event_trigger, sender));
  return 0;
}
