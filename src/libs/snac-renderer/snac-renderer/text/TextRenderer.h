#pragma once


#include "Text.h"

#include "../Render.h"

#include <span>


namespace ad {
namespace snac {


struct GlyphInstanceStream : InstanceStream
{
    GlyphInstanceStream();
};


class TextRenderer
{
public:
    void render(const GlyphInstanceStream & aGlyphs,
                const Font & aFont,
                Renderer & aRenderer,
                snac::ProgramSetup & aProgramSetup);

private:
    snac::Pass mTextPass{"text"};
};


} // namespace snac
} // namespace ad
