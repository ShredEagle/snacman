#include "DebugDrawing.h"

#include <mutex> // for once_flag

namespace ad::renderer {


namespace {

    std::once_flag debugDrawersInitializationOnce;

    void initializeDebugDrawers_impl()
    {
        snac::DebugDrawer::AddDrawer(drawer::gCamera);
        snac::DebugDrawer::AddDrawer(drawer::gLight);
        snac::DebugDrawer::AddDrawer(drawer::gRig);
        snac::DebugDrawer::AddDrawer(drawer::gScene);
        snac::DebugDrawer::AddDrawer(drawer::gShadow);
    }

} // unnamed namespace


void initializeDebugDrawers()
{
    std::call_once(debugDrawersInitializationOnce, &initializeDebugDrawers_impl);
}


} // namespace ad::renderer