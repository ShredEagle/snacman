#include "Bawls.h"

#include "ComponentGeometry.h"
#include "ComponentVelocity.h"


namespace ad {
namespace bawls {


namespace {


math::Size<2, GLfloat> getWindowSizeInWorld(math::Size<2, int> aWindowResolution,
                                            GLfloat aWindowHeight_world)
{
    GLfloat worldUnitPerPixel = aWindowHeight_world / aWindowResolution.height();
    return math::Size<2, GLfloat>{aWindowResolution.as<math::Size, float>() * worldUnitPerPixel};
}


} // anonymous namespace


Bawls::Bawls(const ad::graphics::AppInterface & aAppInterface) :
    mWindowSize_world{
        getWindowSizeInWorld(aAppInterface.getWindowSize(), gWindowHeight_world)},
    mMoveSystem{mWorld, math::Rectangle<GLfloat>{{0.f, 0.f}, mWindowSize_world}.centered()},
    mCollideSystem{mWorld},
    mGraphicStateProducer{mWorld}
{
    ent::Phase prepopulate;
    mWorld.addEntity().get(prepopulate)
        ->add<component::Geometry>({.position = {0.f, 0.f}, .radius = 2.f})
        .add<component::Velocity>({1.f, 3.f})
        ;
    mWorld.addEntity().get(prepopulate)
        ->add<component::Geometry>({.position = {3.f, -1.f}, .radius = 0.5f})
        .add<component::Velocity>({1.2f, -3.5f})
        ;
    mWorld.addEntity().get(prepopulate)
        ->add<component::Geometry>({.position = {-3.f, 2.f}})
        .add<component::Velocity>({5.5f, 1.0f})
        ;
    mWorld.addEntity().get(prepopulate)
        ->add<component::Geometry>({.position = {3.f, 2.f}})
        .add<component::Velocity>({5.5f, 1.0f})
        ;
}


void Bawls::update(float aDelta)
{
    //mHistory.push(Backup{mWorld.saveState()});
    mMoveSystem.update(aDelta);
    mCollideSystem.update();
}


//void Bawls::restorePrevious()
//{
//    mWorld.restoreState(mHistory.rewindBounded().state);
//}
//
//
//void Bawls::restoreNext()
//{
//    mWorld.restoreState(mHistory.advanceBounded().state);
//}


} // namespace bawls
} // namespace ad
