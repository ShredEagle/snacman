#pragma once


#include "Model.h"


namespace ad::renderer {


struct RenderGraph
{
    RenderGraph();

    void render();

    Storage mStorage;
    std::vector<Instance> mScene;
    SemanticBufferViews mInstanceStream;
};


} // namespace ad::renderer