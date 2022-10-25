#pragma once

#include "GraphicState.h"

#include <graphics/2d/Shaping.h>

#include <math/Transformations.h>


namespace ad {
namespace bawls {


class Renderer
{
public:
    explicit Renderer(math::Size<2, GLfloat> aWindowSize_world)
    {
        mCameraProjection.setCameraTransformation(GetProjection(aWindowSize_world));
    }

    void render(const GraphicState & aState)
    {
        // TODO get rid of this loop, the graphic state should be the expected type
        std::vector<graphics::r2d::Shaping::Circle> balls;
        for (const auto & ballState : aState.balls)
        {
            balls.emplace_back(ballState.position, ballState.radius);
        };
        mBalls.resetCircles(balls);

        mShaping.render(mBalls, mCameraProjection);
    }

private:
    static math::AffineMatrix<4, GLfloat> GetProjection(math::Size<2, GLfloat> aWindowSize_world)
    {
        constexpr GLfloat gFarPlane = 1.f;

        return math::trans3d::orthographicProjection<GLfloat>(
            math::Box<GLfloat>{
                math::Position<3, GLfloat>::Zero(),
                {aWindowSize_world, 2 * gFarPlane}
            }.centered());
    }

    graphics::r2d::ShapeSet mBalls;
    graphics::r2d::Shaping mShaping;

    // The camera projection remains constant in this scene
    // (but not made const, because it would prevent move construction)
    graphics::CameraProjection mCameraProjection;
};

} // namespace bawls
} // namespace ad
