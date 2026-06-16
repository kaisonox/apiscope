#include "api_hook.h"
#include "trace_transport.h"

DEFINE_API_HOOK(
    NtSetValueKey,
    "ntdll.dll",
    "NtSetValueKey",
    NTSTATUS,
    NTAPI,
    HANDLE KeyHandle,
    UNICODE_STRING* ValueName,
    ULONG TitleIndex,
    ULONG Type,
    PVOID Data,
    ULONG DataSize) {
    HookCallGuard hookCall;
    TraceEvent event;
    InitializeTraceEvent(&event, "ntdll.dll", "NtSetValueKey");
    AddTracePointer(&event, "key_handle", KeyHandle);
    AddTraceUnicodeString(&event, "value_name", ValueName);
    AddTraceUInt32(&event, "type", Type);
    AddTraceUInt32(&event, "data_size", DataSize);
    AddTraceBufferPreview(&event, "data", Data, DataSize);

    NTSTATUS result = CALL_ORIGINAL(
        NtSetValueKey,
        KeyHandle,
        ValueName,
        TitleIndex,
        Type,
        Data,
        DataSize);
    AddTraceStatus(&event, "result", result);
    EmitTraceEvent(&event);
    return result;
}
