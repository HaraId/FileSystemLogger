#pragma once

#include "ControlDeviceQueue.h"
#include "ThreadLockClasses.h"

#define CONTROL_DEVICE_NAME L"\\Device\\FileSysLog"
#define CONTROL_DEVICE_SYMLINK_NAME L"\\DosDevices\\FileSysLog"


struct FsOperationEventItem {
	LIST_ENTRY list;
	FsOperationEvent data;
};


struct FilterGlobalContext {
public:
	LIST_ENTRY logItemList;
	ULONG logItemlistSize;
	FastMutex contextLock;

	UNICODE_STRING targetApplicationName;

	bool isStoped;


	void clearContextInterlocked();
	static void init(FilterGlobalContext& inst);
	static void free(FilterGlobalContext& inst);
};

EXTERN_C_START

NTSTATUS CreateControlDeviceObject(WDFDRIVER WdfDriver);

EVT_WDF_OBJECT_CONTEXT_CLEANUP EvtWdfDeviceCleanup;


EXTERN_C_END