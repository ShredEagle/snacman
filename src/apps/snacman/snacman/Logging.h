#pragma once

#include "detail/Logger.h"

#include <spdlog/spdlog.h>


#define SELOG_LG(logger, severity) spdlog::get(logger)->severity
#define SELOG(severity) spdlog::get(ad::snac::gMainLogger)->severity
