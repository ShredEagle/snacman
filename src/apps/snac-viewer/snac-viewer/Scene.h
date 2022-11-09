#pragma once


#include <snac-renderer/Camera.h>
#include <snac-renderer/Cube.h>
#include <snac-renderer/Render.h>

#include <graphics/AppInterface.h>

#include <math/Transformations.h>
#include <math/VectorUtilities.h>


namespace ad {
namespace snac {


InstanceStream makeInstances()
{
    std::vector<math::Matrix<4, 4, float>> transformations{
        math::trans3d::translate(math::Vec<3, float>{ 0.f, 0.f, -1.f}),
        math::trans3d::translate(math::Vec<3, float>{-4.f, 0.f, -1.f}),
        math::trans3d::translate(math::Vec<3, float>{ 4.f, 0.f, -1.f}),
    };

    InstanceStream instances{
        .mInstanceCount = (GLsizei)std::size(transformations),
    };
    std::size_t instanceSize = sizeof(decltype(transformations)::value_type);
    instances.mInstanceBuffer.mStride = instanceSize;

    bind(instances.mInstanceBuffer.mBuffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        instanceSize * std::size(transformations),
        transformations.data(),
        GL_DYNAMIC_DRAW);

    {
        graphics::ClientAttribute transformation{
            .mDimension = 16,
            .mOffset = 0,
            .mDataType = GL_FLOAT,
        };
        instances.mAttributes.emplace(Semantic::LocalToWorld, VertexStream::Attribute{0, transformation});
    }
    return instances;
}


struct Scene
{
    Scene(graphics::AppInterface & aAppInterface);

    void update();

    void render(Renderer & aRenderer) const;

    Mesh mMesh{makeCube()};
    InstanceStream mInstances{makeInstances()};
    Camera mCamera;
    OrbitalControl mCameraControl;

    std::shared_ptr<graphics::AppInterface::SizeListener> mSizeListener;
};


inline Scene::Scene(graphics::AppInterface & aAppInterface) :
    mCamera{math::getRatio<float>(aAppInterface.getFramebufferSize())},
    mCameraControl{aAppInterface.getWindowSize()}
{
    using namespace std::placeholders;

    aAppInterface.registerCursorPositionCallback(
        std::bind(&OrbitalControl::callbackCursorPosition, &mCameraControl, _1, _2));

    aAppInterface.registerMouseButtonCallback(
        std::bind(&OrbitalControl::callbackMouseButton, &mCameraControl, _1, _2, _3, _4, _5));

    aAppInterface.registerScrollCallback(
        std::bind(&OrbitalControl::callbackScroll, &mCameraControl, _1, _2));

    mSizeListener = aAppInterface.listenWindowResize(
        [this](const math::Size<2, int> & size)
        {
            mCamera.setAspectRatio(math::getRatio<float>(size)); 
            mCameraControl.setWindowSize(size);
        });
}


inline void Scene::update()
{
    mCamera.setWorldToCamera(mCameraControl.getParentToLocal());
}


inline void Scene::render(Renderer & aRenderer) const
{
    clear();
    aRenderer.render(mMesh, mCamera, mInstances);
}

} // namespace snac
} // namespace ad
