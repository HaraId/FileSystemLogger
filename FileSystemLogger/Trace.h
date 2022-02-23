#pragma once


//
// Define the tracing flags.
//
// Tracing GUID - 9232A4C6-C44B-48A2-AF41-B84BE63AB4AF
//

#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(FileSystemLoggerTraceGuid, (9232A4C6,C44B,48A2,AF41,B84BE63AB4AF),  \
        WPP_DEFINE_BIT(WPP_MYDRIVER_ALL_INFO)  \
        WPP_DEFINE_BIT(WPP_DRIVER_TRACE)   \
        WPP_DEFINE_BIT(WPP_DEVICE_TRACE)   \
        WPP_DEFINE_BIT(WPP_QUEUE_TRACE)    \
        WPP_DEFINE_BIT(WPP_FILTER_TRACE) )


#define WPP_FLAG_LEVEL_LOGGER(flag, level)                                  \
    WPP_LEVEL_LOGGER(flag)

#define WPP_FLAG_LEVEL_ENABLED(flag, level)                                 \
    (WPP_LEVEL_ENABLED(flag) &&                                             \
     WPP_CONTROL(WPP_BIT_ ## flag).Level >= level)

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) \
           WPP_LEVEL_LOGGER(flags)

#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
           (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

//           
// WPP orders static parameters before dynamic parameters. To support the Trace function
// defined below which sets FLAGS=MYDRIVER_ALL_INFO, a custom macro must be defined to
// reorder the arguments to what the .tpl configuration file expects.
//
#define WPP_RECORDER_FLAGS_LEVEL_ARGS(flags, lvl) WPP_RECORDER_LEVEL_FLAGS_ARGS(lvl, flags)
#define WPP_RECORDER_FLAGS_LEVEL_FILTER(flags, lvl) WPP_RECORDER_LEVEL_FLAGS_FILTER(lvl, flags)

//
// This comment block is scanned by the trace preprocessor to define our
// Trace function.
//
// begin_wpp config
// FUNC Trace{FLAGS=MYDRIVER_ALL_INFO}(LEVEL, MSG, ...);
// FUNC TraceEvents(LEVEL, FLAGS, MSG, ...);
// end_wpp
//


/*

%!FILE!	    Displays the name of the source file from which the trace message was generated. This variable can also be used in the trace message prefix.
%!FLAGS!	Displays the value of the trace flags that enable the trace message. This variable can also be used in the trace message prefix.
%!FUNC!	    Displays the function that generated the trace message. This variable can also be used in the trace message prefix.
%!LEVEL!	Displays the name of the trace level that enables the trace message. This variable can also be used in the trace message prefix.
%!LINE!	    Displays the line number of the line in the code that generated the trace prefix. This variable can also be used in the trace message prefix.

%!bool!	Displays TRUE or FALSE
%!irql!	Displays the name of the current IRQL.
%!sid!	Represents a pointer to Security Identifier (pSID). Displays the SID.

GUIDs:
%!GUID!	Represents a pointer to a GUID (pGUID). Displays the GUID that is pointed to.
%!CLSID!	Class ID. Represents a pointer to a class ID GUID. Displays the string associated with the GUID. WPP locates the string in the registry when it formats the trace messages.
%!LIBID!	Type library. Represents the GUID of a COM type library. Displays the string associated with the GUID. WPP locates the string in the registry when it formats the trace messages.
%!IID!	Interface ID. Represents a pointer to an interface ID GUID. Displays the string associated with the GUID. WPP locates the string in the registry when it formats the trace messages.

Time:
%!delta!	Displays the difference between two time values, in milliseconds. It is a LONGLONG value that is displayed in day~h:m:s format.
%!WAITTIME!	Displays the time that was spent waiting for something to be completed, in milliseconds. It is a LONGLONG value that is displayed in day~h:m:s format. Designed to be used with %!due!.
%!due!	Displays the time that something is expected to be completed, in milliseconds. It is a LONGLONG value that is displayed in day~h:m:s format. Designed to be used with %!WAITTIME!.
%!TIMESTAMP!
%!datetime!
%!TIME!	Displays the value of system time at a particular moment. These are LONGLONG (SINT64) values that are displayed in SYSTEMTIME format.
You can use these variables to represent different time values in your program and to distinguish among them.


%!STATUS!	Represents a status value and displays the string associated with the status code.
%!WINERROR!	Represents a Windows error code and displays the string associated with the error.
%!HRESULT!	Represents an error or warning and displays the code in HRESULT format.



%!IPADDR!	Represents a pointer to an IP address. Displays the IP address.
%!PORT!	Displays a port number.

*/