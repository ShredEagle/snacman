#pragma once


#include <spdlog/spdlog.h>
#include <fmt/ostream.h>


namespace ad::profiler {


inline constexpr const char * gMainLogger = "profiler";


} // namespace ad::profiler


#define SELOG_LG(logger, severity) spdlog::get(logger)->severity
#define SELOG(severity) spdlog::get(::ad::profiler::gMainLogger)->severity

// NOTE Ad 2024/02/09: It seems that the linker is happilly removing LoggerInitialization::gInitialized symbol.
// This hack seems to make it keep the symbol... (although the function is empty)
// The symbol is declared in Logging-init.h, defined in Logging-init.cpp
#if defined(_MSC_VER)
#pragma comment(linker, "/EXPORT:ad_profiler_loggerinitialization")
#endif