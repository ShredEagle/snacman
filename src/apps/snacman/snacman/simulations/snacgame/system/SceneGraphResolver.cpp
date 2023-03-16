#include "SceneGraphResolver.h"

#include "../GameParameters.h"
#include "../component/SceneNode.h"

namespace ad {
namespace snacgame {
namespace system {

namespace {
inline component::GlobalPosition resolveGlobalPos(const component::Geometry & aLocalGeo, const component::GlobalPosition & aParentPos)
{
    component::GlobalPosition result{};
    result.mPosition.x() = -aLocalGeo.mPosition.y() - aParentPos.mPosition.y();
    result.mPosition.z() = -aLocalGeo.mPosition.x() - aParentPos.mPosition.x();
    result.mPosition.y() = static_cast<float>((int)aLocalGeo.mLayer) * gCellSize * 0.1f;
    result.mOrientation = aLocalGeo.mOrientation * aParentPos.mOrientation;
    result.mScaling = aLocalGeo.mScaling.cwMul(aParentPos.mScaling);
    return result;
}

inline void depthFirstResolve(const component::SceneNode & aSceneNode, const component::GlobalPosition & aParentPos, ent::Phase & aPhase)
{
    if (aSceneNode.aFirstChild)
    {
        ent::Handle<ent::Entity> current = *aSceneNode.aFirstChild;
        component::SceneNode * node = &current.get(aPhase)->get<component::SceneNode>();

        while(node->aNextChild != aSceneNode.aFirstChild)
        {
            const component::Geometry geo = current.get(aPhase)->get<component::Geometry>();
            component::GlobalPosition & currGlobalPos = current.get(aPhase)->get<component::GlobalPosition>();
            currGlobalPos = resolveGlobalPos(geo, aParentPos);
            depthFirstResolve(aSceneNode, currGlobalPos, aPhase);
            current = node->aNextChild;
            node = &current.get(aPhase)->get<component::SceneNode>();
        }
    }
}
}

void SceneGraphResolver::update()
{
    mRoots.each([](component::SceneRoot & aRoot, component::Geometry & aGeo, component::GlobalPosition & aGlobalPos) {
        ent::Phase resolve;
        if (aRoot.aFirstChild)
        {
            ent::Handle<ent::Entity> current = *aRoot.aFirstChild;
            component::SceneNode node = current.get(resolve)->get<component::SceneNode>();
            depthFirstResolve(node, aGlobalPos, resolve);
        }
    });
}

}
} // namespace snacgame
} // namespace ad
