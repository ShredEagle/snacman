#pragma once


#include "Model.h"


namespace ad::renderer {


struct Scene;


class SceneGui
{
public:
    SceneGui(const Storage & aStorage) :
        mStorage{aStorage}
    {}

    void present(Scene & aScene);

private:
    void presentNodeTree(Node & aNode, unsigned int aIndex);
    void presentObject(const Object & aObject);
    void presentEffect(Handle<const Effect> aEffect);
    void presentShaders(const IntrospectProgram & aIntrospectProgram);

    void presentSelection();

    void showNodeWindow(Node & aNode);
    void showPartWindow(const Part & aPart);
    void showSourceWindow(const std::string & aSourceString);

    static const int gBaseFlags;
    static const int gLeafFlags;
    static const int gPartFlags;

    Handle<const Part> mSelectedPart = gNullHandle;
    // There are no handles on Nodes at the moment
    // The node Pose might be mutated, so it cannot be const
    Node * mSelectedNode = nullptr;
    const std::string * mSelectedShaderSource = nullptr;

    const Storage & mStorage;
};


} // namespace ad::renderer