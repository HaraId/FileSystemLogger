#include "ControlDeviceQueue.h"
#include "ControlDeviceQueue.tmh"

#include "ControlDevice.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, EvtWdfIoQueueIoRead)
#pragma alloc_text(PAGE, EvtWdfIoQueueIoDeviceControl)
#endif

extern FilterGlobalContext g_FilterContext;


void EvtWdfIoQueueIoRead(
	WDFQUEUE Queue,
	WDFREQUEST Request,
	size_t Length
)
{
	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(Length);

	NTSTATUS status;
	UCHAR* buffer;
	size_t bufferSize;
	size_t bufferWriteOffset = 0;

	TraceEvents(TRACE_LEVEL_INFORMATION, WPP_QUEUE_TRACE, "%!FUNC! Entry");

	status = WdfRequestRetrieveOutputBuffer(Request, MIN_RECIVE_BUFFER_SIZE, (PVOID*)&buffer, &bufferSize);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_INFORMATION, WPP_QUEUE_TRACE, "%!FUNC! WdfRequestRetrieveInputBuffer error-status =  %!STATUS!.", status);
		WdfRequestComplete(Request, STATUS_INSUFFICIENT_RESOURCES);
		return;
	}


	AutoLock<FastMutex> lock(g_FilterContext.contextLock);

	while (g_FilterContext.logItemlistSize > 0)
	{
		if (IsListEmpty(&g_FilterContext.logItemList)) {
			TraceEvents(TRACE_LEVEL_INFORMATION, WPP_QUEUE_TRACE, "%!FUNC! Critical error: logItemlistSize != 0 for empty list.");
			break;
		}

		// Получаем последний элемент в списке
		const FsOperationEventItem* item = (FsOperationEventItem*)CONTAINING_RECORD(g_FilterContext.logItemList.Blink, FsOperationEventItem, list);

		// Если места в пользовательском буфере больше не осталось, то переходим к отправке
		if (item->data.size > bufferSize - bufferWriteOffset)
			break;

		// Удаляем последний элемент из списка
		PLIST_ENTRY entry = RemoveTailList(&g_FilterContext.logItemList);
		g_FilterContext.logItemlistSize--;

		item = (FsOperationEventItem*)CONTAINING_RECORD(entry, FsOperationEventItem, list);
		
		RtlCopyMemory(PUCHAR(buffer) + bufferWriteOffset, PVOID(&(item->data)), item->data.size);
		bufferWriteOffset += item->data.size;
		
		ExFreePool(PVOID(item));
	}


	WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, bufferWriteOffset);
}


void EvtWdfIoQueueIoDeviceControl(
	WDFQUEUE Queue,
	WDFREQUEST Request,
	size_t OutputBufferLength,
	size_t InputBufferLength,
	ULONG IoControlCode
)
{
	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(OutputBufferLength);

	NTSTATUS status = STATUS_SUCCESS;
	PCHAR buffer = nullptr;
	size_t bufferSize = 0;


	TraceEvents(TRACE_LEVEL_INFORMATION, WPP_QUEUE_TRACE, "%!FUNC! Entry");

	if (IOCTL_SET_TARGET_APP == IoControlCode)
	{
		if (!InputBufferLength)
		{
			TraceEvents(TRACE_LEVEL_INFORMATION, WPP_QUEUE_TRACE, "%!FUNC! (IOCTL_SET_TARGET_APP) input buffer empty.");
			WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
			return;
		}

		status = WdfRequestRetrieveInputBuffer(Request, 0, (PVOID*)&buffer, &bufferSize);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_INFORMATION, WPP_QUEUE_TRACE, "%!FUNC! (IOCTL_SET_TARGET_APP) error-status =  %!STATUS!.", status);
			WdfRequestComplete(Request, STATUS_INSUFFICIENT_RESOURCES);
			return;
		}

		if (InputBufferLength != bufferSize || bufferSize > MAX_TARGET_FILE_NAME_SIZE)
		{
			TraceEvents(TRACE_LEVEL_INFORMATION, WPP_QUEUE_TRACE, "%!FUNC! (IOCTL_SET_TARGET_APP) InputBufferLength != bufferSize.");
			WdfRequestComplete(Request, STATUS_INSUFFICIENT_RESOURCES);
			return;
		}

		ANSI_STRING ansiAppName;
		ansiAppName.Length = ansiAppName.MaximumLength = USHORT(InputBufferLength);
		ansiAppName.Buffer = buffer;
		
		AutoLock<FastMutex>(g_FilterContext.contextLock);

		g_FilterContext.isStoped = true;

		g_FilterContext.clearTargetAppName();

		RtlAnsiStringToUnicodeString(&g_FilterContext.targetApplicationName, &ansiAppName, TRUE); // auto memory allocation
	}

	else if (IOCTL_CLEAR == IoControlCode)
	{

	}

	else if (IOCTL_START_LOG == IoControlCode)
	{
		AutoLock<FastMutex>(g_FilterContext.contextLock);
		g_FilterContext.isStoped = false;
	}

	else if (IOCTL_STOP_LOG == IoControlCode)
	{
		AutoLock<FastMutex>(g_FilterContext.contextLock);
		g_FilterContext.isStoped = true;
	}


	WdfRequestComplete(Request, STATUS_SUCCESS);
}