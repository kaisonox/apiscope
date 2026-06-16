#include "trace_correlator.h"
#include "trace_renderer.h"
#include "trace_transport.h"
#include <sstream>
#include <stdio.h>

static size_t WideLength(size_t arraySize) {
    return arraySize / sizeof(wchar_t) - 1;  // Exclude the terminating null.
}

int main() {
    // A create that observed a handle and path is recorded; a later read on the
    // same handle is annotated with the resolved path.
    HandlePathTracker tracker;
    TraceEvent createEvent;
    InitializeTraceEvent(&createEvent, "ntdll.dll", "NtCreateFile");
    wchar_t path[] = L"C:\\Temp\\scope.txt";
    AddTraceWideString(&createEvent, "path", path, WideLength(sizeof(path)));
    AddTracePointer(&createEvent, "file_handle", (PVOID)0x1234);
    AddTraceStatus(&createEvent, "result", 0);
    tracker.Observe(createEvent);

    TraceEvent readEvent;
    InitializeTraceEvent(&readEvent, "ntdll.dll", "NtReadFile");
    AddTracePointer(&readEvent, "file_handle", (PVOID)0x1234);
    AddTraceUInt32(&readEvent, "length", 16);
    AddTraceStatus(&readEvent, "result", 0);
    uint16_t fieldsBefore = readEvent.header.fieldCount;
    tracker.Annotate(&readEvent);
    if (readEvent.header.fieldCount != fieldsBefore + 1 ||
        !IsValidTraceEvent(readEvent, readEvent.header.size)) {
        printf("Annotate did not add a valid path field\n");
        return 1;
    }
    std::ostringstream readJson;
    RenderTraceEventJsonl(readJson, readEvent);
    if (readJson.str().find("\"path\":\"") == std::string::npos ||
        readJson.str().find("scope.txt") == std::string::npos) {
        printf("Resolved path was not rendered on the read event\n");
        return 1;
    }

    // An unknown handle must not be annotated.
    TraceEvent unknownRead;
    InitializeTraceEvent(&unknownRead, "ntdll.dll", "NtReadFile");
    AddTracePointer(&unknownRead, "file_handle", (PVOID)0x9999);
    uint16_t unknownBefore = unknownRead.header.fieldCount;
    tracker.Annotate(&unknownRead);
    if (unknownRead.header.fieldCount != unknownBefore) {
        printf("Unknown handle should not be annotated\n");
        return 1;
    }

    // A failed create produced no usable handle and must not be recorded.
    HandlePathTracker failedTracker;
    TraceEvent failedCreate;
    InitializeTraceEvent(&failedCreate, "ntdll.dll", "NtCreateFile");
    AddTraceWideString(&failedCreate, "path", path, WideLength(sizeof(path)));
    AddTracePointer(&failedCreate, "file_handle", (PVOID)0x4321);
    AddTraceStatus(&failedCreate, "result", (NTSTATUS)0xC0000022L);  // ACCESS_DENIED
    failedTracker.Observe(failedCreate);
    TraceEvent failedRead;
    InitializeTraceEvent(&failedRead, "ntdll.dll", "NtReadFile");
    AddTracePointer(&failedRead, "file_handle", (PVOID)0x4321);
    uint16_t failedBefore = failedRead.header.fieldCount;
    failedTracker.Annotate(&failedRead);
    if (failedRead.header.fieldCount != failedBefore) {
        printf("Handle from a failed create should not be tracked\n");
        return 1;
    }

    // A relative open chains through a known root_directory handle.
    HandlePathTracker relativeTracker;
    TraceEvent dirCreate;
    InitializeTraceEvent(&dirCreate, "ntdll.dll", "NtCreateFile");
    wchar_t dir[] = L"C:\\Temp";
    AddTraceWideString(&dirCreate, "path", dir, WideLength(sizeof(dir)));
    AddTracePointer(&dirCreate, "file_handle", (PVOID)0x10);
    AddTraceStatus(&dirCreate, "result", 0);
    relativeTracker.Observe(dirCreate);

    TraceEvent relativeCreate;
    InitializeTraceEvent(&relativeCreate, "ntdll.dll", "NtCreateFile");
    wchar_t relative[] = L"scope.txt";
    AddTraceWideString(&relativeCreate, "path", relative, WideLength(sizeof(relative)));
    AddTracePointer(&relativeCreate, "root_directory", (PVOID)0x10);
    AddTracePointer(&relativeCreate, "file_handle", (PVOID)0x20);
    AddTraceStatus(&relativeCreate, "result", 0);
    relativeTracker.Observe(relativeCreate);

    TraceEvent relativeRead;
    InitializeTraceEvent(&relativeRead, "ntdll.dll", "NtReadFile");
    AddTracePointer(&relativeRead, "file_handle", (PVOID)0x20);
    relativeTracker.Annotate(&relativeRead);
    std::ostringstream relativeText;
    RenderTraceEventText(relativeText, relativeRead);
    if (relativeText.str().find("C:\\Temp\\scope.txt") == std::string::npos) {
        printf("Relative open was not resolved against the root directory\n");
        return 1;
    }

    // NtClose annotates the released path, then evicts so a reused value is
    // not mislabeled.
    HandlePathTracker closeTracker;
    TraceEvent openForClose;
    InitializeTraceEvent(&openForClose, "ntdll.dll", "NtOpenFile");
    AddTraceWideString(&openForClose, "path", path, WideLength(sizeof(path)));
    AddTracePointer(&openForClose, "file_handle", (PVOID)0x55);
    AddTraceStatus(&openForClose, "result", 0);
    closeTracker.Observe(openForClose);

    TraceEvent closeEvent;
    InitializeTraceEvent(&closeEvent, "ntdll.dll", "NtClose");
    AddTracePointer(&closeEvent, "handle", (PVOID)0x55);
    AddTraceStatus(&closeEvent, "result", 0);
    closeTracker.Annotate(&closeEvent);
    closeTracker.Evict(closeEvent);
    std::ostringstream closeJson;
    RenderTraceEventJsonl(closeJson, closeEvent);
    if (closeJson.str().find("scope.txt") == std::string::npos) {
        printf("Close event was not annotated with the released path\n");
        return 1;
    }

    TraceEvent reuseRead;
    InitializeTraceEvent(&reuseRead, "ntdll.dll", "NtReadFile");
    AddTracePointer(&reuseRead, "file_handle", (PVOID)0x55);
    uint16_t reuseBefore = reuseRead.header.fieldCount;
    closeTracker.Annotate(&reuseRead);
    if (reuseRead.header.fieldCount != reuseBefore) {
        printf("Evicted handle should not be annotated\n");
        return 1;
    }

    // Registry key handles correlate the same way as file handles.
    HandlePathTracker keyTracker;
    TraceEvent openKey;
    InitializeTraceEvent(&openKey, "ntdll.dll", "NtOpenKey");
    wchar_t keyPath[] = L"\\REGISTRY\\MACHINE\\SOFTWARE\\ApiScope";
    AddTraceWideString(&openKey, "path", keyPath, WideLength(sizeof(keyPath)));
    AddTracePointer(&openKey, "key_handle", (PVOID)0x77);
    AddTraceStatus(&openKey, "result", 0);
    keyTracker.Observe(openKey);

    TraceEvent setValue;
    InitializeTraceEvent(&setValue, "ntdll.dll", "NtSetValueKey");
    AddTracePointer(&setValue, "key_handle", (PVOID)0x77);
    keyTracker.Annotate(&setValue);
    std::ostringstream setJson;
    RenderTraceEventJsonl(setJson, setValue);
    if (setJson.str().find("ApiScope") == std::string::npos) {
        printf("Registry key handle was not correlated to its path\n");
        return 1;
    }

    return 0;
}
