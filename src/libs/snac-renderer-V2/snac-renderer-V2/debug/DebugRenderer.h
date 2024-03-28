#pragma once

#include "../Model.h"

// TODO #debugdraw There should be a debug drawer direclty in this lib
#include <snac-renderer-V1/DebugDrawer.h>


namespace ad::renderer {


struct Loader;


// IMPORTANT: This is a Q&D (probably incomplete) implementation aimed at the V1 DebugDrawer.

/// @brief Used to render on screen the debug DrawList.
class DebugRenderer
{
public:
    DebugRenderer(Storage & aStorage, const Loader & aLoader);

    void render(snac::DebugDrawer::DrawList aDrawList,
                const RepositoryUbo & aUboRepository,
                Storage & aStorage);

private:
    void drawLines(std::span<snac::DebugDrawer::LineVertex> aLines,
                   const RepositoryUbo & aUboRepository,
                   Storage & aStorage);

    Part mLines;
    const graphics::BufferAny * mLinePositionBuffer;
    const graphics::BufferAny * mLineColorBuffer;
    Handle<ConfiguredProgram> mLineProgram;
};

} // namespace ad::renderer