#pragma once


#include "LoadInterface.h"

#include <handy/Guard.h>

#include <math/Box.h>
#include <math/Color.h>
#include <math/Quaternion.h>
#include <math/Vector.h>

#include <memory>
#include <vector>


namespace ad {
namespace snac {



/// @note Several instance of DebugDrawers can live with an application.
/// They will all write to the same shared list of commands,
/// but allow for filtering on channels (i.e. names) and severity (i.e. levels).
/// @warning **Not** thread safe at the moment.
class DebugDrawer
{
public:
    enum class Level
    {
        trace,
        debug,
        info,
        warn,
        error,
        off,
    };

    static constexpr std::array<Level, 6> gLevels =
    {
        Level::trace,
        Level::debug,
        Level::info,
        Level::warn,
        Level::error,
        Level::off,
    };

    inline static const std::array<std::string, 6> gLevelStrings{
        "trace",
        "debug",
        "info",
        "warn",
        "error",
        "off",
    };

    /// @brief Controls the pose and color of the added debug drawing.
    /// It is used as a parameter to the addXxx() methods in DebugDrawer. 
    struct Entry
    {
        math::Position<3, float> mPosition;
        math::Size<3, float> mScaling{1.f, 1.f, 1.f};
        math::Quaternion<float> mOrientation = math::Quaternion<float>::Identity();
        math::hdr::Rgba_f mColor = math::hdr::gMagenta<float>;
    };

    struct Text
    {
        math::Position<3, float> mPosition;
        std::string mMessage;
        math::hdr::Rgba_f mColor = math::hdr::gWhite<float>;
    };

    struct Commands
    {
        std::vector<Entry> mBoxes;
        std::vector<Entry> mArrows;
        std::vector<Text>  mTexts;
    };

    struct DrawList
    {
        DrawList(std::shared_ptr<Commands> aCommands) :
            mCommands{std::move(aCommands)}
        {}

        // TODO remove
        static DrawList MakeEmpty()
        { return DrawList{}; };

        std::shared_ptr<Commands> mCommands;

    private:
        // TODO remove
        DrawList();
    };


    /// @brief Registry of all DebugDrawers, accessed by associated name.
    class Registry
    {
        friend class DebugDrawer;

        void startFrame();
        DrawList endFrame();

        std::shared_ptr<DebugDrawer> addDrawer(const std::string & aName);

        std::shared_ptr<DebugDrawer> get(const std::string & aName) const;

        void setLevel(Level aLevel);

        static Registry & GetInstance()
        {
            static Registry gInstance;
            return gInstance;
        }

        std::unordered_map<std::string/*logger name*/, std::shared_ptr<DebugDrawer>> mDrawers;
        Level mLevel{Level::trace};
        std::shared_ptr<Commands> mFrameCommands;
    };

    /// @brief Default-constructed instance of this class allow to iterate all drawers in range-based for loops.
    struct IterateDrawers;

public:
    // Should be private, but we want to use it with make_shared 
    DebugDrawer(Registry * aRegistry, Level aLevel) :
        mRegistry{aRegistry},
        mLevel{aLevel}
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

    void SetGlobalLevel(Level aLevel)
    { GetRegistry().setLevel(aLevel); }

    auto & getLevel()
    { return mLevel; }

    /// @brief Add a unit box (from [0, 0, 0] to [1, 1, 1] with pose defined by `aEntry`.
    void addBox(Level aLevel, const Entry & aEntry);

    /// @brief Add a unit the box `aBox` with pose defined by `aEntry`.
    void addBox(Level aLevel, Entry aEntry, const math::Box<float> aBox);

    /// @brief Add unit arrow along positive Y.
    /// @param aEntry 
    void addArrow(Level aLevel, const Entry & aEntry);

    /// @brief Add a frame-of-reference as 3 arrows along positive X(red), Y(green) and Z(blue).
    /// @note The color in `aEntry` will be ignored. 
    void addBasis(Level aLevel, Entry aEntry);

    void addText(Level aLevel, Text aText);

private:
    Commands & commands(Level aLevelSanityCheck);

    bool passDrawFilter(Level aDrawLevel);

    static Registry & GetRegistry()
    { return Registry::GetInstance(); }

    Registry * mRegistry;
    Level mLevel;
};


const std::string & to_string(DebugDrawer::Level aLevel);


struct DebugDrawer::IterateDrawers
{
    auto begin()
    { return DebugDrawer::Begin(); }

    auto end()
    { return DebugDrawer::End(); }
};


} // namespace snac
} // namespace ad
