#include "trace_stats.h"
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <vector>

void TraceStats::Record(const std::string& hook) {
    ++events;
    ++perHook[hook];
}

static std::string GroupThousands(uint64_t value) {
    std::string digits = std::to_string(value);
    std::string out;
    int count = 0;
    for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
        if (count != 0 && count % 3 == 0) {
            out.push_back(',');
        }
        out.push_back(*it);
        ++count;
    }
    std::reverse(out.begin(), out.end());
    return out;
}

static std::string FormatDuration(double seconds) {
    if (seconds < 0.0) {
        seconds = 0.0;
    }
    uint64_t total = (uint64_t)(seconds + 0.5);
    uint64_t hours = total / 3600;
    uint64_t minutes = (total % 3600) / 60;
    uint64_t secs = total % 60;
    char buffer[32];
    if (hours != 0) {
        snprintf(buffer, sizeof(buffer), "%llu:%02llu:%02llu",
                 (unsigned long long)hours, (unsigned long long)minutes, (unsigned long long)secs);
    } else {
        snprintf(buffer, sizeof(buffer), "%llu:%02llu",
                 (unsigned long long)minutes, (unsigned long long)secs);
    }
    return buffer;
}

static std::vector<std::pair<std::string, uint64_t>> SortedHooks(const TraceStats& stats) {
    std::vector<std::pair<std::string, uint64_t>> hooks(stats.perHook.begin(), stats.perHook.end());
    std::sort(hooks.begin(), hooks.end(), [](const auto& a, const auto& b) {
        if (a.second != b.second) {
            return a.second > b.second;
        }
        return a.first < b.first;
    });
    return hooks;
}

std::string FormatStatusLine(const TraceStats& stats, double elapsedSeconds, size_t maxWidth) {
    double rate = elapsedSeconds > 0.0 ? (double)stats.events / elapsedSeconds : 0.0;
    std::ostringstream line;
    line << " " << GroupThousands(stats.events) << " events"
         << "  " << stats.dropped << " drops"
         << "  " << (uint64_t)(rate + 0.5) << "/s"
         << "  " << FormatDuration(elapsedSeconds);

    std::vector<std::pair<std::string, uint64_t>> hooks = SortedHooks(stats);
    for (size_t index = 0; index < hooks.size() && index < 4; ++index) {
        const std::string& name = hooks[index].first;
        size_t bang = name.find('!');
        std::string shortName = bang == std::string::npos ? name : name.substr(bang + 1);
        line << "  " << shortName << " " << hooks[index].second;
    }

    std::string result = line.str();
    if (maxWidth != 0 && result.size() > maxWidth) {
        result = result.substr(0, maxWidth);
    }
    return result;
}

std::string FormatSummary(const TraceStats& stats, double elapsedSeconds) {
    std::ostringstream summary;
    summary << "[=] Session summary\n";
    summary << "    events   : " << stats.events << "\n";
    summary << "    dropped  : " << stats.dropped << "\n";
    summary << "    duration : " << FormatDuration(elapsedSeconds) << "\n";
    std::vector<std::pair<std::string, uint64_t>> hooks = SortedHooks(stats);
    if (!hooks.empty()) {
        summary << "    per hook :\n";
        for (const auto& hook : hooks) {
            summary << "      " << hook.first << " : " << hook.second << "\n";
        }
    }
    return summary.str();
}
