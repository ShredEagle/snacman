#pragma once


#include "Model.h"

#include <graphics/ApplicationGlfw.h>


namespace ad::renderer {


struct RenderGraph
{
    RenderGraph();

    void render(const graphics::ApplicationGlfw & aGlfwApp);

    Storage mStorage;
    std::vector<Instance> mScene;
    SemanticBufferViews mInstanceStream;
};


} // namespace ad::renderer