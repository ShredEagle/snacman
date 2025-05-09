#pragma once

#include "Logging-channels.h"

#if !defined(_MSC_VER) || defined(__llvm__)
#include "Logging-init.h"
#endif

#include <fmt/ostream.h>
#include <spdlog/spdlog.h>

namespace ad::renderer {

} // namespace ad::renderer

#define SELOG_LG(logger, severity) spdlog::get(logger)->severity
#define SELOG(severity) spdlog::get(::ad::renderer::gMainLogger)->severity

#if defined(_MSC_VER) && !defined(__llvm__)
// NOTE Ad 2024/02/09: It seems that the linker is happilly removing
// LoggerInitialization::gInitialized symbol. This hack seems to make it keep
// the symbol... (although the function is empty) The symbol is declared in
// Logging-init.h, defined in Logging-init.cpp
#pragma comment(linker, "/EXPORT:ad_renderer_loggerinitialization")
#endif
