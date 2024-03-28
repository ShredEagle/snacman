#pragma once


namespace ad::renderer {


inline constexpr const char * gMainLogger = "renderer";
// Intended for diagnotiscs of the renderer pipeline
// This exists as a separate logger not for semantic (it is purely part of the renderer)
// but for convenience, as diagnostics can be too invasive at the point
// (there is a TODO to address this nuisance, but in the meantime)
inline constexpr const char * gPipelineDiag = "pipe-diag";


} // namespace ad::renderer