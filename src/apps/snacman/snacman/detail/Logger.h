#pragma once


namespace ad {
namespace snac {


constexpr const char * gMainLogger = "snacman";


namespace detail {


/// \brief Safe initialization of the logger, guaranteed to happen only once.
/// Must be called before any logging operation take place.
void initializeLogging();


} // namespace detail
} // namespace snac
} // namespace ad
