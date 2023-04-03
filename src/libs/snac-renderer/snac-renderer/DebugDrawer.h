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


/// @note Several instance of DebugDrawers can live with an application.
/// They will all write to the same shared list of commands,
/// but allow for filtering on channels (i.e. names) and severity (i.e. levels).
class DebugDrawer
{
    // TODO remove
    friend Guard initializeDebugDrawing(Load<Technique> & aTechniqueAccess);

    struct SharedData
    {
        Mesh mCube;
        Mesh mArrow;
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
        std::vector<Entry> mArrows;
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


    /// @brief Registry of all DebugDrawers, accessed by associated name.
    class Registry
    {
        friend class DebugDrawer;
        // TODO remove
        friend Guard initializeDebugDrawing(Load<Technique> & aTechniqueAccess);

        Registry()
        {
            assert(gSharedData != nullptr);
        }

        void startFrame();
        DrawList endFrame();

        std::shared_ptr<DebugDrawer> addDrawer(const std::string & aName);

        std::shared_ptr<DebugDrawer> get(const std::string & aName) const;

        static Registry & GetInstance()
        {
            static Registry gInstance;
            return gInstance;
        }

        std::unordered_map<std::string/*logger name*/, std::shared_ptr<DebugDrawer>> mDrawers;
        std::shared_ptr<Commands> mCommands;
        static std::unique_ptr<SharedData> gSharedData;
    };

    /// @brief Default-constructed instance of this class allow to iterate all drawers in range-based for loops.
    struct IterateDrawers;

public:
    // Should be private, but we want to use it with make_shared 
    DebugDrawer(Registry * aRegistry) :
        mRegistry{aRegistry}
    {}

    static void StartFrame()
    { GetRegistry().startFrame(); }

    static DrawList EndFrame()
    { return GetRegistry().endFrame(); }

    static std::shared_ptr<DebugDrawer> Get(const std::string & aName)
    { return GetRegistry().get(aName); }

    static std::shared_ptr<DebugDrawer> AddDrawer(const std::string & aName)
    { return GetRegistry().addDrawer(aName); }

    static auto Begin()
    { return GetRegistry().mDrawers.begin(); }

    static auto End()
    { return GetRegistry().mDrawers.end(); }

    /// @brief Add a unit box (from [0, 0, 0] to [1, 1, 1] with pose defined by `aEntry`.
    void addBox(const Entry & aEntry);

    /// @brief Add a unit the box `aBox` with pose defined by `aEntry`.
    void addBox(Entry aEntry, const math::Box<GLfloat> aBox);

    /// @brief Add unit arrow along positive Y.
    /// @param aEntry 
    void addArrow(const Entry & aEntry);

    /// @brief Add a frame-of-reference as 3 arrows along positive X(red), Y(green) and Z(blue).
    /// @note The color in `aEntry` will be ignored. 
    void addBasis(Entry aEntry);

private:
    Commands & commands();

    static Registry & GetRegistry()
    { return Registry::GetInstance(); }

    Registry * mRegistry;
    //Level mLevel{Level::Off};
};


struct DebugDrawer::IterateDrawers
{
    auto begin()
    { return DebugDrawer::Begin(); }

    auto end()
    { return DebugDrawer::End(); }
};


} // namespace snac
} // namespace ad
