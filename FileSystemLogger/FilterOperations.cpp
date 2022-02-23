#include "FilterOperations.h"
#include "FilterOperations.tmh"

#include "ControlDevice.h"

ULONG_PTR OperationStatusCtx = 1;

#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002

ULONG gTraceFlags = 1;

#define PT_DBG_PRINT( _dbgLevel, _string )          \
        DbgPrint _string 

QUERY_INFO_PROCESS ZwQueryInformationProcess = NULL;

static int thread_blocker = 0;

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

    PEPROCESS processInst;
    HANDLE hProcess;

    const size_t MAX_PROCESS_IMAGE_NAME_SIZE = 512;
    ULONG retProcessImageNameSize;
    PUNICODE_STRING processImageName;

    PFLT_FILE_NAME_INFORMATION fileNameInformation;


    // Не используем трассировку из-за большого количества событий
    //TraceEvents(TRACE_LEVEL_INFORMATION, WPP_FILTER_TRACE, "%!FUNC! Entry");

    if (Data->RequestorMode == KernelMode)
        return FLT_PREOP_SUCCESS_NO_CALLBACK;


    // Получаем экземпляр процесса по экземпляру потока, который иниировал операцию i/o
    processInst = PsGetThreadProcess(Data->Thread);

    // Получаем дескриптор процесса
    status = ObOpenObjectByPointer(
        processInst,       // Экземпляр объекта
        OBJ_KERNEL_HANDLE, // (HandleAttributes), флаг, говорит, что дескриптор может использоваться только из пространства ядра
        NULL,              // (PassedAccessState). В драйверах это поле обычно не используется
        NULL,              // (DesiredAccess). Маска доступа, если аргумент AccessMode = KernelMode то это поле может быть равно NULL
        *PsProcessType,    // (Object Type)
        KernelMode,        // (AccessMode), режим доступа. 
        &hProcess          // [ out ] дескриптор
    );

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_INFORMATION, WPP_FILTER_TRACE, "%!FUNC! ObOpenObjectByPointer return-status = %!STATUS!.", status);
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    // Поиск функции ZwQueryInformationProcess
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

    processImageName = (PUNICODE_STRING)ExAllocatePool(PagedPool, MAX_PROCESS_IMAGE_NAME_SIZE);
    if (processImageName == NULL) {
        TraceEvents(TRACE_LEVEL_INFORMATION, WPP_FILTER_TRACE, "%!FUNC! ExAllocatePool, buffer for process name not allocated.");
        ZwClose(hProcess);
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    RtlSecureZeroMemory(PVOID(processImageName), MAX_PROCESS_IMAGE_NAME_SIZE);

    status = ZwQueryInformationProcess(
        hProcess,                           // Дескриптор процесса информацию о котором мы хотим получить
        ProcessImageFileName,               // Класс (тип) запрашиваемой информации
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

    // Закрываем дескриптор процесса, он больше не понадобится
    ZwClose(hProcess);
    
    // Получение идентификатора процесса:
    //DbgPrint("Process ID: %d", HandleToULong(PsGetThreadProcessId(Data->Thread)));

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

        // Не задано имя приложения, для которого требуется вести лог
        if (g_FilterContext.targetApplicationName.Buffer == nullptr)
            break;

        // Список событий уже переполнен
        if (g_FilterContext.logItemlistSize >= 3000)
            break;

        // Данный процесс не является целевым
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
    

    //
    //  See if this is an operation we would like the operation status
    //  for.  If so request it.
    //
    //  NOTE: most filters do NOT need to do this.  You only need to make
    //        this call if, for example, you need to know if the oplock was
    //        actually granted.
    //

    /*if (BaseFileSystemFilterDoRequestOperationStatus(Data)) {

        status = FltRequestOperationStatusCallback(Data,
            BaseFileSystemFilterOperationStatusCallback,
            (PVOID)(++OperationStatusCtx));
        if (!NT_SUCCESS(status)) {

            PT_DBG_PRINT(PTDBG_TRACE_OPERATION_STATUS,
                ("BaseFileSystemFilter!BaseFileSystemFilterPreOperation: FltRequestOperationStatusCallback Failed, status=%08x\n",
                    status));
        }
    }*/

    // This template code does not do anything with the callbackData, but
    // rather returns FLT_PREOP_SUCCESS_WITH_CALLBACK.
    // This passes the request down to the next miniFilter in the chain.

    //  FLT_PREOP_COMPLETE — означает, что драйвер завершает операцию.  и не передает запрос мини-фильтрам нижних уровней.
    //  FLT_PREOP_SUCCESS_NO_CALLBACK — означает, что функция обратного вызова перед операцией завершила обработку запроса и позволяет ему перейти
        // к следующему фильтру.Драйвер не хочет, чтобы для этой операции была
        // активизирована функция обратного вызова после операции
    //  FLT_PREOP_SUCCESS_WITH_CALLBACK — означает, что драйвер позволяет диспетчеру фильтров передать запрос фильтрам нижних уровней, но он хочет, 
        // чтобы для этой операции была активизирована функция обратного вызова после операции.
    //  FLT_PREOP_PENDING — означает, что драйвер приостанавливает операцию. Диспетчер фильтров не продолжает обработку запроса до того момента,
        // когда драйвер вызовет FltCompletePendedPreOperation; этот вызов сообщает диспетчеру фильтров, что обработку запроса можно продолжить.
    //  FLT_PREOP_SYNCHRONIZE — аналог FLT_PREOP_SUCCESS_WITH_CALLBACK, но драйвер приказывает диспетчеру фильтров активизировать свою функцию обратного вызова после операции в том же потоке на уровне IRQL <= APC_LEVEL
        // (обычно функция обратного вызова после операции может активизироваться на уровне IRQL <= DISPATCH_LEVEL в произвольном потоке).

    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}



VOID
BaseFileSystemFilterOperationStatusCallback(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
    _In_ NTSTATUS OperationStatus,
    _In_ PVOID RequesterContext
)
/*++

Routine Description:

    This routine is called when the given operation returns from the call
    to IoCallDriver.  This is useful for operations where STATUS_PENDING
    means the operation was successfully queued.  This is useful for OpLocks
    and directory change notification operations.

    This callback is called in the context of the originating thread and will
    never be called at DPC level.  The file object has been correctly
    referenced so that you can access it.  It will be automatically
    dereferenced upon return.

    This is non-pageable because it could be called on the paging path

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    RequesterContext - The context for the completion routine for this
        operation.

    OperationStatus -

Return Value:

    The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER(FltObjects);

    PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
        ("BaseFileSystemFilter!BaseFileSystemFilterOperationStatusCallback: Entered\n"));

    PT_DBG_PRINT(PTDBG_TRACE_OPERATION_STATUS,
        ("BaseFileSystemFilter!BaseFileSystemFilterOperationStatusCallback: Status=%08x ctx=%p IrpMj=%02x.%02x \"%s\"\n",
            OperationStatus,
            RequesterContext,
            ParameterSnapshot->MajorFunction,
            ParameterSnapshot->MinorFunction,
            FltGetIrpName(ParameterSnapshot->MajorFunction)));
}


FLT_POSTOP_CALLBACK_STATUS
BaseFileSystemFilterPostOperation(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
)
/*++

Routine Description:

    This routine is the post-operation completion routine for this
    miniFilter.

    This is non-pageable because it may be called at DPC level.

    Что нужно знать про эту функуию:
        СЛУЧАЙ (1) - Pre opretion завершился со статусом отличным от FLT_PREOP_SYNCHRONIZE:

            Функция вызывается в контексте произвольного потока при IRQL <= DISPATCH_LEVEL

            1) Драйвер не может обращаться к выгружаемой памяти
            2) Не может использовать функции по ограничению irql
            3) не может захватывать объекты синхронизации
            4) не может устанавливать, получать или удалять контексты

            Что делать:
            1) FltDoCompletionProcessongWhenSafe
            2) Рабочие элементы (FltQueueDeferredIoWorkItem)

        СЛУЧАЙ (2) - Pre operation завершился со статусом FLT_PREOP_SYNCHRONIZE:

            Функция вызывается в контексте потока в котором была вызвана Pre operaion при IRQL <= APC_LEVEL

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The completion context set in the pre-operation routine.

    Flags - Denotes whether the completion is successful or is being drained.

Return Value:

    The return value is the status of the operation.

    OR

    FLT_POSTOP_MORE_PROCESSING_REQUIRED -> FltCompletePendedPostOperation

--*/
{
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);

    PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
        ("BaseFileSystemFilter!BaseFileSystemFilterPostOperation: Entered\n"));

    return FLT_POSTOP_FINISHED_PROCESSING;
}


BOOLEAN
BaseFileSystemFilterDoRequestOperationStatus(
    _In_ PFLT_CALLBACK_DATA Data
)
/*++

Routine Description:

    This identifies those operations we want the operation status for.  These
    are typically operations that return STATUS_PENDING as a normal completion
    status.

Arguments:

Return Value:

    TRUE - If we want the operation status
    FALSE - If we don't

--*/
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;

    //
    //  return boolean state based on which operations we are interested in
    //

    return (BOOLEAN)

        //
        //  Check for oplock operations
        //

        (((iopb->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
            ((iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_FILTER_OPLOCK) ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_BATCH_OPLOCK) ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_1) ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2)))

            ||

            //
            //    Check for directy change notification
            //

            ((iopb->MajorFunction == IRP_MJ_DIRECTORY_CONTROL) &&
                (iopb->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY))
            );
}
