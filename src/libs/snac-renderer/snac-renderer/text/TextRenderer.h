#pragma once


#include "Text.h"

#include "../Render.h"

#include <span>


namespace ad {
namespace snac {


class TextRenderer
{
public:
    TextRenderer();

    void respecifyInstanceData(std::span<GlyphInstance> aInstances);

    void render(const Font & aFont, Renderer & aRenderer, snac::ProgramSetup & aProgramSetup);

private:
    snac::Pass mTextPass{"text"};
    snac::InstanceStream mGlyphInstances;
};


} // namespace snac
} // namespace ad
