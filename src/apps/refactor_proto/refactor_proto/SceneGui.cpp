#include "SceneGui.h"

#include "Scene.h"

#include <imgui.h>


namespace ad::renderer {


namespace {


// see: https://github.com/ocornut/imgui/issues/1872
bool isItemDoubleClicked(ImGuiMouseButton mouse_button = 0)
{
    return
        ImGui::IsMouseDoubleClicked(mouse_button) 
        && ImGui::IsItemHovered();
}


GLuint getInstanceDivisor(const VertexStream & aStream, const AttributeAccessor & aAccessor)
{
    GLuint divisor = aStream.mVertexBufferViews.at(aAccessor.mBufferViewIndex).mInstanceDivisor;
    // Note: As of this writting, I do not think we have use for divisors other than 0 or 1.
    assert(divisor <= 1);
    return divisor;
}


} // unnamed namespace


const ImGuiTreeNodeFlags SceneGui::gBaseFlags = 
    ImGuiTreeNodeFlags_OpenOnArrow 
    | ImGuiTreeNodeFlags_OpenOnDoubleClick 
    | ImGuiTreeNodeFlags_SpanAvailWidth;

const ImGuiTreeNodeFlags SceneGui::gLeafFlags = 
    SceneGui::gBaseFlags 
    | ImGuiTreeNodeFlags_Leaf 
    | ImGuiTreeNodeFlags_Bullet;

const ImGuiTreeNodeFlags SceneGui::gPartFlags = 
    SceneGui::gLeafFlags 
    | ImGuiTreeNodeFlags_NoTreePushOnOpen; // ImGuiTreeNodeFlags_Bullet


void SceneGui::present(const Scene & aScene)
{
    unsigned int index = 0;
    for(const Node & topNode : aScene.mRoot)
    {
        presentNodeTree(topNode, index);
    }

    presentSelection();
}


// Note: currently returns the selected Node (but not Part or any heterogeneous type).
// This is inherited from the time before this was free function, not sure it can be usefull now.
const Node * SceneGui::presentNodeTree(const Node & aNode, unsigned int & aIndex)
{
    //auto & nodeTree = mSkin.mRig.mScene;

    const Node * selected = nullptr;

    int flags = (aNode.mChildren.empty() && aNode.mInstance.mObject == nullptr) ? gLeafFlags : gBaseFlags;
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


void SceneGui::presentObject(const Object & aObject)
{
    assert(!aObject.mParts.empty());
    if(ImGui::TreeNodeEx("[Parts]", gBaseFlags))
    {
        unsigned int partIdx = 0;
        for(const Part & part : aObject.mParts)
        {
            ImGuiTreeNodeFlags partFlags = 
                gPartFlags
                | ((mSelectedPart == &part) ? ImGuiTreeNodeFlags_Selected : 0);

            if(part.mName.empty())
            {
                ImGui::TreeNodeEx((void*)(uintptr_t)partIdx, partFlags, "<part_%i>", partIdx);
            }
            else
            {
                ImGui::TreeNodeEx((void*)(uintptr_t)partIdx, partFlags, "%s", part.mName.c_str());
            }

            // Note: IsItemToggledOpen() is true when the treenode is opened via arrow
            if (isItemDoubleClicked() 
                // Not usefull since we check for double clicked
                //&& !ImGui::IsItemToggledOpen()/* clicking the arrow does not alter selection*/
            )
            {
                mSelectedPart = &part;
            }

            ++partIdx;
        }

        ImGui::TreePop();
    }
}


void SceneGui::showPartWindow(const Part & aPart) const
{
    ImGui::Begin("Part");

    // Not sure we want to repeat the name of the part here (we have a complication for unnamed parts)?
    ImGui::Text("%s", aPart.mName.c_str());

    // TODO Ad 2023/11/08: Instead of showing indices count, higher level logic showing the currect number and type of primities (dependso on mode)
    ImGui::Text("mode: %s, %i indices", graphics::to_string(aPart.mPrimitiveMode).c_str(), aPart.mIndicesCount);

    // if opened
    if(ImGui::TreeNodeEx(&aPart, gBaseFlags, "%i vertices", aPart.mVertexCount))
    {
        const VertexStream & vertexStream = *aPart.mVertexStream;
        // Sort the vertex attributes per divisor
        std::array<std::vector<const std::string *>, 2> perDivisor;
        for(const auto & [semantic, accessor] : vertexStream.mSemanticToAttribute)
        {
            perDivisor[getInstanceDivisor(vertexStream, accessor)].push_back(&semantic);
        }
        // Show the attributes
        {
            ImGui::BulletText("per vertex");
            ImGui::TreePush("per vertex");
            for(const std::string * semantic : perDivisor[0])
            {
                ImGui::BulletText(semantic->c_str());
            }
            ImGui::TreePop();

            ImGui::BulletText("per instance");
            ImGui::TreePush("per instance");
            for(const std::string * semantic : perDivisor[1])
            {
                ImGui::BulletText(semantic->c_str());
            }
            ImGui::TreePop();
        }
        ImGui::TreePop();
    }

    ImGui::Text("material: %s", getName(aPart.mMaterial, mStorage).c_str());

    ImGui::End();
}


void SceneGui::presentSelection()
{
    if (mSelectedPart)
    {
        showPartWindow(*mSelectedPart);
    }
}


} // namespace ad::renderer