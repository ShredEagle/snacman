#include "Scene.h"

#include <imgui.h>


namespace ad::renderer {


namespace {


    void showPartWindow()
    {
        ImGui::Begin("Part");
        ImGui::Text("part window");
        ImGui::End();
    }


    void presentObject(const Object & aObject)
    {
        assert(!aObject.mParts.empty());
        if(ImGui::TreeNodeEx("[Parts]"))
        {
            unsigned int partIdx = 0;
            for(const Part & part : aObject.mParts)
            {
                // TODO have a notion of base flags, something static in the class
                ImGuiTreeNodeFlags partFlags = /*baseFlags |*/ ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen; // ImGuiTreeNodeFlags_Bullet
                if(part.mName.empty())
                {
                    ImGui::TreeNodeEx((void*)(uintptr_t)partIdx, partFlags, "<part_%i>", partIdx);
                }
                else
                {
                    ImGui::TreeNodeEx((void*)(uintptr_t)partIdx, partFlags, "%s", part.mName.c_str());
                }

                // TODO what does ImGui::IsItemToggledOpen() do?
                if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
                {
                    // do per part stuff, probably requires user (not imgui) managed state
                }

                ++partIdx;
            }
            ImGui::TreePop();
        }
    }


    const Node * presentNodeTree(const Node & aNode, unsigned int & aIndex)
    {
        //auto & nodeTree = mSkin.mRig.mScene;
    
        const Node * selected = nullptr;

        //static constexpr ImGuiTreeNodeFlags baseFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
        constexpr ImGuiTreeNodeFlags leafFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
        int flags = (aNode.mChildren.empty() && aNode.mInstance.mObject == nullptr) ? leafFlags : 0;
        // TODO Ad 2023/11/07: Address this indexing non-sense
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
            if(const Object * object = aNode.mInstance.mObject)
            {
                presentObject(*object);
            }

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