#include "api_hook.h"
#include "trace_transport.h"

DEFINE_API_HOOK(
    NtOpenKey,
    "ntdll.dll",
    "NtOpenKey",
    NTSTATUS,
    NTAPI,
    HANDLE* KeyHandle,
    ACCESS_MASK DesiredAccess,
    OBJECT_ATTRIBUTES* ObjectAttributes) {
    HookCallGuard hookCall;
    TraceEvent event;
    InitializeTraceEvent(&event, "ntdll.dll", "NtOpenKey");
    AddTraceUInt32(&event, "desired_access", DesiredAccess);
    AddTraceObjectPath(&event, "path", ObjectAttributes);

    NTSTATUS result = CALL_ORIGINAL(
        NtOpenKey,
        KeyHandle,
        DesiredAccess,
        ObjectAttributes);
    HANDLE openedHandle = (NT_SUCCESS(result) && KeyHandle) ? *KeyHandle : nullptr;
    AddTracePointer(&event, "key_handle", openedHandle);
    AddTraceStatus(&event, "result", result);
    EmitTraceEvent(&event);
    return result;
}
