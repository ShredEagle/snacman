pragma #once


#include <vector>


namespace se::scenerenderer {


struct Part
{

};


struct Object
{
    std::vector<Part> mPart;
};


} // namespace se::scenerenderer