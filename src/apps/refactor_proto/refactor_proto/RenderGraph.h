#pragma once


#include "Model.h"

#include <graphics/ApplicationGlfw.h>


namespace ad::renderer {


using DrawList = std::vector<Instance>;

struct Scene
{
    Scene & addToRoot(Instance aInstance)
    {
        mRoot.push_back(Node{.mInstance = std::move(aInstance)});
        return *this;
    }

    std::vector<Node> mRoot; 

    DrawList populateDrawList() const;
};

struct RenderGraph
{
    RenderGraph();

    void render(const graphics::ApplicationGlfw & aGlfwApp);

    Storage mStorage;
    Scene mScene;
    SemanticBufferViews mInstanceStream;
};


} // namespace ad::renderer