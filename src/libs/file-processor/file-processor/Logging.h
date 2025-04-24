#pragma once

#include "Logging-init.h"

#include <spdlog/spdlog.h>


namespace ad::processor
{


constexpr const char * gMainLogger = "asset-processor";


} // namespace ad::processor


#define SELOG_LG(logger, severity) spdlog::get(logger)->severity
#define SELOG(severity) spdlog::get(ad::processor::gMainLogger)->severity