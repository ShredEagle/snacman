#pragma once

#include <spdlog/spdlog.h>
#include <fmt/ostream.h>


namespace se 
{


constexpr const char * gMainLogger = "protoapp";


/// \brief Safe initialization of the logger, guaranteed to happen only once.
/// Must be called before any logging operation take place.
void initializeLogging();

#define SELOG_LG(logger, severity) spdlog::get(logger)->severity
#define SELOG(severity) spdlog::get(se::gMainLogger)->severity


} // namespace se