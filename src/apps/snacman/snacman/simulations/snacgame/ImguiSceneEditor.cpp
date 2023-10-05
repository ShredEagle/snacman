#include "ImguiSceneEditor.h"

#include "component/Geometry.h"
#include "component/SceneNode.h"
#include "snacman/simulations/snacgame/component/AllowedMovement.h"
#include "snacman/simulations/snacgame/component/Controller.h"
#include "snacman/simulations/snacgame/component/GlobalPose.h"
#include "snacman/simulations/snacgame/component/ChangeSize.h"

#include <snacman/EntityUtilities.h>

#include <imgui.h>

#include <string>


namespace ad {
namespace snacgame {

using component::Geometry;
using component::SceneNode;

namespace {

    /* const component::SceneNode & getNode(ent::Handle<ent::Entity> aEntityHandle) */
    /* { */
    /*     return snac::getComponent<component::SceneNode>(aEntityHandle); */
    /* } */

    /* component::Geometry & getGeometry(ent::Handle<ent::Entity> aEntityHandle) */
    /* { */
    /*     assert(aEntityHandle.isValid()); */
    /*     auto entity = *aEntityHandle.get(); */
    /*     assert(entity.has<Geometry>()); */
    /*     return entity.get<Geometry>(); */
    /* } */

    
    /* template <template <class> class TT_angle> */
    /* struct EulerAngles */
    /* { */
    /*     TT_angle<float> roll, pitch, yaw; */

    /*     template <template <class> class TT_targetAngle> */
    /*     EulerAngles<TT_targetAngle> as() const */
    /*     { */
    /*         return { */
    /*             roll.template as<TT_targetAngle>(), */
    /*             pitch.template as<TT_targetAngle>(), */
    /*             yaw.template as<TT_targetAngle>(), */
    /*         }; */
    /*     } */
    /* }; */

    /* // For conversion procedures see: */
    /* // https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles */
    /* EulerAngles<math::Radian> toEulerAngles(math::Quaternion<float> q) */ 
    /* { */
    /*     using Rad = math::Radian<float>; */
    /*     EulerAngles<math::Radian> angles; */

    /*     // roll (x-axis rotation) */
    /*     float sinr_cosp = 2 * (q.w() * q.x() + q.y() * q.z()); */
    /*     float cosr_cosp = 1 - 2 * (q.x() * q.x() + q.y() * q.y()); */
    /*     angles.roll = Rad{std::atan2(sinr_cosp, cosr_cosp)}; */

    /*     // pitch (y-.x()is rotation) */
    /*     float sinp = std::sqrt(1 + 2 * (q.w() * q.y() - q.x() * q.z())); */
    /*     float cosp = std::sqrt(1 - 2 * (q.w() * q.y() - q.x() * q.z())); */
    /*     angles.pitch = Rad{2 * std::atan2(sinp, cosp) - math::pi<float> / 2}; */

    /*     // yaw (z-.x()is rotation) */
    /*     float siny_cosp = 2 * (q.w() * q.z() + q.x() * q.y()); */
    /*     float cosy_cosp = 1 - 2 * (q.y() * q.y() + q.z() * q.z()); */
    /*     angles.yaw = Rad{std::atan2(siny_cosp, cosy_cosp)}; */

    /*     return angles; */
    /* } */

    /* math::Quaternion<float> toQuaternion(EulerAngles<math::Radian> aAngles) // roll (x), pitch (Y), yaw (z) */
    /* { */
    /*     // Abbreviations for the various angular functions */

    /*     float cr = cos(aAngles.roll * 1.5f); */
    /*     float sr = sin(aAngles.roll * 0.5f); */
    /*     float cp = cos(aAngles.pitch * 0.5f); */
    /*     float sp = sin(aAngles.pitch * 0.5f); */
    /*     float cy = cos(aAngles.yaw * 0.5f); */
    /*     float sy = sin(aAngles.yaw * 0.5f); */

    /*     math::Vec<4, float> v{ */
    /*         sr * cp * cy - cr * sp * sy, */
    /*         cr * sp * cy + sr * cp * sy, */
    /*         cr * cp * sy - sr * sp * cy, */
    /*         cr * cp * cy + sr * sp * sy, */
    /*     }; */
    /*     v.normalize(); */

    /*     return math::Quaternion<float>{ */
    /*         v.x(), */
    /*         v.y(), */
    /*         v.z(), */
    /*         v.w(), */
    /*     }; */
    /* } */

    /* void presentPoseEditor(ent::Handle<ent::Entity> aEntityHandle) */
    /* { */
    /*     ImGui::Begin("Pose"); */

    /*     /1* Geometry & geometry = getGeometry(aEntityHandle); *1/ */
    /*     /1* ImGui::InputFloat3("Position", geometry.mPosition.data()); *1/ */
    /*     /1* ImGui::InputFloat("Scaling", &geometry.mScaling); *1/ */
    /*     /1* ImGui::InputFloat3("Instance scaling", geometry.mInstanceScaling.data()); *1/ */

    /*     /1* EulerAngles<math::Radian> angles = toEulerAngles(geometry.mOrientation); *1/ */
    /*     /1* EulerAngles<math::Degree> degrees = angles.as<math::Degree>(); *1/ */
    /*     /1* if(ImGui::InputFloat3("Orientation", (float*)&degrees)) *1/ */
    /*     /1* { *1/ */
    /*     /1*     angles = degrees.as<math::Radian>(); *1/ */
    /*     /1*     geometry.mOrientation = toQuaternion(angles); *1/ */
    /*     /1* } *1/ */

    /*     ImGui::End(); */
    /* } */

    
} // unnamed namespace


void SceneEditor::showEditor(ent::EntityManager & aManager)
{
    ImGui::Begin("Entities");
    
    ImGui::BeginChild("left pane", ImVec2(150, 0), true);
    aManager.forEachHandle([this](ent::Handle<ent::Entity> aHandle, const char * aName)
    {
        presentNode(aHandle, aName);

    });
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("Entity components");
    // Note: when changing scene, selected node might become invalid.
    // Address it naively by testing.
    if(mSelected.isValid())
    {
        presentComponents(mSelected);
    }
    ImGui::EndChild();

    ImGui::End();
}

template<class T_component>
void drawDebugComponentUi(ent::Handle<ent::Entity> aHandle)
{
    if (aHandle.get()->has<T_component>())
    {
        snac::getComponent<T_component>(aHandle).drawUi();
    }
}

void SceneEditor::presentComponents(ent::Handle<ent::Entity> aHandle)
{
    drawDebugComponentUi<component::Geometry>(aHandle);
    drawDebugComponentUi<component::GlobalPose>(aHandle);
    drawDebugComponentUi<component::Controller>(aHandle);
    drawDebugComponentUi<component::AllowedMovement>(aHandle);
    drawDebugComponentUi<component::ChangeSize>(aHandle);
}


void SceneEditor::presentNode(ent::Handle<ent::Entity> aEntityHandle, const char * aName)
{
    if (ImGui::Selectable(aName, mSelected.isValid() && mSelected == aEntityHandle))
    {
        mSelected = aEntityHandle;
    }
}


} // namespace snacgame
} // namespace ad
