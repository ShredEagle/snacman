#pragma once


namespace ad::renderer {


struct DebugDrawGui
{
    void present();

    bool mShow = false;

    bool mRenderToMainView = false;
    bool mRenderToSecondaryView = true;
};


} // namespace ad::renderer