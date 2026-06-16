#include "api_hook.h"
#include "trace_transport.h"

DEFINE_API_HOOK(
    NtClose,
    "ntdll.dll",
    "NtClose",
    NTSTATUS,
    NTAPI,
    HANDLE Handle) {
    HookCallGuard hookCall;
    TraceEvent event;
    InitializeTraceEvent(&event, "ntdll.dll", "NtClose");
    AddTracePointer(&event, "handle", Handle);

    NTSTATUS result = CALL_ORIGINAL(NtClose, Handle);
    AddTraceStatus(&event, "result", result);
    EmitTraceEvent(&event);
    return result;
}
