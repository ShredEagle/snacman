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

    void present(const Scene & aScene);

private:
    const Node * presentNodeTree(const Node & aNode, unsigned int & aIndex);
    void presentObject(const Object & aObject);
    void presentSelection();

    void showPartWindow(const Part & aPart) const;

    static const int gBaseFlags;
    static const int gLeafFlags;
    static const int gPartFlags;

    Handle<const Part> mSelectedPart = gNullHandle;

    const Storage & mStorage;
};


} // namespace ad::renderer