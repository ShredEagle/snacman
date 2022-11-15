#pragma once


#include "Renderer.h"

#include <memory>


namespace ad {
namespace cubes {


class Cubes
{
public:
    using Renderer_t = Renderer;

    void update(float aDelta);

    std::unique_ptr<GraphicState> makeGraphicState() const; 
};


} // namespace cubes
} // namespace ad
