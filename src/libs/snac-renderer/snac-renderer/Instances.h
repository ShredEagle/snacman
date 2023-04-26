// A repository of common instance data types
// until we have a better generic solution, easily allowing clients to define custom instance types

#pragma once


#include "Mesh.h"

#include <math/Color.h>
#include <math/Matrix.h>


namespace ad {
namespace snac {


struct PoseColor
{
    math::Matrix<4, 4, float> pose;
    math::sdr::Rgba albedo;
};


template <class>
InstanceStream initializeInstanceStream();


template <>
inline InstanceStream initializeInstanceStream<PoseColor>()
{
    InstanceStream instances;
    {
        graphics::ClientAttribute transformation{
            .mDimension = {4, 4},
            .mOffset = offsetof(PoseColor, pose),
            .mComponentType = GL_FLOAT,
        };
        instances.mAttributes.emplace(Semantic::LocalToWorld,
                                      transformation);
    }
    {
        graphics::ClientAttribute albedo{
            .mDimension = 4,
            .mOffset = offsetof(PoseColor, albedo),
            .mComponentType = GL_UNSIGNED_BYTE,
        };
        instances.mAttributes.emplace(Semantic::Albedo, albedo);
    }
    instances.mInstanceBuffer.mStride = sizeof(PoseColor);
    return instances;
}


} // namespace snac
} // namespace ad



