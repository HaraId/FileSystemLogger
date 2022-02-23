#include "Main.h"
#include "Main.tmh"

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

PFLT_FILTER gFilterHandle;

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, BaseFileSystemFilterUnload)
#pragma alloc_text(PAGE, BaseFileSystemFilterInstanceQueryTeardown)
#pragma alloc_text(PAGE, BaseFileSystemFilterInstanceSetup)
#pragma alloc_text(PAGE, BaseFileSystemFilterInstanceTeardownStart)
#pragma alloc_text(PAGE, BaseFileSystemFilterInstanceTeardownComplete)
#endif

extern FilterGlobalContext g_FilterContext;

//
//  operation registration
//

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_CREATE,
      0,
      BaseFileSystemFilterPreOperation,
      BaseFileSystemFilterPostOperation },

    { IRP_MJ_OPERATION_END }
};

//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version

    // NPFS_MSFS - умение фильтровать именованные каналы и почтовые слоты
    // DAX_VOLUME - умение фильтовать тома прямого доступа
    FLTFL_REGISTRATION_SUPPORT_NPFS_MSFS | FLTFL_REGISTRATION_SUPPORT_DAX_VOLUME, //  Flags                                 

    NULL,                               //  Context
    Callbacks,                          //  Operation callbacks

    BaseFileSystemFilterUnload,                           //  MiniFilterUnload

    BaseFileSystemFilterInstanceSetup,                    //  InstanceSetup
    BaseFileSystemFilterInstanceQueryTeardown,            //  InstanceQueryTeardown
    BaseFileSystemFilterInstanceTeardownStart,            //  InstanceTeardownStart
    BaseFileSystemFilterInstanceTeardownComplete,         //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent

};



NTSTATUS
BaseFileSystemFilterInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
/*++

Routine Description:

    This routine is called whenever a new instance is created on a volume. This
    gives us a chance to decide if we need to attach to this volume or not.

    If this routine is not defined in the registration structure, automatic
    instances are always created.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Flags describing the reason for this attach request.

Return Value:

    STATUS_SUCCESS - attach
    STATUS_FLT_DO_NOT_ATTACH - do not attach

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, WPP_DRIVER_TRACE, "%!FUNC! Entry");

    return STATUS_SUCCESS;
}


NTSTATUS
BaseFileSystemFilterInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This is called when an instance is being manually deleted by a
    call to FltDetachVolume or FilterDetach thereby giving us a
    chance to fail that detach request.

    If this routine is not defined in the registration structure, explicit
    detach requests via FltDetachVolume or FilterDetach will always be
    failed.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Indicating where this detach request came from.

Return Value:

    Returns the status of this operation.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, WPP_DRIVER_TRACE, "%!FUNC! Entry");

    return STATUS_SUCCESS;
}


VOID
BaseFileSystemFilterInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This routine is called at the start of instance teardown.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Reason why this instance is being deleted.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, WPP_DRIVER_TRACE, "%!FUNC! Entry");
}


VOID
BaseFileSystemFilterInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This routine is called at the end of instance teardown.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Reason why this instance is being deleted.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, WPP_DRIVER_TRACE, "%!FUNC! Entry");
}


/*************************************************************************
    MiniFilter initialization and unload routines.
*************************************************************************/

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for this miniFilter driver.  This
    registers with FltMgr and initializes all global data structures.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.

    RegistryPath - Unicode string identifying where the parameters for this
        driver are located in the registry.

Return Value:

    Routine can return non success error codes.

--*/
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES driverAttributes;
    WDF_DRIVER_CONFIG driverConfig;
    WDFDRIVER driver;

    //
    // Инициализируем WPP Tracing
    //
    WPP_INIT_TRACING(DriverObject, RegistryPath);
    TraceEvents(TRACE_LEVEL_INFORMATION, WPP_DRIVER_TRACE, "%!FUNC! Entry");

    //
    // Инициализируем глобальный контекст драйвера
    //
    FilterGlobalContext::init(g_FilterContext);

    //
    // Инициализация wdf драйвера
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&driverAttributes);
    driverAttributes.EvtCleanupCallback = EvtWdfDriverContextCleanup;

    WDF_DRIVER_CONFIG_INIT(&driverConfig, WDF_NO_EVENT_CALLBACK);
    driverConfig.DriverPoolTag = DRIVER_POOL_TAG;
    driverConfig.DriverInitFlags |= WdfDriverInitNonPnpDriver;
    driverConfig.EvtDriverUnload = EvtWdfDriverUnload;

    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        &driverAttributes,
        &driverConfig,
        &driver
    );
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_INFORMATION, WPP_DRIVER_TRACE, "%!FUNC! [-] Driver not created");
        WPP_CLEANUP(DriverObject);
    }
    TraceEvents(TRACE_LEVEL_INFORMATION, WPP_DRIVER_TRACE, "%!FUNC! [+] Driver created");

    //
    // Создание управляющего устройства
    //
    status = CreateControlDeviceObject(driver);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_INFORMATION, WPP_DRIVER_TRACE, "%!FUNC! [-] CDO fatal error, status = %!STATUS!.", status);
        WPP_CLEANUP(DriverObject);
    }

    //
    //  Регистрация фильтра файловой системы
    //

    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &gFilterHandle );

    

    FLT_ASSERT( NT_SUCCESS( status ) );

    if (NT_SUCCESS( status )) {

        //
        //  Start filtering i/o
        //

        status = FltStartFiltering( gFilterHandle );

        if (!NT_SUCCESS( status )) {

            FltUnregisterFilter( gFilterHandle );
            TraceEvents(TRACE_LEVEL_INFORMATION, WPP_DRIVER_TRACE, "%!FUNC! [-] Filter Unregistry");

            WPP_CLEANUP(DriverObject);
            return status;
        }

        TraceEvents(TRACE_LEVEL_INFORMATION, WPP_DRIVER_TRACE, "%!FUNC! [+] Filter successfully registered");
    }
    else {
        TraceEvents(TRACE_LEVEL_INFORMATION, WPP_DRIVER_TRACE, "%!FUNC! [-] Filter registration error, status = %!STATUS!.", status);
    }

    return status;
}


NTSTATUS
BaseFileSystemFilterUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
/*++

Routine Description:

    This is the unload routine for this miniFilter driver. This is called
    when the minifilter is about to be unloaded. We can fail this unload
    request if this is not a mandatory unload indicated by the Flags
    parameter.

Arguments:

    Flags - Indicating if this is a mandatory unload.

Return Value:

    Returns STATUS_SUCCESS.

--*/
{
    UNREFERENCED_PARAMETER( Flags );
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, WPP_DRIVER_TRACE, "%!FUNC! Entry");

    FltUnregisterFilter( gFilterHandle );

    return STATUS_SUCCESS;
}

void EvtWdfDriverContextCleanup(WDFOBJECT WdfDriver)
{
    PAGED_CODE();
    TraceEvents(TRACE_LEVEL_INFORMATION, WPP_DRIVER_TRACE, "%!FUNC! Entry");

    FilterGlobalContext::free(g_FilterContext);

    WPP_CLEANUP(WdfDriver);
}

void EvtWdfDriverUnload(WDFDRIVER Driver) 
{
    UNREFERENCED_PARAMETER(Driver);
    PAGED_CODE();
}