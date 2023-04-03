#pragma once


#include "LoadInterface.h"
#include "Mesh.h"
#include "Render.h"

#include <handy/Guard.h>

#include <math/Quaternion.h>
#include <math/Vector.h>

#include <memory>
#include <vector>


namespace ad {
namespace snac {



Guard initializeDebugDrawing(Load<Technique> & aTechniqueAccess);


class DebugDrawer
{
    friend Guard initializeDebugDrawing(Load<Technique> & aTechniqueAccess);

    struct SharedData
    {
        Mesh mCube;
        InstanceStream mInstances;
    };

public:
    struct Entry
    {
        math::Position<3, GLfloat> mPosition;
        math::Size<3, GLfloat> mScaling{1.f, 1.f, 1.f};
        math::Quaternion<GLfloat> mOrientation = math::Quaternion<GLfloat>::Identity();
        math::hdr::Rgba_f mColor = math::hdr::gMagenta<GLfloat>;
    };

    class Commands
    {
    private:
        friend class DebugDrawer;

        std::vector<Entry> mBoxes;
    };

    class DrawList
    {
    public:
        DrawList(std::shared_ptr<Commands> aCommands,
                 SharedData * aSharedData) :
            mCommands{std::move(aCommands)},
            mSharedData{aSharedData}
        {}

        // TODO remove
        static DrawList MakeEmpty()
        { return DrawList{}; };

        void render(Renderer & aRenderer, ProgramSetup & aSetup) const;

    private:
        // TODO remove
        DrawList();

        std::shared_ptr<Commands> mCommands;
        SharedData * mSharedData;
        Pass mPass{"DebugDrawing"};
    };

public:
    DebugDrawer()
    {
        assert(gSharedData != nullptr);
    }

    void startFrame();

    DrawList endFrame()
    { return {std::move(mCommands), gSharedData.get()}; }

    /// @brief Adds a unit box (from [0, 0, 0] to [1, 1, 1] with pose defined by `aEntry`.
    void addBox(const Entry & aEntry);

    /// @brief Adds a unit the box `aBox` with pose defined by `aEntry`.
    void addBox(Entry aEntry, const math::Box<GLfloat> aBox);

private:
    std::shared_ptr<Commands> mCommands;
    static std::unique_ptr<SharedData> gSharedData;
};


} // namespace snac
} // namespace ad
