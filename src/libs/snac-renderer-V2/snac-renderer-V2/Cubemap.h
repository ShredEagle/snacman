#pragma once

#include "Model.h"

#include <renderer/Texture.h>


namespace ad::renderer {


struct Loader;


extern const std::array<math::AffineMatrix<4, GLfloat>, 6> gCubeCaptureViewsNegateY;


/// @param aImageStrip should be in the standard OpenGL order (+X, -X, +Y, -Y, +Z, -Z)
graphics::Texture loadCubemapFromStrip(filesystem::path aImageStrip);

graphics::Texture loadCubemapFromSequence(filesystem::path aImageSequence);

graphics::Texture loadCubemapFromDds(filesystem::path aDds);

graphics::Texture loadEquirectangular(filesystem::path aEquirectangularMap);


// TODO Ad 2024/12/12: This should be removed
struct Skybox
{
    Skybox(const Loader & aLoader, Storage & aStorage);

    //Part mUnitCube;
    Handle<graphics::VertexArrayObject> mCubeVao;
    // NOTE: to disappear in the part effect
    Handle<const Effect> mEffect;
};


} // namespace ad::renderer