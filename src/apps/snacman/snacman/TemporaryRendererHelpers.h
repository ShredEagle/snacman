#pragma once


#include <snac-renderer-V2/Handle.h>
#include <snac-renderer-V2/Model.h>

#include "simulations/snacgame/component/VisualModel.h"


namespace ad::snacgame {


// TODO Ad 2024/03/26: #RV2 API get rid of those, or refactor and relocate them

inline renderer::Handle<const renderer::Object> recurseToObject(const renderer::Node & aNode)
{
    if(auto object = aNode.mInstance.mObject; object != renderer::gNullHandle)
    {
        return object;
    }

    for(const renderer::Node & child : aNode.mChildren)
    {
        if(auto childResult = recurseToObject(child);
            childResult != renderer::gNullHandle)
        {
            return childResult;
        }
    }

    return renderer::gNullHandle;
}


} // namespace ad::snacgame