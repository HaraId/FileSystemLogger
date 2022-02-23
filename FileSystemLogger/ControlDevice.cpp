#include "ControlDevice.h"
#include "ControlDevice.tmh"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, CreateControlDeviceObject)
//#pragma alloc_text(PAGE, BaseFileSystemFilterUnload)

#endif

FilterGlobalContext g_FilterContext;


NTSTATUS CreateControlDeviceObject(WDFDRIVER WdfDriver)
{
	NTSTATUS status = STATUS_SUCCESS;

	WDFDEVICE device;
	PWDFDEVICE_INIT deviceInit;

	WDF_OBJECT_ATTRIBUTES deviceAttributes;
	
	WDFQUEUE deviceDefQueue;
	WDF_OBJECT_ATTRIBUTES queueAttribute;
	WDF_IO_QUEUE_CONFIG queueConfig;

	DECLARE_CONST_UNICODE_STRING(uDeviceName, CONTROL_DEVICE_NAME);
	DECLARE_CONST_UNICODE_STRING(uDeviceSymLinkName, CONTROL_DEVICE_SYMLINK_NAME);


	TraceEvents(TRACE_LEVEL_INFORMATION, WPP_DEVICE_TRACE, "%!FUNC! Entry. IRQL = %!irql!.", KeGetCurrentIrql());

	// Выдуляем память под структуру инициализации девайса
	deviceInit = WdfControlDeviceInitAllocate(WdfDriver, &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R);

	if (deviceInit == NULL)
	{
		TraceEvents(TRACE_LEVEL_INFORMATION, WPP_DEVICE_TRACE, "%!FUNC! [-] WdfControlDeviceInitAllocate return null.");
		status = STATUS_INSUFFICIENT_RESOURCES;
		return status;
	}


	// Для управляющего устройства обязательно необходимо задават имя для устройства
	status = WdfDeviceInitAssignName(deviceInit, &uDeviceName);

	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_INFORMATION, WPP_DEVICE_TRACE, "%!FUNC! [-] WdfDeviceInitAssignName status = %!STATUS!.", status);
		WdfDeviceInitFree(deviceInit);
		deviceInit = NULL;
		return status;
	}


	WdfDeviceInitSetExclusive(deviceInit, TRUE);

	WdfDeviceInitSetIoType(deviceInit, WdfDeviceIoBuffered);



	WDF_OBJECT_ATTRIBUTES_INIT(&deviceAttributes);
	deviceAttributes.EvtCleanupCallback = EvtWdfDeviceCleanup;

	status = WdfDeviceCreate(
		&deviceInit,
		&deviceAttributes,
		&device
	);

	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_INFORMATION, WPP_DEVICE_TRACE, "%!FUNC! [-] WdfDeviceCreate status = %!STATUS!.", status);
		WdfDeviceInitFree(deviceInit);
		return status;
	}


	//
	// Создание общей очереди для всех запросов
	//

	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchSequential);
	queueConfig.EvtIoDeviceControl = EvtWdfIoQueueIoDeviceControl;
	queueConfig.EvtIoRead = EvtWdfIoQueueIoRead;

	WDF_OBJECT_ATTRIBUTES_INIT(&queueAttribute);
	queueAttribute.ExecutionLevel = WdfExecutionLevelPassive;

	status = WdfIoQueueCreate(
		device,
		&queueConfig,
		&queueAttribute,
		&deviceDefQueue
	);

	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_INFORMATION, WPP_DEVICE_TRACE, "%!FUNC! [-] WdfIoQueueCreate status = %!STATUS!.", status);
		//WdfObjectDelete(device);
		return status;
	}

	
	status = WdfDeviceCreateSymbolicLink(device, &uDeviceSymLinkName);

	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_INFORMATION, WPP_DEVICE_TRACE, "%!FUNC! [-] WdfDeviceCreateSymbolicLink status = %!STATUS!.", status);
		return status;
	}

	TraceEvents(TRACE_LEVEL_INFORMATION, WPP_DEVICE_TRACE, "%!FUNC! [+] Exit successfully status = %!STATUS!.", status);

	return status;
}

void EvtWdfDeviceCleanup(WDFOBJECT WdfDevice)
{
	UNREFERENCED_PARAMETER(WdfDevice);
	
}