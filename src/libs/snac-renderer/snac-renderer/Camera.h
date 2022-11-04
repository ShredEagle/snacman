#pragma once

#include <renderer/UniformBuffer.h>


namespace ad {
namespace snac {


struct Camera
{
    Camera();
    graphics::UniformBufferObject mViewing;
};

inline Camera::Camera()
{
    const std::array<math::Matrix<4, 4, GLfloat>, 2> identities{
        math::Matrix<4, 4, GLfloat>::Identity(),
        math::Matrix<4, 4, GLfloat>::Identity(),
    };

    load(mViewing, std::span{identities}, graphics::BufferHint::DynamicDraw);
}


} // namespace snac
} // namespace ad
