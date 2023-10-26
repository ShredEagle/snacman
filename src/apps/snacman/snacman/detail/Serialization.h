#include "math/Interpolation/ParameterAnimation.h"
#include "snacman/simulations/snacgame/InputCommandConverter.h"
#include "snacman/simulations/snacgame/component/Context.h"
#include "snacman/simulations/snacgame/component/Explosion.h"
#include "snacman/simulations/snacgame/component/Geometry.h"
#include "snacman/simulations/snacgame/component/GlobalPose.h"
#include "snacman/simulations/snacgame/component/LevelData.h"
#include "snacman/simulations/snacgame/component/MenuItem.h"
#include "../SnacArchive.h"
#include "../simulations/snacgame/component/AllowedMovement.h"
#include "../simulations/snacgame/component/Collision.h"
#include "../simulations/snacgame/component/Controller.h"
#include <concepts>
#include <ostream>
#include <type_traits>

namespace ad {
namespace snacgame {
namespace detail {

template<class T_enum>
    requires std::is_enum_v<T_enum>
std::istream & operator&(std::istream & os, T_enum & aData)
{
    int value;
    os >> value;
    aData = (GamepadAtomicInput)value;
    return os;
}

template<class T_enum>
    requires std::is_enum_v<T_enum>
std::ostream & operator&(std::ostream & os, const T_enum & aData)
{
    os << (int)aData;
    return os;
}

template<class T_archive>
    concept SnacArchive = std::is_same_v<T_archive, SnacArchiveOut> || std::is_same_v<T_archive, SnacArchiveIn>;

template<class T_streamable_type>
    concept Streamable = requires(T_streamable_type & b, std::ostream & a) { a << b; };

template<Streamable T_streamable_type>
SnacArchiveOut & operator&(SnacArchiveOut & archive, const T_streamable_type & aData)
{
    archive.mBuffer << aData;
    return archive;
}

template<Streamable T_streamable_type>
SnacArchiveIn & operator&(SnacArchiveIn & archive, T_streamable_type & aData)
{
    archive.mBuffer >> aData;
    return archive;
}

template<SnacArchive T_archive, int N, class T_repr>
T_archive & operator&(T_archive & archive, math::Vec<N, T_repr> & aData)
{
    archive & N;
    for (int i = 0; i < N; i++)
    {
        archive & aData.at(i);
    }
    return archive;
}

template<SnacArchive T_archive, class T_repr>
T_archive & operator&(T_archive & archive, math::Box<T_repr> & aData)
{
    archive & aData.mPosition;
    archive & aData.mDimension;
    return archive;
}

template<SnacArchive T_archive, int N, class T_repr>
T_archive & operator&(T_archive & archive, math::Size<N, T_repr> & aData)
{
    archive & aData.template as<math::Vec>();
    return archive;
}

template<SnacArchive T_archive, int N, class T_repr>
T_archive & operator&(T_archive & archive, math::Position<N, T_repr> & aData)
{
    archive & aData.template as<math::Vec>();
    return archive;
}

template<SnacArchive T_archive, int N, class T_repr>
T_archive & operator&(T_archive & archive, math::Quaternion<T_repr> & aData)
{
    archive & aData.x();
    archive & aData.y();
    archive & aData.z();
    archive & aData.w();
    return archive;
}

template<SnacArchive T_archive, int N, class T_repr>
T_archive & operator&(T_archive & archive, math::hdr::Rgba<T_repr> & aData)
{
    archive & aData.r();
    archive & aData.g();
    archive & aData.b();
    archive & aData.a();
    return archive;
}

template<class T_element>
SnacArchiveOut & operator&(SnacArchiveOut & archive, const std::vector<T_element> & v)
{
    archive & v.size();
    for (auto el : v)
    {
        archive & el;
    }
}

template<class T_element>
SnacArchiveIn & operator&(SnacArchiveIn & archive, const std::vector<T_element> & v)
{
    size_t s;
    archive & s;
    for (int i = 0; i < s; i++)
    {
        T_element el;
        archive & el;
        v.push_back(el);
    }
}

template<class T_key, class T_element>
SnacArchiveOut & operator&(SnacArchiveOut & archive, const std::unordered_map<T_key, T_element> & v)
{
    archive & v.size();
    for (auto [key, value] : v)
    {
        archive & key;
        archive & value;
    }
}

template<class T_key, class T_element>
SnacArchiveIn & operator&(SnacArchiveIn & archive, const std::unordered_map<T_key, T_element> & v)
{
    size_t s;
    archive & s;
    for (int i = 0; i < s; i++)
    {
        T_key key;
        T_element value;
        archive & key & value;
        v.insert(key, value);
    }
}

template<class T_key, class T_element>
SnacArchiveOut & operator&(SnacArchiveOut & archive, const std::map<T_key, T_element> & v)
{
    archive & v.size();
    for (auto [key, value] : v)
    {
        archive & key;
        archive & value;
    }
}

template<class T_key, class T_element>
SnacArchiveIn & operator&(SnacArchiveIn & archive, const std::map<T_key, T_element> & v)
{
    size_t s;
    archive & s;
    for (int i = 0; i < s; i++)
    {
        T_key key;
        T_element value;
        archive & key & value;
        v.insert(key, value);
    }
}

template<SnacArchive T_archive, class T_repr>
T_archive & operator&(T_archive & archive, KeyMapping<T_repr> & aData)
{
    archive & aData.mKeymaps;
    return archive;
}

template<SnacArchive T_archive>
T_archive & operator&(T_archive & archive, component::AllowedMovement & aData)
{
    archive & aData.mAllowedMovement;
    archive & aData.mWindow;
    return archive;
}

template<SnacArchive T_archive>
T_archive & operator&(T_archive & archive, component::Collision & aData)
{
    archive & aData.mHitbox;
    return archive;
}

template<SnacArchive T_archive>
T_archive & operator&(T_archive & archive, component::MappingContext & aData)
{
    archive & aData.mGamepadMapping;
    archive & aData.mKeyboardMapping;
    return archive;
}


template<SnacArchive T_archive>
T_archive & operator&(T_archive & archive, component::Controller & aData)
{
    archive & aData.mType;
    archive & aData.mControllerId;
    archive & aData.mInput;
    return archive;
}

// TODO: (franz) a lot of data structure will need there own definition for the
// serialization
/* template<SnacArchive T_archive, class T_parameter, math::AnimationResult N_resultRange, class TT_periodicity, class TT_easeFunctor> */
/* T_archive & operator&(T_archive & archive, math::ParameterAnimation<T_parameter, N_resultRange, TT_periodicity, TT_easeFunctor> & aData) */
/* { */
/*     archive & aData. */
/* } */
template<SnacArchive T_archive>
T_archive & operator&(T_archive & archive, component::Explosion & aData)
{
    archive & aData.mParameter;
    archive & aData.mStartTime;
    return archive;
}

template<SnacArchive T_archive>
T_archive & operator&(T_archive & archive, component::Geometry & aData)
{
    archive & aData.mPosition;
    archive & aData.mScaling;
    archive & aData.mInstanceScaling;
    archive & aData.mOrientation;
    archive & aData.mColor;

    return archive;
}

template<SnacArchive T_archive>
T_archive & operator&(T_archive & archive, component::GlobalPose & aData)
{
    archive & aData.mPosition;
    archive & aData.mScaling;
    archive & aData.mInstanceScaling;
    archive & aData.mOrientation;
    archive & aData.mColor;

    return archive;
}

template<SnacArchive T_archive>
T_archive & operator&(T_archive & archive, component::LevelSetupData & aData)
{
    archive & aData.mAssetRoot;
    archive & aData.mFile;
    archive & aData.mAvailableSizes.size();
    for (auto size : aData.mAvailableSizes)
    {
        archive & size;
    }
    archive & aData.mSeed;

    return archive;
}

template<SnacArchive T_archive>
T_archive & operator&(T_archive & archive, component::Tile & aData)
{
    archive & (int)aData.mType;
    archive & aData.mAllowedMove;
    archive & aData.mPos;
    return archive;
}

template<SnacArchive T_archive>
T_archive & operator&(T_archive & archive, component::PathfindNode & aData)
{
    archive & aData.mIndex;
    archive & aData.mPos;
    archive & aData.mPathable;
    return archive;
}

template<SnacArchive T_archive>
T_archive & operator&(T_archive & archive, component::Level & aData)
{
    archive & aData.mSize;
    archive & aData.mTiles.size();
    for (auto tile : aData.mTiles)
    {
        archive & tile;
    }

    archive & aData.mNodes.size();
    for (auto node : aData.mNodes)
    {
        archive & node;
    }

    archive & aData.mPortalIndex.size();
    for (auto portalIndex : aData.mPortalIndex)
    {
        archive & portalIndex;
    }

    return archive;
}

template<SnacArchive T_archive>
T_archive & operator&(T_archive & archive, component::MenuItem & aData)
{
    archive & aData.mName;
    archive & aData.mSelected;
    archive & aData.mNeighbors.size();
    for (auto [key, value] : aData.mNeighbors)
    {
    }
    return archive;
}

}
}}
