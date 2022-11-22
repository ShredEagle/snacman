#pragma once

#include <build_info.h>

namespace ad {
namespace snac {


constexpr const char * gMainLogger = gApplicationName;


namespace detail {


/// \brief Safe initialization of the logger, guaranteed to happen only once.
/// Must be called before any logging operation take place.
void initializeLogging();


} // namespace detail
} // namespace snac
} // namespace ad
