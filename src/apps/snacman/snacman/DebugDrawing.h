#include <snac-renderer/DebugDrawer.h>


namespace ad {
namespace snac {


constexpr const char * gBoundingBoxDrawer = "bounding boxes";
constexpr const char * gWorldDrawer = "world";


void initializeDebugDrawers();


#define SEDBGDRAW(drawer) ::ad::snac::DebugDrawer::Get(drawer)


} // namespace snac
} // namespace ad