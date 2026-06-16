#include "api_hook.h"
#include "trace_transport.h"

DEFINE_API_HOOK(
    NtQueryValueKey,
    "ntdll.dll",
    "NtQueryValueKey",
    NTSTATUS,
    NTAPI,
    HANDLE KeyHandle,
    UNICODE_STRING* ValueName,
    ULONG KeyValueInformationClass,
    PVOID KeyValueInformation,
    ULONG Length,
    ULONG* ResultLength) {
    HookCallGuard hookCall;
    TraceEvent event;
    InitializeTraceEvent(&event, "ntdll.dll", "NtQueryValueKey");
    AddTracePointer(&event, "key_handle", KeyHandle);
    AddTraceUnicodeString(&event, "value_name", ValueName);
    AddTraceUInt32(&event, "info_class", KeyValueInformationClass);
    AddTraceUInt32(&event, "length", Length);

    NTSTATUS result = CALL_ORIGINAL(
        NtQueryValueKey,
        KeyHandle,
        ValueName,
        KeyValueInformationClass,
        KeyValueInformation,
        Length,
        ResultLength);
    AddTraceStatus(&event, "result", result);
    EmitTraceEvent(&event);
    return result;
}
