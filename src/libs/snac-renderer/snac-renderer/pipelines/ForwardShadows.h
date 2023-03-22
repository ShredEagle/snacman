#pragma once


#include "../LoadInterface.h"
#include "../Render.h"

#include <handy/StringId.h>

#include <math/Vector.h>

#include <renderer/FrameBuffer.h>
#include <renderer/ScopeGuards.h>

#include <resource/ResourceFinder.h>

#include <imguiui/ImguiUi.h>


namespace ad {
namespace snac {


class ForwardShadows
{
public:
    ForwardShadows(const graphics::AppInterface & aAppInterface,
                   Load<Technique> & aTechniqueLoader);

    ForwardShadows(const graphics::AppInterface & aAppInterface,
                   Load<Technique> && aTechniqueLoader) :
        ForwardShadows{aAppInterface, aTechniqueLoader}
    {}

    void execute(
        const std::vector<Pass::Visual> & aEntities,
        const Camera & aLightViewpoint,
        Renderer & aRenderer,
        ProgramSetup & aProgramSetup);

private:
    void drawGui();

    static constexpr math::Size<2, int> gShadowMapSize{1024, 1024};

    const graphics::AppInterface & mAppInterface;

    graphics::Texture depthMap{GL_TEXTURE_2D};
    graphics::FrameBuffer depthFBO;
    Mesh screenQuad;

    GLfloat mShadowBias = 0.00005f;
    GLenum mDetphMapFilter = GL_NEAREST;
};


} // namespace snac
} // namespace ad
