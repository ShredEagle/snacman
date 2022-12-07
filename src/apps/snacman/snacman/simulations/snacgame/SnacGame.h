#pragma once


#include "ComponentGeometry.h"
#include "EntityWrap.h"
#include "Renderer.h"
#include "SystemOrbitalCamera.h"

#include "../../Input.h"

#include <entity/EntityManager.h>

#include <graphics/AppInterface.h>

#include <memory>


namespace ad {
namespace snacgame {


class SnacGame
{
public:
    using Renderer_t = Renderer;

    /// \brief Initialize the scene;
    SnacGame(graphics::AppInterface & aAppInterface);

    void update(float aDelta, const snac::Input & aInput);

    std::unique_ptr<visu::GraphicState> makeGraphicState() const; 

    snac::Camera::Parameters getCameraParameters() const;

    Renderer_t makeRenderer() const;

private:
    void blinkCube(ent::Phase & aPhase);

    graphics::AppInterface * mAppInterface;
    snac::Camera::Parameters mCameraParameters = snac::Camera::gDefaults;

    ent::EntityManager mWorld;
    ent::Handle<ent::Entity> mSystemMove;
    EntityWrap<system::OrbitalCamera> mSystemOrbitalCamera;
    EntityWrap<ent::Query<component::Geometry>> mQueryRenderable;

    ent::Handle<ent::Entity> mBlinkingCube;

    // A float would run out of precision too quickly.
    double mSimulationTime{0.};
};


} // namespace cubes
} // namespace ad
