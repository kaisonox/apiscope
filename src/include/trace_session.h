#pragma once

#include "common.h"
#include "hook_manager.h"
#include "trace_correlator.h"
#include "trace_protocol.h"
#include "trace_ring.h"
#include "trace_stats.h"
#include <chrono>
#include <condition_variable>
#include <deque>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>

enum class TraceOutputFormat {
    Text,
    Jsonl,
};

enum class TraceColorMode {
    Auto,
    Always,
    Never,
};

struct TraceOutputOptions {
    TraceOutputFormat format;
    std::wstring outputPath;
    bool quiet;
    TraceColorMode color = TraceColorMode::Auto;
    TraceColorMode status = TraceColorMode::Auto;
};

class TraceSession {
public:
    TraceSession();
    ~TraceSession();

    bool Start(
        HANDLE targetProcess,
        PVOID hookImageBase,
        const TraceOutputOptions& outputOptions);
    bool ConfigureTransport(HANDLE targetProcess, PVOID ntdllBase, PVOID hookImageBase);
    bool CleanupRemote(HANDLE targetProcess, PVOID hookImageBase);
    void Drain();
    void Stop();

private:
    bool OpenOutput(const TraceOutputOptions& outputOptions);
    bool CreateSharedRing(HANDLE targetProcess);
    void ReleaseLocalRing();
    bool TryReadRing(TraceEvent* event);
    bool WriteRemoteExport(HANDLE targetProcess, PVOID hookImageBase, const char* exportName, const void* value, SIZE_T size);
    void ReaderLoop();
    void WriterLoop();
    void RenderEvent(const TraceEvent& event);
    void PrintSummary();
    void EnableVtMode();
    void DrawFooter();
    void EraseFooter();
    void FooterLoop();
    size_t ConsoleWidth();
    static std::string CurrentHookName(const TraceEvent& event);

    HANDLE mappingHandle_;
    TraceRing* localRing_;
    TraceRing* remoteRing_;
    HANDLE wakeEvent_;
    HANDLE remoteWakeEvent_;
    HANDLE stopEvent_;
    std::thread readerThread_;
    std::thread writerThread_;
    std::mutex queueMutex_;
    std::condition_variable queueNotEmpty_;
    std::condition_variable queueNotFull_;
    std::deque<TraceEvent> eventQueue_;
    bool readerFinished_;
    std::ofstream outputFile_;
    TraceOutputFormat outputFormat_;
    bool quiet_;
    bool colorEnabled_;
    bool footerEnabled_;
    bool footerDrawn_;
    uint32_t reportedDroppedCount_;
    HandlePathTracker handleTracker_;
    TraceStats stats_;
    std::chrono::steady_clock::time_point startTime_;
    bool started_;
    bool summaryPrinted_;
    std::mutex consoleMutex_;
    std::thread footerThread_;
    RemoteTrampoline setEventBypass_;
    RemoteTrampoline readMemoryBypass_;
};
