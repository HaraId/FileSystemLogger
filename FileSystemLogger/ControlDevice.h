#pragma once

#include "ControlDeviceQueue.h"
#include "WdfClassExtension.h"

#define CONTROL_DEVICE_NAME L"\\Device\\FileSysLog"
#define CONTROL_DEVICE_SYMLINK_NAME L"\\DosDevices\\FileSysLog"



struct FsOperationEventItem {
	LIST_ENTRY list;

	FsOperationEvent data;
};


struct FilterGlobalContext {
	LIST_ENTRY logItemList;
	ULONG logItemlistSize;
	FastMutex contextLock;

	UNICODE_STRING targetApplicationName;

	bool isStoped;


	void clearTargetAppName()
	{
		if (targetApplicationName.Buffer != nullptr)
			RtlFreeUnicodeString(&targetApplicationName);

		targetApplicationName.Buffer = nullptr;
		targetApplicationName.Length = targetApplicationName.MaximumLength = 0;
	}

	static void init(FilterGlobalContext& inst) 
	{
		inst.contextLock.Init();
		InitializeListHead(&inst.logItemList);
		inst.logItemlistSize = 0;

		inst.targetApplicationName.Length = inst.targetApplicationName.MaximumLength = 0;
		inst.targetApplicationName.Buffer = nullptr;

		inst.isStoped = true;
	}

	static void free(FilterGlobalContext& inst)
	{
		AutoLock<FastMutex>(inst.contextLock);

		inst.isStoped = true;

		inst.clearTargetAppName();
	}
};

EXTERN_C_START

NTSTATUS CreateControlDeviceObject(WDFDRIVER WdfDriver);

EVT_WDF_OBJECT_CONTEXT_CLEANUP EvtWdfDeviceCleanup;


EXTERN_C_END