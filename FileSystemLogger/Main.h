#pragma once

#include "FilterOperations.h"

#include "ControlDevice.h"

#define DRIVER_POOL_TAG 'FSLD'

EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
);

NTSTATUS
BaseFileSystemFilterInstanceSetup(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
);

VOID
BaseFileSystemFilterInstanceTeardownStart(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
);

VOID
BaseFileSystemFilterInstanceTeardownComplete(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
);

NTSTATUS
BaseFileSystemFilterUnload(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
);

NTSTATUS
BaseFileSystemFilterInstanceQueryTeardown(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
);

EVT_WDF_DRIVER_UNLOAD EvtWdfDriverUnload;
EVT_WDF_OBJECT_CONTEXT_CLEANUP EvtWdfDriverContextCleanup;

EXTERN_C_END


/*
    Пару слов про имя файла:
    
    Том - имя усройства, на которое ссылается символическая ссылка, 
        например: "C:" -> "\Device\HarddiskVolume3"



*/

/*

typedef struct _FLT_CALLBACK_DATA {
    FLT_CALLBACK_DATA_FLAGS Flags;
    PETHREAD CONST Thread;                     // не менять
    PFLT_IO_PARAMETER_BLOCK CONST Iopb;
    IO_STATUS_BLOCK IoStatus;
    
    struct _FLT_TAG_DATA_BUFFER *TagData;
    
    union {
        struct {
            LIST_ENTRY QueueLinks;
            PVOID QueueContext[2];
        };
        PVOID FilterContext[4];
    };
    KPROCESSOR_MODE RequestorMode;              // не менять
} FLT_CALLBACK_DATA, *PFLT_CALLBACK_DATA;

    .Flags :
        1) FLTFL_CALLBACK_DATA_DIRTY - драйвер внес изменения в структуру и вызвал FltSetCallbackDataDirty. Изменить можно каждое поле структуры, кроме Thread и RequestorMode.
        2) FLTFL_CALLBACK_DATA_FAST_IO_OPERATION — означает, что операция является операцией быстрого ввода/вывода.
        3) FLTFL_CALLBACK_DATA_IRP_OPERATION — означает, что операция является операцией на базе IRP.
        4) FLTFL_CALLBACK_DATA_GENERATED_IO — означает, что операция сгенерирована другим мини-фильтром.
        5) FLTFL_CALLBACK_DATA_POST_OPERATION — означает, что обратный активизируется после операции (а не перед операцией).

    . Thread — непрозрачный указатель на поток, который запросил выполнение операции.

    . IoStatus — статус запроса. Обратный вызов перед операцией может задать 
        это значение, а затем указать, что обработка перед операцией завершена, вернув FLT_PREOP_COMPLETE. Обратный вызов после операции может проверить 
        итоговый статус операции.
    
    .  RequestorMode — сообщает, поступил ли запрос из пользовательского режима (UserMode) или режима ядра (KernelMode).

    . Iopb — структура, содержащая подробные параметры запроса. Определение структуры выглядит так:

    {
        ULONG IrpFlags;
        UCHAR MajorFunction;
        UCHAR MinorFunction;
        UCHAR OperationFlags;
        UCHAR Reserved;
        PFILE_OBJECT TargetFileObject;
        PFLT_INSTANCE TargetInstance;
        FLT_PARAMETERS Parameters;
    } FLT_IO_PARAMETER_BLOCK, *PFLT_IO_PARAMETER_BLOCK;

        .. TargetFileObject — объект файла, который является целью операции; значение может быть полезно при вызове некоторых функций API.

        .. Parameters — огромное объединение с данными конкретной операции (на 
            концептуальном уровне напоминает поле Parameters структуры IO_STACK_LOCATION). Чтобы получить необходимую информацию, драйвер проверяет 
            соответствующую структуру в этом объединении. Некоторые из этих структур будут рассмотрены позднее в этой главе, когда мы займемся конкретными типами операций.




    typedef struct _FLT_RELATED_OBJECTS {

        USHORT CONST Size;
        USHORT CONST TransactionContext;            //TxF mini-version
        PFLT_FILTER CONST Filter;
        PFLT_VOLUME CONST Volume;
        PFLT_INSTANCE CONST Instance;
        PFILE_OBJECT CONST FileObject;
        PKTRANSACTION CONST Transaction;

    } FLT_RELATED_OBJECTS, *PFLT_RELATED_OBJECTS;
*/



/*

Возможные операции:

    { IRP_MJ_CREATE,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_CREATE_NAMED_PIPE,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_CLOSE,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_READ,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_WRITE,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_QUERY_INFORMATION,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

      // Информационные настройки файла (переименоваание, удаление)
    { IRP_MJ_SET_INFORMATION,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

      // чтение расширенных атрибутов из файла/каталога
    { IRP_MJ_QUERY_EA,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_SET_EA,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_FLUSH_BUFFERS,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_QUERY_VOLUME_INFORMATION,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_SET_VOLUME_INFORMATION,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

      // запроск директории
    { IRP_MJ_DIRECTORY_CONTROL,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_FILE_SYSTEM_CONTROL,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_DEVICE_CONTROL,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_INTERNAL_DEVICE_CONTROL,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_SHUTDOWN,
      0,
      BaseFileSystemFilterPreOperationNoPostOperation,
      NULL },                               //post operations not supported

    { IRP_MJ_LOCK_CONTROL,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_CLEANUP,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_CREATE_MAILSLOT,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_QUERY_SECURITY,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_SET_SECURITY,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_QUERY_QUOTA,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_SET_QUOTA,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_PNP,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_ACQUIRE_FOR_MOD_WRITE,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_RELEASE_FOR_MOD_WRITE,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_ACQUIRE_FOR_CC_FLUSH,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_RELEASE_FOR_CC_FLUSH,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_NETWORK_QUERY_OPEN,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_MDL_READ,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_MDL_READ_COMPLETE,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_PREPARE_MDL_WRITE,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_MDL_WRITE_COMPLETE,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_VOLUME_MOUNT,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_VOLUME_DISMOUNT,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },*/
