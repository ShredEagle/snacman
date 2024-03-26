#pragma once


#include <spdlog/spdlog.h>
#include <fmt/ostream.h>


namespace ad::renderer {


inline constexpr const char * gMainLogger = "renderer";
// Intended for diagnotiscs of the renderer pipeline
// This exists as a separate logger not for semantic (it is purely part of the renderer)
// but for convenience, as diagnostics can be too invasive at the point
// (there is a TODO to address this nuisance, but in the meantime)
inline constexpr const char * gPipelineDiag = "pipe-diag";


} // namespace ad::renderer


#define SELOG_LG(logger, severity) spdlog::get(logger)->severity
#define SELOG(severity) spdlog::get(::ad::renderer::gMainLogger)->severity


// NOTE Ad 2024/02/09: It seems that the linker is happilly removing LoggerInitialization::gInitialized symbol.
// This hack seems to make it keep the symbol... (although the function is empty)
// The symbol is declared in Logging-init.h, defined in Logging-init.cpp
#if defined(_MSC_VER)
#pragma comment(linker, "/EXPORT:ad_renderer_loggerinitialization")
#endif