#include "trace_correlator.h"
#include <cstring>

namespace {

struct FieldRef {
    uint8_t type;
    const BYTE* value;
    uint16_t valueSize;
};

// Locates a field by name in an already-validated event. Pass out == nullptr
// to test only for the field's presence.
bool FindField(const TraceEvent& event, const char* name, FieldRef* out) {
    size_t nameLength = strlen(name);
    const BYTE* cursor = event.payload +
        event.header.moduleNameLength + event.header.apiNameLength;
    const BYTE* end = reinterpret_cast<const BYTE*>(&event) + event.header.size;
    for (uint16_t index = 0; index < event.header.fieldCount; ++index) {
        if (cursor + sizeof(TraceFieldHeader) > end) {
            return false;
        }
        TraceFieldHeader field;
        memcpy(&field, cursor, sizeof(field));
        cursor += sizeof(field);
        if (cursor + (size_t)field.nameLength + field.valueSize > end) {
            return false;
        }
        if (field.nameLength == nameLength && memcmp(cursor, name, nameLength) == 0) {
            if (out) {
                out->type = field.type;
                out->value = cursor + field.nameLength;
                out->valueSize = field.valueSize;
            }
            return true;
        }
        cursor += field.nameLength + field.valueSize;
    }
    return false;
}

bool ReadHandle(const TraceEvent& event, const char* name, uint64_t* handle) {
    FieldRef ref;
    if (!FindField(event, name, &ref) ||
        ref.type != TraceFieldPointer ||
        ref.valueSize != sizeof(uint64_t)) {
        return false;
    }
    memcpy(handle, ref.value, sizeof(*handle));
    return true;
}

bool ReadWideString(const TraceEvent& event, const char* name, std::wstring* value) {
    FieldRef ref;
    if (!FindField(event, name, &ref) || ref.type != TraceFieldWideString) {
        return false;
    }
    value->assign(ref.valueSize / sizeof(wchar_t), L'\0');
    memcpy(&(*value)[0], ref.value, value->size() * sizeof(wchar_t));
    return true;
}

bool EventSucceeded(const TraceEvent& event) {
    FieldRef ref;
    if (!FindField(event, "result", &ref) ||
        ref.type != TraceFieldStatus ||
        ref.valueSize != sizeof(int32_t)) {
        return true;  // No status field: treat the handle as usable.
    }
    int32_t status;
    memcpy(&status, ref.value, sizeof(status));
    return status >= 0;  // NT_SUCCESS
}

bool AppendWideString(TraceEvent* event, const char* name, const std::wstring& value) {
    size_t nameLength = strlen(name);
    size_t byteLength = value.size() * sizeof(wchar_t);
    if (byteLength > TRACE_MAX_STRING_BYTES) {
        byteLength = TRACE_MAX_STRING_BYTES;
    }
    size_t required = sizeof(TraceFieldHeader) + nameLength + byteLength;
    if ((size_t)event->header.size + required > sizeof(TraceEvent)) {
        return false;  // No room; leave the event unannotated.
    }

    BYTE* cursor = reinterpret_cast<BYTE*>(event) + event->header.size;
    TraceFieldHeader field = {};
    field.type = TraceFieldWideString;
    field.nameLength = (uint8_t)nameLength;
    field.valueSize = (uint16_t)byteLength;
    memcpy(cursor, &field, sizeof(field));
    cursor += sizeof(field);
    memcpy(cursor, name, nameLength);
    cursor += nameLength;
    memcpy(cursor, value.data(), byteLength);
    event->header.size = (uint16_t)(event->header.size + required);
    ++event->header.fieldCount;
    return true;
}

// The object handle a hook produces or consumes. File hooks name it
// "file_handle", registry hooks "key_handle", and NtClose "handle".
bool FindObjectHandle(const TraceEvent& event, uint64_t* handle) {
    return ReadHandle(event, "file_handle", handle) ||
           ReadHandle(event, "key_handle", handle) ||
           ReadHandle(event, "handle", handle);
}

const size_t MAX_TRACKED_HANDLES = 1 << 16;

}  // namespace

void HandlePathTracker::Observe(const TraceEvent& event) {
    std::wstring path;
    if (!ReadWideString(event, "path", &path) || path.empty()) {
        return;  // Not a source event.
    }
    uint64_t handle = 0;
    if (!FindObjectHandle(event, &handle) || handle == 0) {
        return;
    }
    if (!EventSucceeded(event)) {
        return;  // Failed opens never produced a usable handle.
    }

    uint64_t rootDirectory = 0;
    if (ReadHandle(event, "root_directory", &rootDirectory) && rootDirectory != 0) {
        auto root = handlePaths_.find(rootDirectory);
        if (root != handlePaths_.end()) {
            std::wstring base = root->second;  // Resolve the relative open...
            bool baseSep = !base.empty() && base.back() == L'\\';
            bool relSep = !path.empty() && path.front() == L'\\';
            if (baseSep && relSep) {
                path.erase(0, 1);
            } else if (!baseSep && !relSep) {
                base.push_back(L'\\');
            }
            path = base + path;  // ...without doubling or dropping the separator.
        }
    }

    if (handlePaths_.size() >= MAX_TRACKED_HANDLES) {
        handlePaths_.clear();  // Bound memory on long-running, handle-heavy targets.
    }
    handlePaths_[handle] = path;
}

void HandlePathTracker::Annotate(TraceEvent* event) {
    if (!event || FindField(*event, "path", nullptr)) {
        return;  // Missing event or a directly observed path; nothing to add.
    }
    uint64_t handle = 0;
    if (!FindObjectHandle(*event, &handle) || handle == 0) {
        return;
    }
    auto found = handlePaths_.find(handle);
    if (found != handlePaths_.end()) {
        AppendWideString(event, "path", found->second);
    }
}

void HandlePathTracker::Evict(const TraceEvent& event) {
    uint64_t handle = 0;
    if (ReadHandle(event, "handle", &handle) && handle != 0 && EventSucceeded(event)) {
        handlePaths_.erase(handle);  // NtClose released the handle; forget its path.
    }
}
