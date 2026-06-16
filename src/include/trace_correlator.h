#pragma once

#include "trace_protocol.h"
#include <cstdint>
#include <string>
#include <unordered_map>

// Correlates file handles with the paths they were opened against so later
// operations on the same handle can be annotated with a resolved path.
//
// An event that directly observed a path (a create/open carrying a "path"
// field) is treated as a source and recorded. An event that references a
// known handle but observed no path of its own (a read/write) is annotated
// with the recorded path. Relative opens are resolved by chaining through a
// previously recorded "root_directory" handle.
//
// Single-threaded by design: intended for use from the trace writer thread,
// which processes events in causal order, so no locking is required.
class HandlePathTracker {
public:
    void Observe(const TraceEvent& event);
    void Annotate(TraceEvent* event);
    void Evict(const TraceEvent& event);

private:
    std::unordered_map<uint64_t, std::wstring> handlePaths_;
};
