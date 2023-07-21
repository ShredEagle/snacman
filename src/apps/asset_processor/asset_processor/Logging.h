#pragma once

#include <spdlog/spdlog.h>


namespace ad 
{


constexpr const char * gMainLogger = "asset-processor";


/// \brief Safe initialization of the logger, guaranteed to happen only once.
/// Must be called before any logging operation take place.
void initializeLogging();

#define SELOG_LG(logger, severity) spdlog::get(logger)->severity
#define SELOG(severity) spdlog::get(ad::gMainLogger)->severity

} // namespace ad