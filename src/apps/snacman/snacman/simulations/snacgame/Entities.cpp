#include "Entities.h"

#include "component/AllowedMovement.h"
#include "component/Collision.h"
#include "component/Controller.h"
#include "component/Explosion.h"
#include "component/Geometry.h"
#include "component/GlobalPose.h"
#include "component/MenuItem.h"
#include "component/PlayerGameData.h"
#include "component/PlayerHud.h"
#include "component/PlayerRoundData.h"
#include "component/PlayerSlot.h"
#include "component/Portal.h"
#include "component/PoseScreenSpace.h"
#include "component/PowerUp.h"
#include "component/RigAnimation.h"
#include "component/SceneNode.h"
#include "component/Spawner.h"
#include "component/Speed.h"
#include "component/Tags.h"
#include "component/Text.h"
#include "component/VisualModel.h"
#include "GameContext.h"
#include "GameParameters.h"
#include "SceneGraph.h"
#include "snacman/simulations/snacgame/component/ChangeSize.h"
#include "snacman/simulations/snacgame/scene/MenuScene.h"
#include "ModelInfos.h"
#include "typedef.h"

#include <algorithm>
#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <entity/Query.h>
#include <map>
#include <math/Angle.h>
#include <math/Color.h>
#include <math/Quaternion.h>
#include <math/Vector.h>
#include <optional>
#include <ostream>
#include <snacman/EntityUtilities.h>
#include <snacman/Input.h>
#include <snacman/QueryManipulation.h>
#include <snacman/Resources.h>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ad {
namespace snacgame {

constexpr Size3 lLevelElementScaling = {1.f, 1.f, 1.f};

// TODO phase is not used...
void addGeoNode(GameContext & aContext,
                Entity & aEnt,
                Pos3 aPos,
                float aScale,
                Size3 aInstanceScale,
                Quat_f aOrientation,
                HdrColor_f aColor)
{
    aEnt.add(component::Geometry{.mPosition = aPos,
                                 .mScaling = aScale,
                                 .mInstanceScaling = aInstanceScale,
                                 .mOrientation = aOrientation,
                                 .mColor = aColor})
        .add(component::SceneNode{})
        .add(component::GlobalPose{});
}

std::shared_ptr<snac::Model> addMeshGeoNode(GameContext & aContext,
                                            Entity & aEnt,
                                            const char * aModelPath,
                                            const char * aEffectPath,
                                            Pos3 aPos,
                                            float aScale,
                                            Size3 aInstanceScale,
                                            Quat_f aOrientation,
                                            HdrColor_f aColor)
{
    auto model = aContext.mResources.getModel(aModelPath, aEffectPath);
    aEnt.add(component::Geometry{.mPosition = aPos,
                                 .mScaling = aScale,
                                 .mInstanceScaling = aInstanceScale,
                                 .mOrientation = aOrientation,
                                 .mColor = aColor})
        .add(component::VisualModel{.mModel = model})
        .add(component::SceneNode{})
        .add(component::GlobalPose{});

    return model;
}

namespace {
void createLevelElement(Phase & aPhase,
                        EntHandle aHandle,
                        GameContext & aContext,
                        const math::Position<2, float> & aGridPos,
                        HdrColor_f aColor)
{
    Entity path = *aHandle.get(aPhase);
    addMeshGeoNode(
        aContext, path, "models/square_biscuit/square_biscuit.gltf",
        "effects/MeshTextures.sefx",
        {static_cast<float>(aGridPos.x()), static_cast<float>(aGridPos.y()),
         gLevelHeight},
        0.45f, lLevelElementScaling,
        math::Quaternion<float>{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                                math::Turn<float>{0.25f}},
        aColor);
    path.add(component::LevelTile{});
}
} // namespace

ent::Handle<ent::Entity> createWorldText(GameContext & aContext,
                                         std::string aText,
                                         const component::GlobalPose & aPose)
{
    ent::Phase init;
    auto handle = aContext.mWorld.addEntity("world text");
    Entity text = *handle.get(init);

    auto font =
        aContext.mResources.getFont("fonts/TitanOne-Regular.ttf", 120);

    text.add(component::Text{
                 .mString{std::move(aText)},
                 .mFont = std::move(font),
                 .mColor = math::hdr::gYellow<float>,
             })
        .add(aPose)
        .add(component::GameTransient{});

    return handle;
}

ent::Handle<ent::Entity>
createAnimatedTest(GameContext & aContext,
                   Phase & aPhase,
                   snac::Clock::time_point aStartTime,
                   const math::Position<2, float> & aGridPos)
{
    auto handle = aContext.mWorld.addEntity("animated test");
    Entity entity = *handle.get(aPhase);
    std::shared_ptr<snac::Model> model = addMeshGeoNode(
        aContext, entity, "models/anim/anim.gltf", "effects/MeshRigging.sefx",
        {static_cast<float>(aGridPos.x()), static_cast<float>(aGridPos.y()),
         gLevelHeight},
        0.45f, lLevelElementScaling,
        math::Quaternion<float>{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                                math::Turn<float>{0.25f}});
    const snac::NodeAnimation & animation = model->mAnimations.begin()->second;
    entity.add(component::RigAnimation{
        .mAnimation = &animation,
        .mStartTime = aStartTime,
        .mParameter =
            decltype(component::RigAnimation::mParameter){animation.mEndTime},
    });
    entity.add(component::GameTransient{});

    return handle;
}

ent::Handle<ent::Entity> createPill(GameContext & aContext,
                                    Phase & aPhase,
                                    const math::Position<2, float> & aGridPos)
{
    auto handle = aContext.mWorld.addEntity("pill");
    Entity pill = *handle.get(aPhase);
    addMeshGeoNode(aContext, pill, "models/burger/burger.gltf",
                   "effects/MeshTextures.sefx",
                   {static_cast<float>(aGridPos.x()),
                    static_cast<float>(aGridPos.y()), gPillHeight},
                   0.16f, {1.f, 1.f, 1.f},
                   math::Quaternion<float>{
                       math::UnitVec<3, float>{{1.f, 0.f, 0.f}}, Turn_f{0.1f}});
    pill
        .add(component::Speed{
            .mRotation =
                AxisAngle{.mAxis = math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
                          .mAngle = math::Degree<float>{180.f}},
        })
        .add(component::Pill{})
        .add(component::Collision{component::gPillHitbox})
        .add(component::RoundTransient{});
    return handle;
}

ent::Handle<ent::Entity>
createPowerUp(GameContext & aContext,
              Phase & aPhase,
              const math::Position<2, float> & aGridPos,
              const component::PowerUpType aType,
              float aSwapPeriod)
{
    auto handle = aContext.mWorld.addEntity("power up");
    Entity powerUp = *handle.get(aPhase);
    ModelInfo info =
        gLevelPowerupInfoByType.at(static_cast<unsigned int>(aType));
    addMeshGeoNode(aContext, powerUp, info.mPath, info.mProgPath,
                   {static_cast<float>(aGridPos.x()),
                    static_cast<float>(aGridPos.y()), gPillHeight},
                   info.mScaling, info.mInstanceScale,
                   gLevelBasePowerupQuat * info.mOrientation);
    powerUp
        .add(component::Speed{
            .mRotation =
                AxisAngle{.mAxis = math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
                          .mAngle = math::Degree<float>{180.f}},
        })
        .add(component::PowerUp{
            .mType = aType,
            .mSwapPeriod = aSwapPeriod,
        })
        .add(component::Collision{component::gPowerUpHitbox})
        .add(component::RoundTransient{});
    return handle;
}

EntHandle createPlayerPowerUp(GameContext & aContext,
                              const component::PowerUpType aType)
{
    ent::Phase init;
    auto handle = aContext.mWorld.addEntity("player powerup");
    Entity powerUp = *handle.get(init);
    ModelInfo info =
        gPlayerPowerupInfoByType.at(static_cast<unsigned int>(aType));
    addMeshGeoNode(
        aContext, powerUp, info.mPath, info.mProgPath,
        Pos3{1.f, 1.f, 0.f} + info.mPosOffset, info.mScaling,
        info.mInstanceScale, info.mOrientation);
    powerUp.add(component::RoundTransient{});
    return handle;
}

EntHandle createPathEntity(GameContext & aContext,
                           Phase & aPhase,
                           const math::Position<2, float> & aGridPos)
{
    auto handle = aContext.mWorld.addEntity("path");
    createLevelElement(aPhase, handle, aContext, aGridPos,
                       math::hdr::gWhite<float>);
    return handle;
}

ent::Handle<ent::Entity>
createPortalEntity(GameContext & aContext,
                   Phase & aPhase,
                   const math::Position<2, float> & aGridPos,
                   int aPortalIndex)
{
    auto handle = aContext.mWorld.addEntity("Portal");
    createLevelElement(aPhase, handle, aContext, aGridPos,
                       math::hdr::gRed<float>);
    Entity portal = *handle.get(aPhase);
    portal.add(component::Portal{aPortalIndex});
    return handle;
}

void addPortalInfo(GameContext & aContext,
                   EntHandle aLevel,
                   component::Portal & aPortal,
                   const component::Geometry & aGeo,
                   Vec3 aDirection)
{
    Box_f portalEntrance{component::gPortalHitbox};
    portalEntrance.mPosition += aDirection;
    Box_f portalExit{component::gPortalHitbox};
    portalExit.mPosition -= aDirection * 0.8f;

    Phase addHitboxPhase;
    aPortal.mEnterHitbox = portalEntrance;
    aPortal.mExitHitbox = portalExit;
    aPortal.mMirrorSpawnPosition = aGeo.mPosition + aDirection;

    auto model = aContext.mWorld.addEntity("portal model");
    {
        Phase portalModelPhase;
        Entity modelEnt = *model.get(portalModelPhase);
        addMeshGeoNode(
            aContext, modelEnt, "models/portal/portal.gltf",
            "effects/MeshTextures.sefx",
            {aGeo.mPosition.x() + aDirection.x() * 1.3f, aGeo.mPosition.y(),
             0.f},
            0.2f, {1.f, 1.f, 1.f},
            Quat_f{UnitVec3::MakeFromUnitLength({0.f, 0.f, 1.f}),
                   Turn_f{(-aDirection.x() - 1.f) / 2.f * 0.5f}}
                * Quat_f{UnitVec3::MakeFromUnitLength({1.f, 0.f, 0.f}),
                         Turn_f{0.25f}});
        modelEnt.add(component::RoundTransient{});
    }
    insertEntityInScene(model, aLevel);
}

ent::Handle<ent::Entity>
createCopPenEntity(GameContext & aContext,
                   Phase & aPhase,
                   const math::Position<2, float> & aGridPos)
{
    auto handle = aContext.mWorld.addEntity("cop pen");
    createLevelElement(aPhase, handle, aContext, aGridPos,
                       math::hdr::gYellow<float>);
    return handle;
}

ent::Handle<ent::Entity>
createPlayerSpawnEntity(GameContext & aContext,
                        Phase & aPhase,
                        const math::Position<2, float> & aGridPos)
{
    math::Position<3, float> spawnPos = {static_cast<float>(aGridPos.x()),
                                         static_cast<float>(aGridPos.y()),
                                         gPlayerHeight};
    auto spawner = aContext.mWorld.addEntity("spawner");
    createLevelElement(aPhase, spawner, aContext, aGridPos,
                       math::hdr::gCyan<float>);
    spawner.get(aPhase)->add(component::Spawner{.mSpawnPosition = spawnPos});

    return spawner;
}

ent::Handle<ent::Entity> createHudBillpad(GameContext & aContext,
                                          const int aPlayerIndex)
{
    EntHandle hudHandle = aContext.mWorld.addEntity("hud");
    {
        Phase createHud;

        ent::Entity hud = *hudHandle.get(createHud);
        // Create the Hud common ancestor in the scene graph
        addGeoNode(
            aContext, hud,
            // TODO bake the -7 -7 into the hud positions.
            component::gHudPositionsWorld[aPlayerIndex] + Vec3{-7.f, -7.f, 0.f},
            1.f, {1.f, 1.f, 1.f},
            component::gHudOrientationsWorld[aPlayerIndex]);
    }

    // The score text line
    EntHandle scoreHandle = aContext.mWorld.addEntity("score");
    EntHandle roundHandle = aContext.mWorld.addEntity("round");
    EntHandle powerupHandle = aContext.mWorld.addEntity("powerup hud");
    EntHandle billpadHandle = aContext.mWorld.addEntity("bill pad");
    {
        Phase createScore;

        const std::string fontname = "fonts/notes/Bitcheese.ttf";

        // Score
        {
            ent::Entity scoreText = *scoreHandle.get(createScore);
            scoreText.add(component::Text{
                .mString = "0",
                .mFont = aContext.mResources.getFont(fontname, 100),
                //.mColor = playerSlot.mColor,
                .mColor = math::hdr::gBlack<float>,
            });

            addGeoNode(aContext, scoreText, {-1.7f, 0.6f, 0.f}, 1.f);
        }

        // Rounds
        {
            ent::Entity roundText = *roundHandle.get(createScore);
            roundText.add(component::Text{
                .mString = "0",
                .mFont = aContext.mResources.getFont(fontname, 100),
                //.mColor = playerSlot.mColor,
                .mColor = math::hdr::gBlack<float>,
            });

            addGeoNode(aContext, roundText, {0.5f, 0.3f, 0.f}, 1.1f);
        }

        // Power-up
        {
            ent::Entity powerupText = *powerupHandle.get(createScore);
            powerupText.add(component::Text{
                .mFont = aContext.mResources.getFont(fontname, 100),
                //.mColor = playerSlot.mColor,
                .mColor = math::hdr::gBlack<float>,
            });
            addGeoNode(aContext, powerupText, {-1.9f, -.7f, 0.f}, 0.5f);
        }

        // Billpad model
        {
            ent::Entity billpad = *billpadHandle.get(createScore);
            addMeshGeoNode(
                aContext, billpad, "models/billpad/billpad.gltf",
                "effects/MeshTextures.sefx", {0.f, 0.f, -0.30f}, 8.f,
                {1.f, 1.f, 1.f},
                Quat_f{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                       math::Turn<float>{0.25f}}
                    * Quat_f{math::UnitVec<3, float>{{0.f, 1.f, 0.f}},
                             math::Turn<float>{-0.25f}},
                gSlotColors.at(aPlayerIndex));
        }
    }

    {
        Phase completeSceneGraph;

        insertEntityInScene(scoreHandle, hudHandle);
        insertEntityInScene(roundHandle, hudHandle);
        insertEntityInScene(powerupHandle, hudHandle);
        insertEntityInScene(billpadHandle, hudHandle);

        assert(aContext.mSceneRoot.isValid());
        EntHandle root = aContext.mSceneRoot;
        // Insert the billpad into the root, not the level,
        // because the level scale changes depending on the tile dimensions.
        insertEntityInScene(hudHandle, root);

        hudHandle.get(completeSceneGraph)
            ->add(component::PlayerHud{
                .mScoreText = scoreHandle,
                .mRoundText = roundHandle,
                .mPowerupText = powerupHandle,
            });
    }

    return hudHandle;
}

// TODO: (franz) does not need the slot handle just the index
EntHandle createPlayerModel(GameContext & aContext, EntHandle aSlotHandle)
{
    const component::PlayerSlot & slot =
        aSlotHandle.get()->get<component::PlayerSlot>();
    EntHandle playerModelHandle = aContext.mWorld.addEntity("player model");
    {
        Phase createModel;
        Entity model = *playerModelHandle.get(createModel);

        std::shared_ptr<snac::Model> modelData = addMeshGeoNode(
            aContext, model, "models/donut/donut.gltf",
            "effects/MeshRiggingTextures.sefx", Pos3::Zero(), 1.f,
            gBasePlayerModelInstanceScaling, gBasePlayerModelOrientation,
            gSlotColors.at(slot.mSlotIndex));

        std::string animName = "idle";
        const snac::NodeAnimation & animation =
            modelData->mAnimations.at(animName);
        model.add(component::RigAnimation{
            .mAnimName = animName,
            .mAnimation = &animation,
            .mAnimationMap = &modelData->mAnimations,
            .mStartTime = snac::Clock::now(),
            .mParameter =
                decltype(component::RigAnimation::mParameter){
                    animation.mEndTime},
        });
        model.add(component::ChangeSize{
            .mCurve = ParameterAnimation<float, math::FullRange,
                                         math::None,
                                         math::ease::Bezier>(
                math::ease::Bezier<float>())});
    }

    return playerModelHandle;
}

ent::Handle<ent::Entity> createCrown(GameContext & aContext)
{
    Phase createCrown;
    EntHandle crownHandle = aContext.mWorld.addEntity("crown");
    Entity crown = *crownHandle.get(createCrown);

    std::shared_ptr<snac::Model> crownData = addMeshGeoNode(
        aContext, crown, "models/crown/crown.gltf",
        "effects/MeshTextures.sefx", gBaseCrownPosition, 1.f,
        gBaseCrownInstanceScaling, gBaseCrownOrientation);
    crownHandle.get(createCrown)->add(component::Crown{});

    return crownHandle;
}


// TODO: (franz) does not need the slot handle just the index
EntHandle createJoinGamePlayer(GameContext & aContext, EntHandle aSlotHandle, int aSlotIndex)
{
    EntHandle playerModelHandle = createPlayerModel(aContext, aSlotHandle);
    EntHandle numberHandle = aContext.mWorld.addEntity("player number");

    component::Geometry & aGeo =
        snac::getComponent<component::Geometry>(playerModelHandle);
    aGeo.mOrientation = Quat_f{math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
                               math::Turn<float>{-0.25f}}
                        * Quat_f{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                                 math::Turn<float>{0.25f}};
    aGeo.mPosition = Pos3{
        -4.5f + 3.f * (float)aSlotIndex,
        -5.f,
        0.f
    };

    {
        Phase createNumber;
        Entity number = *numberHandle.get(createNumber);
        ModelInfo info =
            gSlotNumbers.at(aSlotIndex);
        addMeshGeoNode(aContext, number, info.mPath, info.mProgPath,
                       info.mPosOffset.as<math::Position>(),
                       info.mScaling, info.mInstanceScale,
                       info.mOrientation, gSlotColors.at(aSlotIndex));
    }

    insertEntityInScene(numberHandle, playerModelHandle);

    return playerModelHandle;
}

void preparePlayerForGame(GameContext & aContext, EntHandle aSlotHandle)
{
    component::SceneNode & node = snac::getComponent<component::SceneNode>(aSlotHandle);
    
    while(node.mFirstChild.isValid())
    {
        Phase destroyChild;
        eraseEntityRecursive(node.mFirstChild, destroyChild);
    }
}

EntHandle createInGamePlayer(GameContext & aContext,
                             EntHandle aSlotHandle,
                             const Pos3 & aPosition)
{
    EntHandle playerModelHandle = createPlayerModel(aContext, aSlotHandle);
    EntHandle playerHandle = aContext.mWorld.addEntity("in game player");

    {
        Phase sceneInit;
        Entity player = *playerHandle.get(sceneInit);
        addGeoNode(aContext, player, aPosition);
        component::Controller controller = snac::getComponent<component::Controller>(aSlotHandle);
        player
            .add(component::PlayerRoundData{.mModel = playerModelHandle, .mSlot = aSlotHandle})
            .add(controller)
            .add(component::AllowedMovement{})
            .add(component::Collision{component::gPlayerHitbox});
    }

    insertEntityInScene(playerModelHandle, playerHandle);

    return playerHandle;
}

EntHandle addPlayer(GameContext & aContext, const int aControllerIndex)
{
    EntHandle playerSlot = aContext.mWorld.addEntity("player slot");
    if (aContext.mSlotManager.addPlayer(aContext, playerSlot, aControllerIndex))
    {
        return playerSlot;
    }
    else
    {
        Phase destroy;
        playerSlot.get(destroy)->erase();
        return EntHandle{};
    }
}

void addBillpadHud(GameContext & aContext, EntHandle aSlotHandle)
{
    const component::PlayerSlot & slot =
        aSlotHandle.get()->get<component::PlayerSlot>();
    component::PlayerGameData & gameData =
        aSlotHandle.get()->get<component::PlayerGameData>();

    gameData.mHud = createHudBillpad(aContext, slot.mSlotIndex);
}

ent::Handle<ent::Entity>
createMenuItem(GameContext & aContext,
               ent::Phase & aInit,
               const std::string & aString,
               std::shared_ptr<snac::Font> aFont,
               const math::Position<2, float> & aPos,
               const std::unordered_map<int, std::string> & aNeighbors,
               const scene::Transition & aTransition,
               bool aSelected,
               const math::Size<2, float> & aScale)
{
    auto item = makeText(
        aContext, aInit, aString, aFont,
        aSelected ? gColorItemSelected : gColorItemUnselected,
        aPos, aScale);
    item.get(aInit)->add(component::MenuItem{.mName = aString,
                                             .mSelected = aSelected,
                                             .mNeighbors = aNeighbors,
                                             .mTransition = aTransition});

    return item;
}

ent::Handle<ent::Entity>
makeText(GameContext & aContext,
         ent::Phase & aPhase,
         const std::string & aString,
         std::shared_ptr<snac::Font> aFont,
         const math::hdr::Rgba_f & aColor,
         const math::Position<2, float> & aPosition_unitscreen,
         const math::Size<2, float> & aScale)
{
    auto handle = aContext.mWorld.addEntity("text");

    handle.get(aPhase)
        ->add(component::Text{
            .mString{aString},
            .mFont = std::move(aFont),
            .mColor = aColor,
        })
        .add(component::PoseScreenSpace{.mPosition_u = aPosition_unitscreen,
                                        .mScale = aScale});

    return handle;
}

void swapPlayerPosition(Phase & aPhase, EntHandle aPlayer, EntHandle aOther)
{
    component::Geometry & aGeo =
        aPlayer.get(aPhase)->get<component::Geometry>();
    component::Geometry & aOtherGeo =
        aOther.get(aPhase)->get<component::Geometry>();
    Pos3 temp = aGeo.mPosition;
    aGeo.mPosition = aOtherGeo.mPosition;
    aOtherGeo.mPosition = temp;

    // Remove portal image if there is one
}

EntHandle createStageDecor(GameContext & aContext)
{
    EntHandle result = aContext.mWorld.addEntity("stage decor");
    {
        Phase createStage;
        Entity stageEntity = *result.get(createStage);

        addMeshGeoNode(aContext, stageEntity, "models/stage/stage.gltf",
                       "effects/MeshTextures.sefx", {0.f, 0.f, -0.4f}, 1.0f,
                       {1.1f, 1.1f, 1.f},
                       Quat_f{math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
                              math::Turn<float>{0.25f}}
                           * Quat_f{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                                    math::Turn<float>{0.25f}});
    }
    return result;
}

EntHandle createTargetArrow(GameContext & aContext, const HdrColor_f & aColor)
{
    EntHandle result = aContext.mWorld.addEntity("target arrow");
    {
        Phase createTargetArrow;
        Entity targetArrow = *result.get(createTargetArrow);

        addMeshGeoNode(aContext, targetArrow, "models/arrow/arrow.gltf",
                       "effects/MeshTextures.sefx", {0.f, 0.f, 2.f}, 0.4f,
                       {1.f, 1.f, 1.f},
                       Quat_f{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                              math::Turn<float>{0.25f}},
                       aColor);

        targetArrow
            .add(component::Speed{
                .mRotation =
                    AxisAngle{.mAxis = math::UnitVec<3, float>{{0.f, 0.f, 1.f}},
                              .mAngle = math::Degree<float>{180.f}}})
            .add(component::RoundTransient{});
    }
    return result;
}

EntHandle createExplosion(GameContext & aContext,
                          const math::Position<3, float> & aPos,
                          const snac::Time & aTime)
{
    EntHandle explosion = aContext.mWorld.addEntity("explosion");
    {
        Phase createExplosion;
        Entity explosionEnt = *explosion.get(createExplosion);
        addMeshGeoNode(aContext, explosionEnt, "models/boom/boom.gltf",
                       "effects/MeshTextures.sefx", aPos, 0.1f);
        explosionEnt
            .add(component::Explosion{
                .mStartTime = aTime.mTimepoint,
                .mParameter = math::ParameterAnimation<
                    float, math::AnimationResult::Clamp>(0.5f)})
            .add(component::RoundTransient{});
    }
    return explosion;
}

EntHandle createPortalImage(GameContext & aContext,
                            component::PlayerRoundData & aPlayerData,
                            const component::Portal & aPortal)
{
    EntHandle newPortalImage = aContext.mWorld.addEntity("portal image");
    {
        Phase addPortalImage;
        component::Geometry modelGeo =
            aPlayerData.mModel.get()->get<component::Geometry>();
        component::RigAnimation animation =
            aPlayerData.mModel.get()->get<component::RigAnimation>();
        Entity portal = *newPortalImage.get(addPortalImage);
        Vec2 relativePos =
            aPortal.mMirrorSpawnPosition.xy() - aPlayerData.mCurrentPortalPos;
        addMeshGeoNode(aContext, portal, "models/donut/donut.gltf",
                       "effects/MeshTextures.sefx",
                       {relativePos.x(), relativePos.y(), 0.f},
                       modelGeo.mScaling, modelGeo.mInstanceScaling,
                       modelGeo.mOrientation, modelGeo.mColor);
        portal.add(component::Collision{component::gPlayerHitbox})
            .add(component::RigAnimation{
                .mAnimName = animation.mAnimName,
                .mAnimation = animation.mAnimation,
                .mStartTime = animation.mStartTime,
                .mParameter =
                    decltype(component::RigAnimation::mParameter){
                        animation.mAnimation->mEndTime},
            });
        aPlayerData.mPortalImage = newPortalImage;
    }

    return newPortalImage;
}

} // namespace snacgame
} // namespace ad
