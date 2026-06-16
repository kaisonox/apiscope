#pragma once

#include <cstdint>
#include <map>
#include <string>

// Per-session counters shared by the live status footer and the end-of-session
// summary. The formatting helpers are pure so they can be unit tested without a
// terminal.
struct TraceStats {
    uint64_t events = 0;
    uint32_t dropped = 0;
    std::map<std::string, uint64_t> perHook;  // "module!api" -> count

    void Record(const std::string& hook);
};

// One-line live status, truncated to maxWidth columns (0 means no limit).
std::string FormatStatusLine(const TraceStats& stats, double elapsedSeconds, size_t maxWidth);

// Multi-line end-of-session summary.
std::string FormatSummary(const TraceStats& stats, double elapsedSeconds);
