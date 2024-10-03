#pragma once


#include <math/Box.h>
#include <math/Vector.h>

#include <span>

#include <cassert>


// TODO Ad 2024/08/30: Move everything to a more generic library, maybe math.
// And importantly: test all that!

namespace ad::renderer {


// TODO this using is too broad
using Triangle = std::array<math::Position<4, float>, 3>;

inline float minForComponent(std::size_t aComponentIdx, const Triangle & aTriangle)
{
    return std::min(
        std::min(
            aTriangle[0][aComponentIdx],
            aTriangle[1][aComponentIdx]),
        aTriangle[2][aComponentIdx]);
}

inline float maxForComponent(std::size_t aComponentIdx, const Triangle & aTriangle)
{
    return std::max(
        std::max(
            aTriangle[0][aComponentIdx],
            aTriangle[1][aComponentIdx]),
        aTriangle[2][aComponentIdx]);
}

// TODO Ad 2024/08/30: Review that.
// In particular, it seems that one is zero and one positive should be considered same side.
// (so, maybe condition is !(sign < 0.), but it requires to treat all zero as fully outside)
/// \brief Returns true if both evaluations place the point on the same side of the plane
///
/// \important Both points are on the same side if:
/// * both their evaluations are non-null and of the same sign 
/// * or both are either negative or zero.
inline bool onSameSide(float aLhs, float aRhs)
{
    float sign = aLhs * aRhs;
    return (sign > 0.) || (sign == 0. && (aLhs + aRhs) <= 0.);
}


// Planes are indexed in the order: l, r, b, t, n, f
/// @return A negative value if the point is inside the plane for the provided volume.
inline float evaluatePlane(std::size_t aPlaneIdx,
                           math::Position<4, float> aPos,
                           math::Box<float> aVolume)
{ 
    assert(aPlaneIdx < 6);
    switch(aPlaneIdx)
    {
        case 0:
            return -aPos.x() + aVolume.borderAt(aPlaneIdx) * aPos.w();
        case 1:
            return  aPos.x() - aVolume.borderAt(aPlaneIdx) * aPos.w();
        case 2:
            return -aPos.y() + aVolume.borderAt(aPlaneIdx) * aPos.w();
        case 3:
            return  aPos.y() - aVolume.borderAt(aPlaneIdx) * aPos.w();
        case 4: // Z min
            return -aPos.z() + aVolume.borderAt(aPlaneIdx) * aPos.w();
        case 5: // Z max
            return  aPos.z() - aVolume.borderAt(aPlaneIdx) * aPos.w();
        default:
            return 0;
    }
}


// TODO Ad 2024/08/30: Could be more generic, taking the border value (and component idx)
/// @return The parameter t at the intersection
inline float solveIntersection(std::size_t aPlaneIdx,
                               math::Position<4, float> a,
                               math::Position<4, float> b,
                               math::Box<float> aClippingVolume)
{
    const float boundary = aClippingVolume.borderAt(aPlaneIdx);
    // The component idx of the axis corresponding to aPlaneIdx.
    const std::size_t idx = aPlaneIdx / 2;

    // (The litterature usually assumes clipping against the unit [-1, 1]^3 box,
    //  which leads to simplified versions of this)
    return 
        (boundary * a.w() - a[idx])
        /
        ((b[idx] - a[idx]) - boundary * (b.w() - a.w()));
}


// Note: Our clipping implementation follows the outline presented in FoCG 3rd 8.1.3 p171
// For each triangle:
//   For each plane: <- this level is the clip() function below
//     For each clipping plane:
//        If all vertices are outside:
//          Discard triangle
//        Else if vertices span the plane:
//           Clip triangle
//           If spawns quad:
//             Break into 2 triangles    
//     Surviving triangle(s) are clipped.

// Note: There is a special (and usual in the litterature) case of clipping against the
// unit volume [-1, 1]^3. For the moment, we implemented the general approach,
// but it might be better to instead transform the triangle so it clips against the unit volume.

// TODO Ad 2024/08/30: I do not like the callback-based interface. 
// Should we simply return a collection of clipped triangles?
/// @brief Clip an homogeneous triangle to a box.
template <class T_ClippedFunctor>
inline void clip(const Triangle & aTriangle,
                 const math::Box<float> & aClippingVolume,
                 T_ClippedFunctor aClippedFunctor,
                 std::size_t aBeginPlane,
                 std::size_t aEndPlane)
{
    using Vertex_t = math::Position<4, float>;

    // Implementation note:
    // Clipping in homogeneous space against planes l, r, b, t, zMin, zMax:
    //   x/w against l (inside when x/w >= l) and r (inside when x/w <= r)
    //   y/w against b (inside when y/w >= b) and t (inside when y/w <= t)
    //   z/w against n (inside when z/w >= zMin) and t (inside when z/w <= zMax) 
    //     * we use zMin, zMax because near and far depend on handedness
    // The equation to solve the for the intersection parameter t is:
    //   component_at_t / w_at_t = plane_boundary (component being either x, y, z)
    // The line between a and b is parameterized as: p(t) = a + t * (b - a)

    for (std::size_t planeIdx = aBeginPlane;
         planeIdx != aEndPlane;
         ++planeIdx)
    {
        float fa = evaluatePlane(planeIdx, aTriangle[0], aClippingVolume);
        float fb = evaluatePlane(planeIdx, aTriangle[1], aClippingVolume);
        float fc = evaluatePlane(planeIdx, aTriangle[2], aClippingVolume);

        // TODO Ad 2024/08/30: It seems all >= 0 is actually outside
        if (fa > 0. && fb > 0. && fc > 0.) // all vertices are outside
        {
            return; // the triangle is entirely clipped
        }
        else if (fa > 0. || fb > 0. || fc > 0) // points are on different sides 
                                               // (at least one is inside from first condition)
                                               // (at least one is outside from this condition)
        {
            Vertex_t a = aTriangle[0];
            Vertex_t b = aTriangle[1];
            Vertex_t c = aTriangle[2];

            // Reorganize the triangle edges to make sure that:
            // * c is on one side
            // * a & b are both on the other side
            // see FoCG 3rd p296
            if (onSameSide(fa, fc)) // b is alone on the other side
            {
                // rotate backward (a=c, b=a, c=b)
                std::swap(fb, fc);
                std::swap(b, c);
                std::swap(fa, fb);
                std::swap(a, b);
            }
            else if (onSameSide(fb, fc)) // a is alone on the otherside
            {
                // rotate forward (a=b, b=c, c=a)
                std::swap(fa, fc);
                std::swap(a, c);
                std::swap(fa, fb);
                std::swap(a, b);
            }

            // NOTE: After swapping, C must be alone on one side of the clipping plane.
            assert(( (void)"Point C must be swapped alone on one side of the plane.",
                     (fc <= 0. && fa > 0. && fb > 0.) || (fc > 0. && fa <= 0. && fb <= 0.) )
            );


            float t_ac = solveIntersection(planeIdx, a, c, aClippingVolume);
            float t_bc = solveIntersection(planeIdx, b, c, aClippingVolume);

            // NOTE: The solution must strictly be on the line segment.
            assert( 0.0 <= t_ac && t_ac <= 1.0 && 0.0 <= t_bc && t_bc <= 1.0 );

            // Define intersection position
            Vertex_t vertexAC{a + t_ac * (c - a)};
            Vertex_t vertexBC{b + t_bc * (c - b)};

            // NOTE: interpolation of varying vertex attributes could take place here.

            // NOTE: evaluation == 0 is also considering the point on the in side.
            // The clipped triangle(s) are now clipped against remaining planes.
            if (fc <= 0) // c is on the in side of the plane, spawn a single triangle.
            {
                clip({vertexAC, vertexBC, c}, aClippingVolume, aClippedFunctor, aBeginPlane + 1, aEndPlane);
                return;
            }
            else // c is outside, spawn two triangles.
            {
                clip({a, b,        vertexAC}, aClippingVolume, aClippedFunctor, aBeginPlane + 1, aEndPlane);
                clip({b, vertexBC, vertexAC}, aClippingVolume, aClippedFunctor, aBeginPlane + 1, aEndPlane);
                return;
            }
        }
    }
    // If there are no more planes to clip against, 
    // the surviving triangle is entirely inside the clipping volume.
    aClippedFunctor(aTriangle);
}


} // namespace ad::renderer