#pragma once

#include "trace_protocol.h"
#include <ostream>

static const uint32_t TRACE_JSON_SCHEMA_VERSION = 1;

bool IsValidTraceEvent(const TraceEvent& event, size_t bytesReceived);
void RenderTraceEventText(std::ostream& output, const TraceEvent& event);
void RenderTraceEventJsonl(std::ostream& output, const TraceEvent& event);
