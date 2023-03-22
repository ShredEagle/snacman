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
    struct Controls
    {
        void drawGui();

        inline static const std::array<GLenum, 2> gAvailableFilters{GL_NEAREST, GL_LINEAR};
        GLfloat mShadowBias = 0.00005f;
        GLenum mDetphMapFilter = GL_LINEAR;
    };

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

    void drawGui()
    { mControls.drawGui(); }

private:
    static constexpr math::Size<2, int> gShadowMapSize{1024, 1024};

    const graphics::AppInterface & mAppInterface;

    graphics::Texture depthMap{GL_TEXTURE_2D};
    graphics::FrameBuffer depthFBO;
    Mesh screenQuad;

    Controls mControls;
};


} // namespace snac
} // namespace ad
