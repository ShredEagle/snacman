#pragma once


#include "ComponentGeometry.h"
#include "ComponentVelocity.h"

#include <entity/EntityManager.h>
#include <entity/Query.h>


namespace ad {
namespace bawls {
namespace system {


/// \brief Handle collision **between balls** only.
class Collide
{
public:
    Collide(ent::EntityManager & aWorld) :
        mMovables{aWorld}
    {}

    void update()
    {
        mMovables.eachPair([](component::Geometry & aGeometryA, component::Velocity & aVelocityA,
                                  component::Geometry & aGeometryB, component::Velocity & aVelocityB)
            
        {
            if(float separation = getSeparation(aGeometryA, aGeometryB); separation <= 0.f)
            {
                float massA = aGeometryA.mass();
                float massB = aGeometryB.mass();
                //
                // Resolve collision
                //
                math::UnitVec<2, GLfloat> normal{aGeometryB.position - aGeometryA.position};

                // Impulse
                float J = - (1 + gRestitutionCoeff)
                    * (aVelocityB - aVelocityA).dot(normal)
                    / (1 / massA + 1 / massB);

                aVelocityA -= normal * (J / massA);
                aVelocityB += normal * (J / massB);

                float penetration = -separation;
                // TODO I suspect the mass should also influence the repositioning
                aGeometryA.position -= normal * (penetration / 2.f);
                aGeometryB.position += normal * (penetration / 2.f);
            }
        });
    }

private:
    static constexpr float gRestitutionCoeff = 1.f;

    static float getSeparation(component::Geometry & aGeometryA, component::Geometry & aGeometryB)
    {
        return (aGeometryA.position - aGeometryB.position).getNorm() 
               - aGeometryA.radius - aGeometryB.radius;
    }

    ent::Query<component::Geometry, component::Velocity> mMovables;
};


} // namespace system
} // namespace bawls
} // namespace ad
