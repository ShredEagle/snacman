#pragma once


#include "../LoadInterface.h"
#include "../Render.h"

#include <handy/StringId.h>

#include <math/Vector.h>

#include <renderer/FrameBuffer.h>
#include <renderer/ScopeGuards.h>

#include <resource/ResourceFinder.h>

#include <imguiui/ImguiUi.h>

#include <atomic>


namespace ad {
namespace snac {


// Hackish class, to allow to use atomic with movable types.
// Note that this is dangerous, if a thread is using the moved from atomic
// to synchronise with a thread already using the moved to atomic (i.e. sync is lost)
// Yet this is useful if the move is only part of the initialization process.
template <class T>
struct MovableAtomic : public std::atomic<T>
{
    using std::atomic<T>::atomic;
    using std::atomic<T>::operator=;

    MovableAtomic(MovableAtomic && aOther) :
        std::atomic<T>{aOther.load()}
    {};
};


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
