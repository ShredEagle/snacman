#include "Scene.h"

#include <imgui.h>


namespace ad::renderer {


namespace {

    const Node * presentNodeTree(const Node & aNode, unsigned int & aIndex)
    {
        //auto & nodeTree = mSkin.mRig.mScene;

        const Node * selected = nullptr;

        int flags = aNode.mChildren.empty() ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet : 0;
        const std::string name{"Node_" + std::to_string(aIndex)};
        bool opened = ImGui::TreeNodeEx(&aNode, flags, "%s",
             // TODO Ad 2023/10/31: We have a discrepencies in our data model between how Assimp models Nodes, and our distiction between Nodes and Objects
             (aNode.mInstance.mName.empty() ? name.c_str() : aNode.mInstance.mName.c_str()));

        ImGui::PushID(&aNode);

        if(ImGui::IsItemClicked(0))
        {
            selected = &aNode;
        }

        if(opened)
        {
            for(const Node & child : aNode.mChildren)
            {
                if(const Node * childSelected = presentNodeTree(child, ++aIndex);
                childSelected != nullptr)
                {
                    selected = childSelected;
                }
            }
            ImGui::TreePop();
        }

        ImGui::PopID();

        return selected;
    }

} // unnamed namespace


void presentTree(const Scene & aScene)
{
    unsigned int index = 0;
    for(const Node & topNode : aScene.mRoot)
    {
        presentNodeTree(topNode, index);
    }
}


} // namespace ad::renderer