#include "Text.h"

#include <snac-renderer-V2/graph/text/Font.h>


namespace ad::snacgame::component {


Text & changeString(Text & aText, const std::string & aNewString, const renderer::Font & aFont)
{
    std::tie(aText.mString, aText.mDimensions) = renderer::prepareText(aFont, aNewString);
    return aText;
}


} // namespace ad::snacgame::component