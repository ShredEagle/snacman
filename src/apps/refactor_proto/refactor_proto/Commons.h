#pragma once


#include <string>


namespace ad::renderer {


using StringKey = std::string;


// Initially designed as an enum, but the closed-values might be too inflexible for clients.
using Semantic = StringKey;
using BlockSemantic = StringKey;


inline std::string to_string(Semantic aSemantic)
{ return aSemantic; }


} // namespace ad::renderer