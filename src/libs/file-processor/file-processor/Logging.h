#pragma once

#include <spdlog/spdlog.h>


namespace ad::processor
{


constexpr const char * gMainLogger = "asset-processor";


} // namespace ad::processor


#define SELOG_LG(logger, severity) spdlog::get(logger)->severity
#define SELOG(severity) spdlog::get(ad::processor::gMainLogger)->severity


// NOTE Ad 2024/02/09: It seems that the linker is happilly removing LoggerInitialization::gInitialized symbol.
// This hack seems to make it keep the symbol... (although the function is empty)
// The symbol is declared in Logging-init.h, defined in Logging-init.cpp
#if defined(_MSC_VER)
#pragma comment(linker, "/EXPORT:ad_processor_loggerinitialization")
#endif