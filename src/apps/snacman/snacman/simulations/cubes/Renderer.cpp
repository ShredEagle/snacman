#include "Renderer.h"

#include <math/Transformations.h>

#include <snac-renderer/Cube.h>


namespace ad {
namespace cubes {


namespace {

    struct PoseColor
    {
        math::Matrix<4, 4, float> pose;
        math::sdr::Rgba albedo;
    };
    
} // anonymous namespace


snac::InstanceStream initializeInstanceStream()
{
    snac::InstanceStream instances;
    {
        graphics::ClientAttribute transformation{
            .mDimension = 16,
            .mOffset = offsetof(PoseColor, pose),
            .mDataType = GL_FLOAT,
        };
        instances.mAttributes.emplace(snac::Semantic::LocalToWorld, snac::VertexStream::Attribute{0, transformation});
    }
    {
        graphics::ClientAttribute albedo{
            .mDimension = 4,
            .mOffset = offsetof(PoseColor, albedo),
            .mDataType = GL_UNSIGNED_BYTE,
        };
        instances.mAttributes.emplace(snac::Semantic::Albedo, snac::VertexStream::Attribute{0, albedo});
    }
    instances.mInstanceBuffer.mStride = sizeof(PoseColor);
    return instances;
}


Renderer::Renderer(float aAspectRatio, snac::Camera::Parameters aCameraParameters) :
    mCamera{aAspectRatio, aCameraParameters},
    mCubeMesh{snac::makeCube()},
    mInstances{initializeInstanceStream()}
{}


void Renderer::render(const visu::GraphicState & aState)
{
    // Stream the instance buffer data
    std::vector<PoseColor> instanceBufferData;
    for (const visu::Entity & entity : aState.mEntities)
    {
        instanceBufferData.push_back(PoseColor{
            .pose = 
                math::trans3d::rotateY(entity.mYAngle)
                * math::trans3d::translate(entity.mPosition_world.as<math::Vec>()),
            .albedo = to_sdr(entity.mColor),
        });
    }

    // Position camera
    //mCamera.setWorldToCamera(math::trans3d::translate(-aState.mCamera.mPosition_world.as<math::Vec>()));
    mCamera.setWorldToCamera(aState.mCamera.mWorldToCamera);

    mInstances.respecifyData(std::span{instanceBufferData});
    mRenderer.render(mCubeMesh, mCamera, mInstances);
}


} // namespace cubes
} // namespace ad
