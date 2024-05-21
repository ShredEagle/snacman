#pragma once

#include "Logging-init.h"

#include <spdlog/spdlog.h>
#include <fmt/ostream.h>


namespace ad::profiler {


inline constexpr const char * gMainLogger = "profiler";


} // namespace ad::profiler


#define SELOG_LG(logger, severity) spdlog::get(logger)->severity
#define SELOG(severity) spdlog::get(::ad::profiler::gMainLogger)->severity