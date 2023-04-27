#include "DebugDrawing.h"

#include <mutex> // for once_flag


namespace ad {
namespace snac {


namespace {

    std::once_flag debugDrawersInitializationOnce;

    void initializeDebugDrawers_impl()
    {
        snac::DebugDrawer::AddDrawer(gBoundingBoxDrawer);
        snac::DebugDrawer::AddDrawer(gWorldDrawer);
        snac::DebugDrawer::AddDrawer(gPortalDrawer);
        snac::DebugDrawer::AddDrawer(gHitboxDrawer);
    }

} // namespace anonymous


void initializeDebugDrawers()
{
    std::call_once(debugDrawersInitializationOnce, &initializeDebugDrawers_impl);
}


} // namespace snac
} // namespace ad
