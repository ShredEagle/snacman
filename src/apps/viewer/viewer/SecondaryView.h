#pragma once


#include "CameraSystem.h"

#include <graphics/AppInterface.h>

#include <memory>


namespace ad::renderer {


struct ViewerApplication;


struct SecondaryView
{
    SecondaryView(std::shared_ptr<graphics::AppInterface> aGlfwAppInterface);

    // TODO: const the viewer app
    void render(/*const*/ ViewerApplication & aViewerApp);

    std::shared_ptr<graphics::AppInterface> mAppInterface;
    CameraSystem mCameraSystem;
};


} // namespace ad::renderer