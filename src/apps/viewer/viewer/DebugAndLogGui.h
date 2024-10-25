#pragma once


namespace ad::renderer {


struct DebugAndLogGui
{
    void present();

    bool mShowDebugDraw = false;
    bool mShowLog = false;

    bool mRenderToMainView = false;
    bool mRenderToSecondaryView = true;
};


} // namespace ad::renderer