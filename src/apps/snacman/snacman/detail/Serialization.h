#pragma once

#include "math/Interpolation/ParameterAnimation.h"
#include "snacman/simulations/snacgame/component/Context.h"
#include "snacman/simulations/snacgame/component/Explosion.h"
#include "snacman/simulations/snacgame/component/Geometry.h"
#include "snacman/simulations/snacgame/component/GlobalPose.h"
#include "snacman/simulations/snacgame/component/LevelData.h"
#include "snacman/simulations/snacgame/component/MenuItem.h"
#include "snacman/simulations/snacgame/InputCommandConverter.h"

#include "../simulations/snacgame/component/AllowedMovement.h"
#include "../simulations/snacgame/component/Collision.h"
#include "../simulations/snacgame/component/Controller.h"
#include "../SnacArchive.h"

#include <concepts>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <ostream>
#include <type_traits>
#include <utility>

namespace ad {
namespace snacgame {
namespace detail {

#define SERIAL_PARAM(instance, name)                                           \
    {                                                                          \
        #name, instance.name                                                   \
    }
#define SERIAL_FN_PARAM(instance, name)                                        \
    {                                                                          \
        #name, instance.name()                                                 \
    }

template <class T_type>
using serialNvp = std::pair<const char *, T_type>;

template <class T_enum>
    requires std::is_enum_v<T_enum>
std::istream & operator&(std::istream & os, SERIAL_PARAM_DEF(T_enum &) aPair)
{
    auto & [name, aData] = aPair;
    int value;
    os >> value;
    aData = (GamepadAtomicInput) value;
    return os;
}

/// x: {
///   y: 2,
/// }

template <class T_enum>
    requires std::is_enum_v<T_enum>
std::ostream & operator&(std::ostream & os,
                         const SERIAL_PARAM_DEF(const T_enum &) aPair)
{
    const auto & [name, aData] = aPair;
    os << (int) aData;
    return os;
}

template <class T_archive>
concept SnacSerial =
    std::is_same_v<std::remove_cv_t<T_archive>, SnacArchiveOut>
    || std::is_same_v<std::remove_cv_t<T_archive>, SnacArchiveIn>
    || std::is_same_v<std::remove_cv_t<T_archive>, ent::Handle<ent::Entity>>
    || std::is_same_v<std::remove_cv_t<T_archive>, nlohmann::json>;

template <SnacSerial T_archive, class T_data>
T_archive & operator&(T_archive & archive, serialNvp<T_data> aData)
{
    auto & [name, value] = aData;
    archive & value;
}

template <class T_streamable_type>
concept Streamable =
    requires(T_streamable_type & b, std::ostream & a) { a << b; };

template <Streamable T_streamable_type>
SnacArchiveOut & operator&(SnacArchiveOut & archive,
                           const SERIAL_PARAM_DEF(const T_streamable_type &)
                               aPair)
{
    const auto & [name, aData] = aPair;
    archive.mBuffer << aData;
    return archive;
}

template <Streamable T_streamable_type>
SnacArchiveIn & operator&(SnacArchiveIn & archive,
                          SERIAL_PARAM_DEF(T_streamable_type &) aPair)
{
    auto & [name, aData] = aPair;
    archive.mBuffer >> aData;
    return archive;
}

template <SnacSerial T_archive, int N, class T_repr>
T_archive & operator&(T_archive & archive,
                      math::Vec<N, T_repr> & aPair)
{
    auto & [name, aData] = aPair;
    archive & N;
    for (int i = 0; i < N; i++)
    {
        archive & aData.at(i);
    }
    return archive;
}

template <SnacSerial T_archive, class T_repr>
T_archive & operator&(T_archive & archive,
                      math::Box<T_repr> & aPair)
{
    auto & [name, aData] = aPair;
    archive & SERIAL_PARAM(aData, mPosition);
    archive & SERIAL_PARAM(aData, mDimension);
    return archive;
}

template <SnacSerial T_archive, int N, class T_repr>
T_archive & operator&(T_archive & archive,
                      math::Size<N, T_repr> & aPair)
{
    auto & [name, aData] = aPair;
    archive & aData.template as<math::Vec>();
    return archive;
}

template <SnacSerial T_archive, int N, class T_repr>
T_archive &
operator&(T_archive & archive,
          math::Position<N, T_repr> & aPair)
{
    auto & [name, aData] = aPair;
    archive & aData.template as<math::Vec>();
    return archive;
}

template <SnacSerial T_archive, int N, class T_repr>
T_archive & operator&(T_archive & archive,
                      math::Quaternion<T_repr> & aPair)
{
    auto & [name, aData] = aPair;
    archive & SERIAL_FN_PARAM(aData, x);
    archive & SERIAL_FN_PARAM(aData, y);
    archive & SERIAL_FN_PARAM(aData, z);
    archive & SERIAL_FN_PARAM(aData, w);
    return archive;
}

template <SnacSerial T_archive, int N, class T_repr>
T_archive & operator&(T_archive & archive,
                      math::hdr::Rgba<T_repr> & aPair)
{
    auto & [name, aData] = aPair;
    archive & SERIAL_FN_PARAM(aData, r);
    archive & SERIAL_FN_PARAM(aData, g);
    archive & SERIAL_FN_PARAM(aData, b);
    archive & SERIAL_FN_PARAM(aData, a);
    return archive;
}

template <class T_element>
SnacArchiveOut &
operator&(SnacArchiveOut & archive,
          const SERIAL_PARAM_DEF(const std::vector<T_element> &) aPair)
{
    auto & [name, v] = aPair;
    archive & v.size();
    for (auto el : v)
    {
        archive & el;
    }
}

template <class T_element>
SnacArchiveIn & operator&(SnacArchiveIn & archive,
                          SERIAL_PARAM_DEF(std::vector<T_element> &) aPair)
{
    auto & [name, v] = aPair;
    size_t s;
    archive & s;
    for (int i = 0; i < s; i++)
    {
        T_element el;
        archive & el;
        v.push_back(el);
    }
}

template <typename T_map_like>
concept MapLike =
    std::same_as<T_map_like,
                 std::map<typename T_map_like::key_type,
                          typename T_map_like::mapped_type,
                          typename T_map_like::key_compare,
                          typename T_map_like::allocator_type>>
    || std::same_as<T_map_like,
                    std::unordered_map<typename T_map_like::key_type,
                                       typename T_map_like::mapped_type,
                                       typename T_map_like::key_compare,
                                       typename T_map_like::allocator_type>>;

template <MapLike T_map>
SnacArchiveOut & operator&(SnacArchiveOut & archive,
                           const SERIAL_PARAM_DEF(const T_map &) aPair)
{
    auto [name, v] = aPair;
    archive & v.size();
    for (auto [key, value] : v)
    {
        archive & key;
        archive & value;
    }
}

template <MapLike T_map>
SnacArchiveIn & operator&(SnacArchiveIn & archive,
                          SERIAL_PARAM_DEF(T_map &) aPair)
{
    auto [name, v] = aPair;
    size_t s;
    archive & s;
    for (int i = 0; i < s; i++)
    {
        typename T_map::key_type key;
        typename T_map::mapped_type value;
        archive & key & value;
        v.insert(key, value);
    }
}

template <SnacSerial T_archive, class T_repr>
T_archive & operator&(T_archive & archive, KeyMapping<T_repr> & aData)
{
    archive & aData.mKeymaps;
    return archive;
}

template <SnacSerial T_archive>
T_archive & operator&(T_archive & archive, component::AllowedMovement & aData)
{
    archive & SERIAL_PARAM(aData, mAllowedMovement);
    archive & aData.mWindow;
    return archive;
}

template <SnacSerial T_archive>
T_archive & operator&(T_archive & archive, component::Collision & aData)
{
    archive & aData.mHitbox;
    return archive;
}

template <SnacSerial T_archive>
T_archive & operator&(T_archive & archive, component::MappingContext & aData)
{
    archive & aData.mGamepadMapping;
    archive & aData.mKeyboardMapping;
    return archive;
}

template <SnacSerial T_archive>
T_archive & operator&(T_archive & archive, component::Controller & aData)
{
    archive & aData.mType;
    archive & aData.mControllerId;
    archive & aData.mInput;
    return archive;
}

// TODO: (franz) a lot of data structure will need there own definition for the
// serialization
/* template<SnacSerial T_archive, class T_parameter, math::AnimationResult
 * N_resultRange, class TT_periodicity, class TT_easeFunctor> */
/* T_archive & operator&(T_archive & archive,
 * math::ParameterAnimation<T_parameter, N_resultRange, TT_periodicity,
 * TT_easeFunctor> & aData) */
/* { */
/*     archive & aData. */
/* } */
template <SnacSerial T_archive>
T_archive & operator&(T_archive & archive, component::Explosion & aData)
{
    archive & aData.mParameter;
    archive & aData.mStartTime;
    return archive;
}

template <SnacSerial T_archive>
T_archive & operator&(T_archive & archive, component::Geometry & aData)
{
    archive & aData.mPosition;
    archive & aData.mScaling;
    archive & aData.mInstanceScaling;
    archive & aData.mOrientation;
    archive & aData.mColor;

    return archive;
}

template <SnacSerial T_archive>
T_archive & operator&(T_archive & archive, component::GlobalPose & aData)
{
    archive & aData.mPosition;
    archive & aData.mScaling;
    archive & aData.mInstanceScaling;
    archive & aData.mOrientation;
    archive & aData.mColor;

    return archive;
}

template <SnacSerial T_archive>
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

template <SnacSerial T_archive>
T_archive & operator&(T_archive & archive, component::Tile & aData)
{
    archive &(int) aData.mType;
    archive & aData.mAllowedMove;
    archive & aData.mPos;
    return archive;
}

template <SnacSerial T_archive>
T_archive & operator&(T_archive & archive, component::PathfindNode & aData)
{
    archive & aData.mIndex;
    archive & aData.mPos;
    archive & aData.mPathable;
    return archive;
}

template <SnacSerial T_archive>
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

template <SnacSerial T_archive>
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

} // namespace detail
} // namespace snacgame
namespace ent {

using json = nlohmann::json;
ent::Handle<ent::Entity> & operator&(ent::Handle<ent::Entity> & aHandle,
                                     json & aData);

} // namespace ent
} // namespace ad
