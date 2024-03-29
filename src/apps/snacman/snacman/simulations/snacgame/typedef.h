#include <entity/Entity.h>
#include <math/Angle.h>
#include <math/Box.h>
#include <math/Color.h>
#include <math/Homogeneous.h>
#include <math/LinearMatrix.h>
#include <math/Quaternion.h>

namespace ad {
namespace snacgame {
// Entity typedefs
using EntHandle = ent::Handle<ent::Entity>;
using Entity = ent::Entity;
using Phase = ent::Phase;

// 3d geometry typedefs
using Matrix3 = math::Matrix<3, 3, float>;
using TransformMatrix = math::AffineMatrix<4, float>;
using RotationMatrix = math::LinearMatrix<3, 3, float>;
using ScaleMatrix = math::LinearMatrix<3, 3, float>;
using Pos4 = math::Position<4, float>;
using Pos3 = math::Position<3, float>;
using Pos2 = math::Position<2, float>;
using Vec3 = math::Vec<3, float>;
using Vec2 = math::Vec<2, float>;
using Pos2_i = math::Position<2, int>;
using Vec2_i = math::Vec<2, int>;
using UnitVec3 = math::UnitVec<3, float>;
using Size3 = math::Size<3, float>;
using Size3_i = math::Size<3, int>;
using Turn_f = math::Turn<float>;
using Rad_f = math::Radian<float>;
using Quat_f = math::Quaternion<float>;
using Box_f = math::Box<float>;
template<class T_representation>
using HdrColor = math::hdr::Rgba<T_representation>;
using HdrColor_f = math::hdr::Rgba_f;
}
}
