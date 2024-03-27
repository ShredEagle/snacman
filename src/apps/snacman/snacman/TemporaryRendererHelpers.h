#pragma once


#include <snac-renderer-V2/Handle.h>
#include <snac-renderer-V2/Model.h>

#include "simulations/snacgame/component/VisualModel.h"


namespace ad::snacgame {


// TODO Ad 2024/03/26: #RV2 API get rid of those, or refactor and relocate them

inline renderer::Handle<renderer::Object> recurseToObject(const renderer::Node & aNode)
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

inline renderer::Handle<renderer::Object> extractObject(const component::VisualModel & aVisualModel)
{
    auto result = recurseToObject(*aVisualModel.mModel);

    // TODO we also need to ensure unicity of the underlying, but it would be better
    // to simply host objects directly...
    if(result == renderer::gNullHandle)
    {
        throw std::logic_error{"There should have been an object in the Node hierarchy of a VisualModel"};
    }
    else
    {
        return result;
    }
}


} // namespace ad::snacgame