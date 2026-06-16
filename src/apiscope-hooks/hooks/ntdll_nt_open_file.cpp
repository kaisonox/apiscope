#include "api_hook.h"
#include "trace_transport.h"

DEFINE_API_HOOK(
    NtOpenFile,
    "ntdll.dll",
    "NtOpenFile",
    NTSTATUS,
    NTAPI,
    HANDLE* FileHandle,
    ACCESS_MASK DesiredAccess,
    OBJECT_ATTRIBUTES* ObjectAttributes,
    IO_STATUS_BLOCK* IoStatusBlock,
    ULONG ShareAccess,
    ULONG OpenOptions) {
    HookCallGuard hookCall;
    TraceEvent event;
    InitializeTraceEvent(&event, "ntdll.dll", "NtOpenFile");
    AddTraceUInt32(&event, "desired_access", DesiredAccess);
    AddTraceObjectPath(&event, "path", ObjectAttributes);
    AddTracePointer(&event, "io_status_block", IoStatusBlock);
    AddTraceUInt32(&event, "share_access", ShareAccess);
    AddTraceUInt32(&event, "open_options", OpenOptions);

    NTSTATUS result = CALL_ORIGINAL(
        NtOpenFile,
        FileHandle,
        DesiredAccess,
        ObjectAttributes,
        IoStatusBlock,
        ShareAccess,
        OpenOptions);
    HANDLE openedHandle = (NT_SUCCESS(result) && FileHandle) ? *FileHandle : nullptr;
    AddTracePointer(&event, "file_handle", openedHandle);
    AddTraceStatus(&event, "result", result);
    EmitTraceEvent(&event);
    return result;
}
