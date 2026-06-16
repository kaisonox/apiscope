#include "trace_stats.h"
#include <stdio.h>
#include <string>

int main() {
    TraceStats stats;
    for (int index = 0; index < 10; ++index) {
        stats.Record("ntdll.dll!NtClose");
    }
    for (int index = 0; index < 3; ++index) {
        stats.Record("ntdll.dll!NtReadFile");
    }
    stats.dropped = 2;

    std::string line = FormatStatusLine(stats, 4.0, 0);
    if (line.find("13 events") == std::string::npos ||
        line.find("2 drops") == std::string::npos ||
        line.find("NtClose 10") == std::string::npos ||
        line.find("NtReadFile 3") == std::string::npos) {
        printf("Status line was incomplete: %s\n", line.c_str());
        return 1;
    }

    std::string truncated = FormatStatusLine(stats, 4.0, 10);
    if (truncated.size() != 10) {
        printf("Status line was not truncated to width: %zu\n", truncated.size());
        return 1;
    }

    std::string summary = FormatSummary(stats, 4.0);
    if (summary.find("events   : 13") == std::string::npos ||
        summary.find("dropped  : 2") == std::string::npos ||
        summary.find("ntdll.dll!NtClose : 10") == std::string::npos) {
        printf("Summary was incomplete:\n%s\n", summary.c_str());
        return 1;
    }

    return 0;
}
