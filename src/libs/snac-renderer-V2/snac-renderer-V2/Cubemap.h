#pragma once

#include "Model.h"

#include <renderer/Texture.h>


namespace ad::renderer {


struct Loader;


/// @param aImageStrip should be in the standard OpenGL order (+X, -X, +Y, -Y, +Z, -Z)
graphics::Texture loadCubemapFromStrip(filesystem::path aImageStrip);


// TODO Ad 2024/06/26: This should be made generic, and moved in a general header
//Part makeUnitCube(Storage & aStorage);

struct Skybox
{
    Skybox(const Loader & aLoader, Storage & aStorage);

    //Part mUnitCube;
    Handle<graphics::VertexArrayObject> mCubeVao;
    // NOTE: to disappear in the part effect
    IntrospectProgram mProgram;
};



} // namespace ad::renderer