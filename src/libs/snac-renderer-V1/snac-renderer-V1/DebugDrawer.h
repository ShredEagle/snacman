#pragma once


#include "LoadInterface.h"

#include <handy/Guard.h>

#include <math/Box.h>
#include <math/Color.h>
#include <math/Quaternion.h>
#include <math/Vector.h>

#include <memory>
#include <vector>


#if not defined(DBGDRAW_ACTIVE_LEVEL)
#define DBGDRAW_ACTIVE_LEVEL ::ad::snac::DebugDrawer::Level::trace
#endif


#define DBGDRAW(drawer_name, level)                                             \
    if constexpr(DBGDRAW_ACTIVE_LEVEL != ::ad::snac::DebugDrawer::Level::off)   \
        if(auto drawer = *::ad::snac::DebugDrawer::Get(drawer_name);            \
           drawer.shouldDraw(level))                                            \
            drawer


#define DBGDRAW_TRACE(drawer_name) \
    if constexpr(DBGDRAW_ACTIVE_LEVEL <= ::ad::snac::DebugDrawer::Level::trace) \
        DBGDRAW(drawer_name, ::ad::snac::DebugDrawer::Level::trace)


#define DBGDRAW_DEBUG(drawer_name) \
    if constexpr(DBGDRAW_ACTIVE_LEVEL <= ::ad::snac::DebugDrawer::Level::debug) \
        DBGDRAW(drawer_name, ::ad::snac::DebugDrawer::Level::debug)


#define DBGDRAW_INFO(drawer_name) \
    if constexpr(DBGDRAW_ACTIVE_LEVEL <= ::ad::snac::DebugDrawer::Level::info) \
        DBGDRAW(drawer_name, ::ad::snac::DebugDrawer::Level::info)


#define DBGDRAW_WARN(drawer_name) \
    if constexpr(DBGDRAW_ACTIVE_LEVEL <= ::ad::snac::DebugDrawer::Level::warn) \
        DBGDRAW(drawer_name, ::ad::snac::DebugDrawer::Level::warn)


#define DBGDRAW_ERROR(drawer_name) \
    if constexpr(DBGDRAW_ACTIVE_LEVEL <= ::ad::snac::DebugDrawer::Level::error) \
        DBGDRAW(drawer_name, ::ad::snac::DebugDrawer::Level::error)


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
        math::Position<3, float> mPosition = math::Position<3, float>::Zero();
        math::Size<3, float> mScaling{1.f, 1.f, 1.f};
        math::Quaternion<float> mOrientation = math::Quaternion<float>::Identity();
        math::hdr::Rgba_f mColor = math::hdr::gMagenta<float>;
    };

    struct LineVertex
    {
        math::Position<3, float> mPosition;
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
        std::vector<LineVertex> mLineVertices;
        std::vector<Text>  mTexts;
    };

    struct DrawList
    {
        /// @brief Initialize an empty draw list.
        DrawList();

        DrawList(std::shared_ptr<Commands> aCommands) :
            mCommands{std::move(aCommands)}
        {}

        std::shared_ptr<Commands> mCommands;

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

    static std::shared_ptr<DebugDrawer> AddDrawer(const std::string & aName)
    { return GetRegistry().addDrawer(aName); }

    static void StartFrame()
    { GetRegistry().startFrame(); }

    static DrawList EndFrame()
    { return GetRegistry().endFrame(); }

    static std::shared_ptr<DebugDrawer> Get(const std::string & aName)
    { return GetRegistry().get(aName); }

    static auto Begin()
    { return GetRegistry().mDrawers.begin(); }

    static auto End()
    { return GetRegistry().mDrawers.end(); }

    void SetGlobalLevel(Level aLevel)
    { GetRegistry().setLevel(aLevel); }

    Level & getLevel()
    { return mLevel; }

    Level getLevel() const
    { return mLevel; }

    bool shouldDraw(Level aLevel) const;

    /// @brief Add a unit box (from [0, 0, 0] to [1, 1, 1] with pose defined by `aEntry`.
    void addBox(const Entry & aEntry);

    /// @brief Add a unit the box `aBox` with pose defined by `aEntry`.
    void addBox(Entry aEntry, const math::Box<float> aBox);

    /// @brief Add unit arrow along positive Y.
    /// @param aEntry 
    void addArrow(const Entry & aEntry);

    /// @brief Add a frame-of-reference as 3 arrows along positive X(red), Y(green) and Z(blue).
    /// @note The color in `aEntry` will be ignored. 
    void addBasis(Entry aEntry);

    void addText(Text aText);

    void addLine(const LineVertex & aP1, const LineVertex & aP2);

    void addLine(math::Position<3, float> aP1, math::Position<3, float> aP2,
                 math::hdr::Rgba_f aColor = math::hdr::gMagenta<float>)
    {
        addLine(LineVertex{.mPosition = aP1, .mColor = aColor}, 
                LineVertex{.mPosition = aP2, .mColor = aColor});
    }
        
    void addPlane(math::Position<3, float> aOrigin,
                  math::Vec<3, float> aDir1, math::Vec<3, float> aDir2,
                  int aSubdiv1, int aSubdiv2,
                  float aSize1, float aSize2,
                  math::hdr::Rgba_f aOutlineColor = math::hdr::gMagenta<float>,
                  math::hdr::Rgba_f aSubdivColor = math::hdr::gWhite<float> * 0.4f);

private:
    Commands & commands();

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
