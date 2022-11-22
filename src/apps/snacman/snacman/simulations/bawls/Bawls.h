#pragma once

#include "GraphicState.h"
#include "Renderer.h"
#include "SystemCollide.h"
#include "SystemMove.h"

#include "../../Input.h"

#include <entity/EntityManager.h>

#include <graphics/AppInterface.h>

//#include <utils/CircularBuffer.h>


namespace ad {
namespace bawls {

class Bawls
{
    //struct Backup
    //{
    //    ent::State state;
    //};

public:
    using Renderer_t = Renderer;

    Bawls(const ad::graphics::AppInterface & aAppInterface);

    void update(float aDelta, const snac::Input & aInput);

    //void restorePrevious();
    //void restoreNext();

    std::unique_ptr<GraphicState> makeGraphicState() /*const*/ //TODO should allow constness
    { return mGraphicStateProducer.makeGraphicState(); }

    Renderer_t makeRenderer() const;

private:
    static constexpr GLfloat gWindowHeight_world = 10;

    ent::EntityManager mWorld;
    math::Size<2, GLfloat> mWindowSize_world;

    // TODO replace with handles
    system::Move mMoveSystem;
    system::Collide mCollideSystem;

    GraphicStateProducer mGraphicStateProducer;

    //CircularBuffer<Backup, 512> mHistory;
};


} // namespace bawls
} // namespace ad