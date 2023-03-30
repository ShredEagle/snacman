#pragma once


#include "../Mesh.h"

#include <platform/Filesystem.h>


namespace ad {
namespace snac {


Model loadGltf(filesystem::path aModel, std::string_view aName);

inline Model loadGltf(filesystem::path aModel)
{
    // TODO would be more correct to actually use the full identifier to the model
    // as requested by the "top level" code.
    // Sadly, at this point this information is lost, we only have the actual full path.
    return loadGltf(aModel, aModel.filename().string());
}



} // namespace snac
} // namespace ad
