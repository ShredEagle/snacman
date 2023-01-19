#include "Renderer.h"

#include <math/Transformations.h>

#include <snac-renderer/Cube.h>


namespace ad {
namespace snacgame {


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
            .mDimension = {4, 4},
            .mOffset = offsetof(PoseColor, pose),
            .mComponentType = GL_FLOAT,
        };
        instances.mAttributes.emplace(snac::Semantic::LocalToWorld, transformation);
    }
    {
        graphics::ClientAttribute albedo{
            .mDimension = 4,
            .mOffset = offsetof(PoseColor, albedo),
            .mComponentType = GL_UNSIGNED_BYTE,
        };
        instances.mAttributes.emplace(snac::Semantic::Albedo, albedo);
    }
    instances.mInstanceBuffer.mStride = sizeof(PoseColor);
    return instances;
}


Renderer::Renderer(float aAspectRatio, snac::Camera::Parameters aCameraParameters, const resource::ResourceFinder & aResourceFinder) :
    mCamera{aAspectRatio, aCameraParameters},
    mCubeMesh{.mStream = snac::makeCube()},
    mInstances{initializeInstanceStream()}
{
    mCubeMesh.mMaterial = std::make_shared<snac::Material>();
    mCubeMesh.mMaterial->mEffect = std::make_shared<snac::Effect>(snac::Effect{
        .mProgram = {
            graphics::makeLinkedProgram({
                {GL_VERTEX_SHADER, graphics::ShaderSource::Preprocess(*aResourceFinder.find("shaders/Cubes.vert"))},
                {GL_FRAGMENT_SHADER, graphics::ShaderSource::Preprocess(*aResourceFinder.find("shaders/Cubes.frag"))},
            }),
            "PhongLighting"
        }
    });
}

void Renderer::resetProjection(float aAspectRatio, snac::Camera::Parameters aParameters)
{
    mCamera.resetProjection(aAspectRatio, aParameters);
}


void Renderer::render(const visu::GraphicState & aState)
{
    // Stream the instance buffer data
    std::vector<PoseColor> instanceBufferData;
    for (const visu::Entity & entity : aState.mEntities)
    {
        instanceBufferData.push_back(PoseColor{
            .pose = 
                math::trans3d::scale(entity.mScaling)
                * math::trans3d::rotateY(entity.mYAngle)
                * math::trans3d::translate(entity.mPosition_world.as<math::Vec>()),
            .albedo = to_sdr(entity.mColor),
        });
    }

    // Position camera
    //mCamera.setWorldToCamera(math::trans3d::translate(-aState.mCamera.mPosition_world.as<math::Vec>()));
    mCamera.setWorldToCamera(aState.mCamera.mWorldToCamera);

    math::hdr::Rgb_f lightColor =  to_hdr<float>(math::sdr::gBlue);
    math::Position<3, GLfloat> lightPosition{0.f, 0.f, 0.f};
    math::hdr::Rgb_f ambientColor =  math::hdr::Rgb_f{0.1f, 0.2f, 0.1f};
    
    snac::UniformRepository uniforms{
        {snac::Semantic::LightColor, snac::UniformParameter{lightColor}},
        {snac::Semantic::LightPosition, {lightPosition}},
        {snac::Semantic::AmbientColor, {ambientColor}},
    };

    snac::UniformBlocks uniformBlocks{
         {snac::BlockSemantic::Viewing, &mCamera.mViewing},
    };

    mInstances.respecifyData(std::span{instanceBufferData});
    mRenderer.render(mCubeMesh, mInstances, uniforms, uniformBlocks);
}


} // namespace cubes
} // namespace ad
