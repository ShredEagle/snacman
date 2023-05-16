#include "ImguiSceneEditor.h"

#include "component/Geometry.h"
#include "component/SceneNode.h"

#include <imgui.h>

#include <string>


namespace ad {
namespace snacgame {

using component::Geometry;
using component::SceneNode;

namespace {

    const component::SceneNode & getNode(ent::Handle<ent::Entity> aEntityHandle)
    {
        assert(aEntityHandle.isValid());
        auto entity = *aEntityHandle.get();
        assert(entity.has<SceneNode>());
        return entity.get<SceneNode>();
    }

    component::Geometry & getGeometry(ent::Handle<ent::Entity> aEntityHandle)
    {
        assert(aEntityHandle.isValid());
        auto entity = *aEntityHandle.get();
        assert(entity.has<Geometry>());
        return entity.get<Geometry>();
    }

    
    template <template <class> class TT_angle>
    struct EulerAngles
    {
        TT_angle<float> roll, pitch, yaw;

        template <template <class> class TT_angle>
        EulerAngles<TT_angle> as() const
        {
            return {
                roll.as<TT_angle>(),
                pitch.as<TT_angle>(),
                yaw.as<TT_angle>(),
            };
        }
    };

    // For conversion procedures see:
    // https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
    EulerAngles<math::Radian> toEulerAngles(math::Quaternion<float> q) 
    {
        using Rad = math::Radian<float>;
        EulerAngles<math::Radian> angles;

        // roll (x-axis rotation)
        float sinr_cosp = 2 * (q.w() * q.x() + q.y() * q.z());
        float cosr_cosp = 1 - 2 * (q.x() * q.x() + q.y() * q.y());
        angles.roll = Rad{std::atan2(sinr_cosp, cosr_cosp)};

        // pitch (y-.x()is rotation)
        float sinp = std::sqrt(1 + 2 * (q.w() * q.y() - q.x() * q.z()));
        float cosp = std::sqrt(1 - 2 * (q.w() * q.y() - q.x() * q.z()));
        angles.pitch = Rad{2 * std::atan2(sinp, cosp) - math::pi<float> / 2};

        // yaw (z-.x()is rotation)
        float siny_cosp = 2 * (q.w() * q.z() + q.x() * q.y());
        float cosy_cosp = 1 - 2 * (q.y() * q.y() + q.z() * q.z());
        angles.yaw = Rad{std::atan2(siny_cosp, cosy_cosp)};

        return angles;
    }

    math::Quaternion<float> toQuaternion(EulerAngles<math::Radian> aAngles) // roll (x), pitch (Y), yaw (z)
    {
        // Abbreviations for the various angular functions

        float cr = cos(aAngles.roll * 1.5f);
        float sr = sin(aAngles.roll * 0.5f);
        float cp = cos(aAngles.pitch * 0.5f);
        float sp = sin(aAngles.pitch * 0.5f);
        float cy = cos(aAngles.yaw * 0.5f);
        float sy = sin(aAngles.yaw * 0.5f);

        math::Vec<4, float> v{
            sr * cp * cy - cr * sp * sy,
            cr * sp * cy + sr * cp * sy,
            cr * cp * sy - sr * sp * cy,
            cr * cp * cy + sr * sp * sy,
        };
        v.normalize();

        return math::Quaternion<float>{
            v.x(),
            v.y(),
            v.z(),
            v.w(),
        };
    }

    void presentPoseEditor(ent::Handle<ent::Entity> aEntityHandle)
    {
        ImGui::Begin("Pose");

        Geometry & geometry = getGeometry(aEntityHandle);
        ImGui::InputFloat3("Position", geometry.mPosition.data());
        ImGui::InputFloat("Scaling", &geometry.mScaling);
        ImGui::InputFloat3("Instance scaling", geometry.mInstanceScaling.data());

        EulerAngles<math::Radian> angles = toEulerAngles(geometry.mOrientation);
        EulerAngles<math::Degree> degrees = angles.as<math::Degree>();
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
    if(mSelected && mSelected->isValid())
    {
        presentPoseEditor(*mSelected);
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
        for(std::optional<ent::Handle<ent::Entity>> child = sceneNode.mFirstChild;
            child.has_value();
            child = getNode(*child).mNextChild)
        {
            presentNode(*child);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}


} // namespace snacgame
} // namespace ad
