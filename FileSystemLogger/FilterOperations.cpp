#include "FilterOperations.h"
#include "FilterOperations.tmh"

#include "ControlDevice.h"


QUERY_INFO_PROCESS ZwQueryInformationProcess = NULL;

// Global drier context
extern FilterGlobalContext g_FilterContext;



/***************************************************************************************************************************************************************************************************************************
    MiniFilter callback routines.
***************************************************************************************************************************************************************************************************************************/
FLT_PREOP_CALLBACK_STATUS
BaseFileSystemFilterPreOperation(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
/*++
https://docs.microsoft.com/en-us/windows-hardware/drivers/ifs/writing-preoperation-callback-routines
Routine Description:

    This routine is a pre-operation dispatch routine for this miniFilter.

    This is non-pageable because it could be called on the paging path

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    NTSTATUS status;

    PEPROCESS oProcess;
    HANDLE hProcess;

    const size_t MAX_PROCESS_IMAGE_NAME_SIZE = 512;
    ULONG retProcessImageNameSize;
    PUNICODE_STRING processImageName;

    PFLT_FILE_NAME_INFORMATION fileNameInformation;


    // Don't use trace methods in this function, only for debugging
    // TraceEvents(TRACE_LEVEL_INFORMATION, WPP_FILTER_TRACE, "%!FUNC! Entry");


    // If the request is initiated by the kernel side, we should not interfere with it
    if (Data->RequestorMode == KernelMode)
        return FLT_PREOP_SUCCESS_NO_CALLBACK;


    //
    // Usually, the IRP_MJ_CREATE procedure is called in the context of the process that initiated the request, 
    // but here I will show you how to use a more universal method.
    //

    // Get process object by thread id that initiated this request
    oProcess = PsGetThreadProcess(Data->Thread);

    // Get process handle
    status = ObOpenObjectByPointer(
        oProcess,       // Ёкземпл€р объекта
        OBJ_KERNEL_HANDLE, // (HandleAttributes), flag, this descriptor can only be used in kernel space.
        NULL,              // (PassedAccessState). Usually, this field not used...
        NULL,              // (DesiredAccess). If AccessMode is KernelMode this mean that this field can be NULL.
        *PsProcessType,    // (Object Type) Type of required object
        KernelMode,        // (AccessMode), Access mode ...
        &hProcess          // [ out ] Process descriptor
    );

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_INFORMATION, WPP_FILTER_TRACE, "%!FUNC! [-] ObOpenObjectByPointer status = %!STATUS!.", status);
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    // Search function ZwQueryInformationProcess
    // ::TODO:: move this code in main function section 
    if (NULL == ZwQueryInformationProcess) {
        UNICODE_STRING routineName;

        RtlInitUnicodeString(&routineName, L"ZwQueryInformationProcess");

        ZwQueryInformationProcess =
            (QUERY_INFO_PROCESS)MmGetSystemRoutineAddress(&routineName);

        if (NULL == ZwQueryInformationProcess) {
            TraceEvents(TRACE_LEVEL_INFORMATION, WPP_FILTER_TRACE, "%!FUNC! system routine 'ZwQueryInformationProcess' not found.");
            return FLT_PREOP_SUCCESS_NO_CALLBACK;
        }
    }

    // Allocate memory for process image name
    processImageName = (PUNICODE_STRING)ExAllocatePool(PagedPool, MAX_PROCESS_IMAGE_NAME_SIZE);
    if (processImageName == NULL) {
        TraceEvents(TRACE_LEVEL_INFORMATION, WPP_FILTER_TRACE, "%!FUNC! ExAllocatePool, buffer for process name not allocated.");
        ZwClose(hProcess);
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    RtlSecureZeroMemory(PVOID(processImageName), MAX_PROCESS_IMAGE_NAME_SIZE);

    status = ZwQueryInformationProcess(
        hProcess,                           // ƒескриптор процесса информацию о котором мы хотим получить
        ProcessImageFileName,               //  ласс (тип) запрашиваемой информации
        PVOID(processImageName),
        MAX_PROCESS_IMAGE_NAME_SIZE - sizeof(WCHAR),
        &retProcessImageNameSize
    );

    if (!NT_SUCCESS(status) || processImageName->Length == 0)
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, WPP_FILTER_TRACE, "%!FUNC! ZwQueryInformationProcess return error-code, status = %!STATUS!.", status);
        ExFreePool(processImageName);
        ZwClose(hProcess);
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    // Close process descriptor
    ZwClose(hProcess);
    
    status = FltGetFileNameInformation(
        Data,
        FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
        &fileNameInformation
    );
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, WPP_FILTER_TRACE, "%!FUNC! FltGetFileNameInformation return error-code, status = %!STATUS!.", status);
        ExFreePool(processImageName);
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    
    do {
        status = FltParseFileNameInformation(fileNameInformation);
        if (!NT_SUCCESS(status))
            break;

        AutoLock<FastMutex> lock(g_FilterContext.contextLock);

        if (g_FilterContext.isStoped) 
            break;

        // The name of the target application is not set
        if (g_FilterContext.targetApplicationName.Buffer == nullptr)
            break;

        // Event list is filled
        if (g_FilterContext.logItemlistSize >= 3000)
            break;

        // This process isn't target
        if (wcsstr(PWCH(&processImageName->Buffer), g_FilterContext.targetApplicationName.Buffer) == nullptr)
            break;


        FsOperationEventItem* item = (FsOperationEventItem*)ExAllocatePool(PagedPool, sizeof(FsOperationEventItem) + fileNameInformation->Name.Length);
        if (item == nullptr)
            break;

        item->data.size = sizeof(FsOperationEvent) + fileNameInformation->Name.Length;

        item->data.targetFileNameOffset = sizeof(FsOperationEvent);
        item->data.targetFileNameLength = fileNameInformation->Name.Length;

        RtlCopyMemory(
            PCHAR(&item->data) + item->data.targetFileNameOffset,
            fileNameInformation->Name.Buffer,
            item->data.targetFileNameLength
        );

        
        g_FilterContext.logItemlistSize++;
        InsertTailList(&g_FilterContext.logItemList, &item->list);

        FltReleaseFileNameInformation(fileNameInformation);
    } while (false);

    ExFreePool(processImageName);

    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

//  FLT_PREOP_COMPLETE Ч означает, что драйвер завершает операцию.  и не передает запрос мини-фильтрам нижних уровней.
//  FLT_PREOP_SUCCESS_NO_CALLBACK Ч означает, что функци€ обратного вызова перед операцией завершила обработку запроса и позвол€ет ему перейти
    // к следующему фильтру.ƒрайвер не хочет, чтобы дл€ этой операции была
    // активизирована функци€ обратного вызова после операции
//  FLT_PREOP_SUCCESS_WITH_CALLBACK Ч означает, что драйвер позвол€ет диспетчеру фильтров передать запрос фильтрам нижних уровней, но он хочет, 
    // чтобы дл€ этой операции была активизирована функци€ обратного вызова после операции.
//  FLT_PREOP_PENDING Ч означает, что драйвер приостанавливает операцию. ƒиспетчер фильтров не продолжает обработку запроса до того момента,
    // когда драйвер вызовет FltCompletePendedPreOperation; этот вызов сообщает диспетчеру фильтров, что обработку запроса можно продолжить.
//  FLT_PREOP_SYNCHRONIZE Ч аналог FLT_PREOP_SUCCESS_WITH_CALLBACK, но драйвер приказывает диспетчеру фильтров активизировать свою функцию обратного вызова после операции в том же потоке на уровне IRQL <= APC_LEVEL
    // (обычно функци€ обратного вызова после операции может активизироватьс€ на уровне IRQL <= DISPATCH_LEVEL в произвольном потоке).
