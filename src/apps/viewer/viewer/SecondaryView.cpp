#include "SecondaryView.h"

#include "ViewerApplication.h"

#include <graphics/CameraUtilities.h>

#include <snac-renderer-V2/Profiling.h>


namespace ad::renderer {


SecondaryView::SecondaryView(std::shared_ptr<graphics::AppInterface> aGlfwAppInterface) :
    mAppInterface{std::move(aGlfwAppInterface)},
    mCameraSystem{mAppInterface, nullptr /* no inhibiter*/, CameraSystem::Control::FirstPerson}
{
    mCameraSystem.mCamera.setupOrthographicProjection(Camera::OrthographicParameters{
        .mAspectRatio = 1,
        .mViewHeight = 2,
        .mNearZ = 0,
        .mFarZ = -100,
    });
    mCameraSystem.mCamera.setPose(
        graphics::getCameraTransform<GLfloat>({0.f, 5.f, 0.f},
                                              {0.f, -1.f, 0.f},
                                              {0.f, 0.f, -1.f}));
}

void SecondaryView::render(ViewerApplication & aViewerApp)
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "SecondaryView::render()", CpuTime, GpuTime, BufferMemoryWritten);

    aViewerApp.mGraph.renderFrame(aViewerApp.mScene,
                                  mCameraSystem.mCamera,
                                  aViewerApp.mScene.getLightsInCamera(mCameraSystem.mCamera,
                                                                      !aViewerApp.mSceneGui.getOptions().mAreDirectionalLightsCameraSpace),
                                  aViewerApp.mStorage);
}


} // namespace ad::renderer