#pragma once

#include "Gui.h"
#include "Visuals.h"

#include <handy/StringId.h>

#include <math/Vector.h>

#include <renderer/FrameBuffer.h>
#include <renderer/ScopeGuards.h>

#include <resource/ResourceFinder.h>

#include <snac-renderer/Cube.h>
#include <snac-renderer/Render.h>


namespace ad {
namespace snac {


class DrawerShadows
{
public:
    inline DrawerShadows(graphics::ApplicationGlfw & aGlfwApp,
                         const resource::ResourceFinder & aFinder) :
        mAppInterface{*aGlfwApp.getAppInterface()},
        mFinder{aFinder}
    {}

    void draw(
        const VisualEntities & aEntities,
        Renderer & aRenderer,
        const UniformRepository & aUniforms,
        const snac::UniformBlocks & aUniformBlocks);

private:
    void drawGui();

    static constexpr math::Size<2, int> gShadowMapSize{1024, 1024};

    const graphics::AppInterface & mAppInterface;
    const resource::ResourceFinder & mFinder;

    // TODO rename
    GLfloat mShadowBias = 0.00001f;
    GLenum mDetphMapFilter = GL_NEAREST;
};


} // namespace snac
} // namespace ad
