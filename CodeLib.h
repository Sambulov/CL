/*!
   LibPortable.h

    Created on: 10.02.2017
        Author: Sambulov Dmitry
        e-mail: dmitry.sambulov@gmail.com
 */

#ifndef CODE_LIB_H_INCLUDED
#define CODE_LIB_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef __builtin_va_list      va_list;
typedef __INT8_TYPE__          int8_t;
typedef __UINT8_TYPE__         uint8_t;
typedef __INT16_TYPE__         int16_t;
typedef __UINT16_TYPE__        uint16_t;
typedef __INT32_TYPE__         int32_t;
typedef __UINT32_TYPE__        uint32_t;
typedef __INT64_TYPE__         int64_t;
typedef __UINT64_TYPE__        uint64_t;
typedef __SIZE_TYPE__          size_t;
typedef __PTRDIFF_TYPE__       ptrdiff_t;

#define CL_FALSE 	           ((uint8_t)( 0 ))
#define CL_TRUE	               !CL_FALSE
#define CL_PRIVATE(size)       uint32_t __private[(size + sizeof(void *) - 1) >> 2]

#define libNULL                ((void *)0)

#define libUNUSED(x)           ((void)(x))

#define offsetof(TYPE, MEMBER) __builtin_offsetof (TYPE, MEMBER)

#define __weak                 __attribute__((weak))
#define __packed               __attribute__((__packed__))

typedef void** cl_tuple_t;
#define cl_tuple_make(...) ((void *[]){__VA_ARGS__})
#define cl_tuple_get(tuple, index, type) ((type)(((void **)tuple)[index]))

#define va_start(v,l)          __builtin_va_start(v,l)
#define va_end(v)              __builtin_va_end(v)
#define va_arg(v,l)            __builtin_va_arg(v,l)

#define LIB_ASSERRT_STRUCTURE_CAST(private_type, public_type, prv_size_def, def_file) \
    _Static_assert(sizeof(private_type) == sizeof(public_type), "In "#def_file" data structure size of "#public_type" doesn't match, check "#prv_size_def)
#define LIB_ASSERRT_STRUCTURE_PROP(private_type, public_type, prop, def_file) \
    _Static_assert((offsetof(private_type, prop) == offsetof(public_type, prop)), "In "def_file" "#public_type" property "#prop" order doesn't match");

#include "ClMacros.h"
#include "Math/Math.h"
#include "DataStructures/LinkedList.h"
#include "Workflow/CooperativeMultitasking.h"
#include "Workflow/MachineState.h"
#include "Workflow/Event.h"
#include "DataStructures/SimpleCircularBuffer.h"
#include "DataStructures/Mem.h"
#include "DataStructures/MedianFilter.h"
#include "Crypto/Crc.h"
#include "Binary/BitOp.h"
#include "Converters/StringConverter.h"
#include "Converters/CharsetEncoding.h"
#include "DataStructures/Str.h"
#include "DataStructures/CircularBuffer.h"
#include "DataStructures/Fifo.h"
#include "DataStructures/Stream.h"

#include "DataStructures/Printf.h"

#include "Proto/ModBus.h"
#include "Proto/ModBusHelpers.h"

#ifdef __cplusplus
}
#endif
	
#endif /* CODE_LIB_H_INCLUDED */
