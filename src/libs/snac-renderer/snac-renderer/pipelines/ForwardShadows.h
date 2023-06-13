#pragma once


#include "../LoadInterface.h"
#include "../Render.h"

#include <handy/AtomicVariations.h>
#include <handy/StringId.h>

#include <math/Vector.h>

#include <renderer/FrameBuffer.h>
#include <renderer/ScopeGuards.h>

#include <resource/ResourceFinder.h>

#include <imguiui/ImguiUi.h>

#include <atomic>


namespace ad {
namespace snac {


class ForwardShadows
{
    struct Controls
    {
        void drawGui();

        inline static const std::array<GLenum, 2> gAvailableFilters{GL_NEAREST, GL_LINEAR};
        MovableAtomic<GLenum> mDetphMapFilter = GL_LINEAR;

        MovableAtomic<GLfloat> mShadowBias = 0.000015f;

        inline static const std::array<GLenum, 2> gCullFaceModes{GL_FRONT, GL_BACK};
        MovableAtomic<GLenum> mCullFaceMode{GL_BACK};

        MovableAtomic<bool> mShowDepthMap{false};
        math::Vec<2, float> mDepthMapRange{0.3f, 0.7f};
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

    Controls & getControls()
    { return mControls; }

private:
    static constexpr math::Size<2, int> gShadowMapSize{2048, 2048};

    const graphics::AppInterface & mAppInterface;

    graphics::Texture depthMap{GL_TEXTURE_2D};
    graphics::FrameBuffer depthFBO;
    Mesh screenQuad;

    Controls mControls;
};


} // namespace snac
} // namespace ad
