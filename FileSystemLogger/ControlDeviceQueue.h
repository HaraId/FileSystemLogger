#pragma once


#include <ntddk.h>
#include <wdf.h>
#include <initguid.h>

#include "Public.h"
#include "Trace.h"


EXTERN_C_START

EVT_WDF_IO_QUEUE_IO_READ EvtWdfIoQueueIoRead;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL EvtWdfIoQueueIoDeviceControl;

EXTERN_C_END
