#include "ImguiSceneEditor.h"

#include "component/Geometry.h"
#include "component/SceneNode.h"

#include <snacman/EntityUtilities.h>

#include <math/EulerAngles.h>

#include <imgui.h>

#include <string>


namespace ad {
namespace snacgame {

using component::Geometry;
using component::SceneNode;

namespace {

    const component::SceneNode & getNode(ent::Handle<ent::Entity> aEntityHandle)
    {
        return snac::getComponent<component::SceneNode>(aEntityHandle);
    }

    component::Geometry & getGeometry(ent::Handle<ent::Entity> aEntityHandle)
    {
        assert(aEntityHandle.isValid());
        auto entity = *aEntityHandle.get();
        assert(entity.has<Geometry>());
        return entity.get<Geometry>();
    }

    void presentPoseEditor(ent::Handle<ent::Entity> aEntityHandle)
    {
        using math::EulerAngles;

        ImGui::Begin("Pose");

        Geometry & geometry = getGeometry(aEntityHandle);
        ImGui::InputFloat3("Position", geometry.mPosition.data());
        ImGui::InputFloat("Scaling", &geometry.mScaling);
        ImGui::InputFloat3("Instance scaling", geometry.mInstanceScaling.data());

        EulerAngles<float, math::Radian> angles = toEulerAngles(geometry.mOrientation);
        EulerAngles<float, math::Degree> degrees = angles.as<math::Degree>();
        if(ImGui::InputFloat3("Orientation", (float*)&degrees))
        {
            angles = degrees.as<math::Radian>();
            geometry.mOrientation = toQuaternion(angles);
        }

        ImGui::End();
    }
        
} // unnamed namespace


void SceneEditor::showEditor(ent::Handle<ent::Entity> aSceneRoot)
{
    ImGui::Begin("Scene tree");
    
    presentNode(aSceneRoot);

    // Note: when changing scene, selected node might become invalid.
    // Address it naively by testing.
    if(mSelected.isValid())
    {
        presentPoseEditor(mSelected);
    }

    ImGui::End();
}

   
void SceneEditor::presentNode(ent::Handle<ent::Entity> aEntityHandle)
{
    const component::SceneNode & sceneNode = getNode(aEntityHandle);

    int flags = sceneNode.hasChild() ? 0 : ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
    const std::string  name = 
        sceneNode.hasName() ? sceneNode.mName : "Node_" + std::to_string(aEntityHandle.id());

    bool opened = ImGui::TreeNodeEx((void*)aEntityHandle.id(), flags, "%s", name.c_str());

    ImGui::PushID((void*)aEntityHandle.id());

    if(ImGui::IsItemClicked(0))
    {
        mSelected = aEntityHandle;
    }

    if(opened)
    {
        for(ent::Handle<ent::Entity> child = sceneNode.mFirstChild;
            child.isValid();
            child = getNode(child).mNextChild)
        {
            presentNode(child);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}


} // namespace snacgame
} // namespace ad
