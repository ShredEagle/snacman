#include "SceneGui.h"


#include "Scene.h"

#include "runtime_reflect/DearImguiVisitor.h"

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


void SceneGui::presentSection(Scene & aScene, const Timing & aTime)
{
    if(ImGui::BeginMenuBar())
    {
        if(ImGui::BeginMenu("Options"))
        {
            ImGui::MenuItem("Highlight objects", NULL, &mOptions.mHighlightObjects);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    if (ImGui::CollapsingHeader("Scene Tree"))
    {
        Node * hovered = presentNodeTree(aScene.mRoot, 0, aTime);

        handleHighlight(hovered);
        presentSelection();
    }

    if (ImGui::CollapsingHeader("Lights"))
    {
        presentLights(aScene.mLights_world);
    }
}


void SceneGui::handleHighlight(Node * aHovered)
{
    if(mHighlightedNode != aHovered)
    {
        if(mHighlightedNode)
        {
            mHighlightedNode->mInstance.mMaterialOverride = std::nullopt;
        }
        
        if(aHovered && mOptions.mHighlightObjects)
        {
            aHovered->mInstance.mMaterialOverride = mHighlightMaterial;
        }

        mHighlightedNode = aHovered;
    }
}


// Note: currently returns the selected Node (but not Part or any heterogeneous type).
// This is inherited from the time before this was free function, not sure it can be usefull now.
Node * SceneGui::presentNodeTree(Node & aNode, unsigned int aIndex, const Timing & aTime)
{
    //auto & nodeTree = mSkin.mRig.mScene;
    Node * hovered = nullptr;

    // Optional object associated with this node
    const Object * object = aNode.mInstance.mObject;

    int flags = (aNode.mChildren.empty() && aNode.mInstance.mObject == nullptr) ? gLeafFlags : gBaseFlags;
    // TODO Ad 2023/11/09: #dod Currently, the node index is relative to the parent, not absolute regarding all nodes
    // If we ever move to a Data-oriented model, we could probably use the absolute index of the node in the data store.
    const std::string name{"<node_" + std::to_string(aIndex) + ">"};

    if(object)
    {
        static const ImVec4 gImPink(1.0f, 0.0f, 1.0f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, gImPink);
    }
    bool opened = ImGui::TreeNodeEx(&aNode, flags, "%s",
            // TODO Ad 2023/10/31: We have a discrepencies in our data model between how Assimp models Nodes, and our distiction between Nodes and Objects
            (aNode.mInstance.mName.empty() ? name.c_str() : aNode.mInstance.mName.c_str()));
    if(object)
    {
        ImGui::PopStyleColor();
    }
    

    ImGui::PushID(&aNode);

    if(ImGui::IsItemHovered())
    {
        hovered = &aNode;
    }

    if(ImGui::IsItemClicked(0))
    {
        mSelectedNode = &aNode;
    }

    if(opened)
    {
        if(object)
        {
            presentObject(*object, aNode.mInstance, aTime);
        }

        unsigned int childIdx = 0;
        for(Node & child : aNode.mChildren)
        {
            if(Node * hoveredChild = presentNodeTree(child, childIdx++, aTime);
               hoveredChild)
            {
                hovered = hoveredChild;
            }
            
        }
        ImGui::TreePop();
    }

    ImGui::PopID();

    return hovered;
}


void SceneGui::presentAnimations(Handle<AnimatedRig> mAnimatedRig,
                                 Instance & aInstance,
                                 const Timing & aTime)
{
    if(ImGui::TreeNodeEx("[Animations]", gBaseFlags))
    {
        int flags = gLeafFlags;
        for(const auto & [key, rigAnimation] : mAnimatedRig->mNameToAnimation)
        {
            bool opened = ImGui::TreeNodeEx(&rigAnimation, flags, "%s", rigAnimation.mName.c_str());
            if(opened)
            {
                ImGui::TreePop();
            }
            if(isItemDoubleClicked())
            {
                aInstance.mAnimationState = AnimationState{
                    .mAnimation = &rigAnimation,
                    .mStartTimepoint = aTime.mSimulationTimepoint,
                };
            }
        }

        ImGui::TreePop();
    }
}


void SceneGui::presentLights(LightsDataUi & aLightsData)
{
    ImGui::Checkbox("Show point lights", &mOptions.mShowPointLights);
    ImGui::Checkbox("Directional lights in camera space", 
                     &mOptions.mAreDirectionalLightsCameraSpace);
    DearImguiVisitor v;
    r(v, aLightsData);
}


// When the main scene graph in our Model becomes a DOD NodeTree, this should be 
// factorized with main traversal in presentNodeTree()
void SceneGui::presentJointTree(const NodeTree<Rig::Pose> & aTree,
                                NodeTree<Rig::Pose>::Node::Index aNodeIdx)
{
    using Node = NodeTree<Rig::Pose>::Node;

    int flags = (aTree.hasChild(aNodeIdx)) ? gBaseFlags : gLeafFlags;

    std::string name = "<Bone #" + std::to_string(aNodeIdx) + ">";
    if(auto nameIt = aTree.mNodeNames.find(aNodeIdx); nameIt != aTree.mNodeNames.end())
    {
        name = nameIt->second;
    }

    const Node & node = aTree.mHierarchy[aNodeIdx];
    bool opened = ImGui::TreeNodeEx(&node, flags, "%s", name.c_str());

    if(opened)
    {
        if(aTree.hasChild(aNodeIdx))
        {
            presentJointTree(aTree, node.mFirstChild);
        }
        ImGui::TreePop();
    }

    if(node.mNextSibling != Node::gInvalidIndex)
    {
        presentJointTree(aTree, node.mNextSibling);
    }
};


void SceneGui::presentObject(const Object & aObject, Instance & aInstance, const Timing & aTime)
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

    if(aObject.mAnimatedRig != gNullHandle)
    {
        // TODO I suspect this shared name is not ensuring unicity of the "RIG" node state.
        if(ImGui::TreeNodeEx("[Rig]", gBaseFlags))
        {
            presentAnimations(aObject.mAnimatedRig, aInstance, aTime);
            presentJointTree(aObject.mAnimatedRig->mRig.mJointTree,
                             aObject.mAnimatedRig->mRig.mJointTree.mFirstRoot);
            ImGui::TreePop();
        }
    }
}


void SceneGui::showNodeWindow(Node & aNode)
{
    ImGui::Begin("Node");
    ImGui::SeparatorText("Pose");
    Pose & pose = aNode.mInstance.mPose;
    ImGui::InputFloat3("Position", pose.mPosition.data());
    ImGui::InputFloat("Uniform scale", &pose.mUniformScale);
    ImGui::End();
}


void SceneGui::showPartWindow(const Part & aPart)
{
    ImGui::Begin("Part");
    ImGui::PushID(&aPart);  // Otherwise, the Effect node would be in the same state 
                            // for distinct parts using the same effect.

    // Not sure we want to repeat the name of the part here (we have a complication for unnamed parts)?
    ImGui::Text("%s", aPart.mName.c_str());

    // TODO Ad 2023/11/08: Instead of showing indices count, higher level logic showing the currect number and type of primities (dependso on mode)
    ImGui::Text("mode: %s, %i indices", graphics::to_string(aPart.mPrimitiveMode).c_str(), aPart.mIndicesCount);

    // Vertex
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
                ImGui::BulletText("%s", semantic->c_str());
            }
            ImGui::TreePop();

            ImGui::BulletText("per instance");
            ImGui::TreePush("per instance");
            for(const std::string * semantic : perDivisor[1])
            {
                ImGui::BulletText("%s", semantic->c_str());
            }
            ImGui::TreePop();
        }
        ImGui::TreePop();
    }

    ImGui::NewLine();

    // Material (i.e. GenericMaterial / material properties name)
    ImGui::Text("material: %s", getName(aPart.mMaterial, mStorage).c_str());

    // Effect
    presentEffect(aPart.mMaterial.mEffect);

    ImGui::PopID();
    ImGui::End();
}


void SceneGui::presentEffect(Handle<const Effect> aEffect)
{
    if(ImGui::TreeNodeEx((void*)aEffect, gBaseFlags, "effect: %s", getName(aEffect, mStorage).c_str()))
    {
        const Effect & effect = *aEffect;
        unsigned int techniqueIdx = 0;
        for(const Technique & technique : effect.mTechniques)
        {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetTreeNodeToLabelSpacing());
            ImGui::Text("<technique_%i>", techniqueIdx);

            {
                ImGui::TreePush(&technique);

                ImGui::BulletText("annotations");
                {
                    ImGui::TreePush("annotations");
                    for(const auto & [key, value] : technique.mAnnotations)
                    {
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetTreeNodeToLabelSpacing());
                        ImGui::Text("%s: %s", key.c_str(), value.c_str());
                    }
                    ImGui::TreePop();
                }

                const IntrospectProgram & introspect = technique.mConfiguredProgram->mProgram;
                const char * programName = introspect.mName.c_str();

                if(ImGui::TreeNodeEx(programName, gBaseFlags))
                {
                    presentShaders(introspect);
                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }

            ++techniqueIdx;
        }
        ImGui::TreePop();
    }
}


void SceneGui::presentShaders(const renderer::IntrospectProgram & aIntrospectProgram)
{
    for(const auto & [type, shader] : aIntrospectProgram.mSources)
    {
        const std::string * sourceString = &shader.getSource();
        ImGuiTreeNodeFlags shaderFlags = 
            gPartFlags
            | ((mSelectedShaderSource == sourceString) ? ImGuiTreeNodeFlags_Selected : 0);

        ImGui::TreeNodeEx((void*)sourceString, shaderFlags, "%s", graphics::to_string(type).c_str());

        if (isItemDoubleClicked())
        {
            mSelectedShaderSource = sourceString;
        }
    }
}


void SceneGui::showSourceWindow(const std::string & aSourceString)
{
    ImGui::Begin("Shader source");

    ImGui::Text("%s", aSourceString.c_str());

    ImGui::End();
}

void SceneGui::presentSelection()
{
    if(mSelectedNode)
    {
        showNodeWindow(*mSelectedNode);
    }

    if (mSelectedPart)
    {
        showPartWindow(*mSelectedPart);
    }

    if (mSelectedShaderSource)
    {
        showSourceWindow(*mSelectedShaderSource);
    }
}


} // namespace ad::renderer