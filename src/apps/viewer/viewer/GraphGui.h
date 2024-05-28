#pragma once


namespace ad::renderer {


struct TheGraph;


struct GraphGui
{
    void present(TheGraph & aGraph);

    bool mShowGraph = false;
};


} // namespace ad::renderer