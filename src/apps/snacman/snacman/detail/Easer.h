#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <math/Constants.h>
#include <math/Vector.h>
#include <vector>

namespace ad {
namespace snac {
namespace detail {

template <class T_number>
T_number cubeRoot(const T_number & a)
{
    return std::copysign(std::pow(std::abs(a), 1 / 3.f), a);
}

template <class T_parameter>
struct Bezier
{
    // One start point with a handle 18 points with two handles and an end
    // point with one handle
    static constexpr size_t sMaxOnCurve = 20;
    static constexpr size_t sMaxStageSize = Bezier::sMaxOnCurve * 3 - 2;

    // NFFNFFNFFNFFN
    // indices for on curve are 0, 3, 6, 9, 12
    std::array<T_parameter, Bezier::sMaxStageSize> mXValues;
    std::array<T_parameter, Bezier::sMaxStageSize> mYValues;
    size_t mOnCurveCount;

    Bezier() :
        mXValues{0.f, 0.f, 1.f, 1.f},
        mYValues{0.f, 0.f, 1.f, 1.f},
        mOnCurveCount{2}
    {}

    T_parameter ease(T_parameter aInput) const
    {
        size_t valueIndex = getValueIndex(aInput) * 3;
        return easeFromIndex(aInput, valueIndex);
    }

    T_parameter easeFromIndex(T_parameter aInput, size_t aValueIndex) const
    {
        T_parameter startOnCurve = mYValues.at(aValueIndex);
        T_parameter startOffCurve = mYValues.at(aValueIndex + 1);
        T_parameter endOffCurve = mYValues.at(aValueIndex + 2);
        T_parameter endOnCurve = mYValues.at(aValueIndex + 3);

        T_parameter startOnCurveX = mXValues.at(aValueIndex);
        T_parameter startOffCurveX = mXValues.at(aValueIndex + 1);
        T_parameter endOffCurveX = mXValues.at(aValueIndex + 2);
        T_parameter endOnCurveX = mXValues.at(aValueIndex + 3);
        T_parameter t =
            getCubicRoots(startOnCurveX - aInput, startOffCurveX - aInput,
                          endOffCurveX - aInput, endOnCurveX - aInput)
                .at(0);

        T_parameter tSqr = t * t;
        T_parameter OneMinusT = 1 - t;
        T_parameter OneMinusTSqr = (1 - t) * (1 - t);

        return startOnCurve * OneMinusTSqr * OneMinusT
               + startOffCurve * 3 * OneMinusTSqr * t
               + endOffCurve * 3 * OneMinusT * tSqr + endOnCurve * t * tSqr;
    }

    size_t getValueIndex(T_parameter aInput) const
    {
        size_t currentIndex = 0;
        while (currentIndex < mOnCurveCount - 1
               && mXValues.at((currentIndex + 1) * 3) < aInput)
        {
            currentIndex++;
        }
        return currentIndex;
    }

    int addPoint(T_parameter aInput)
    {
        if (mOnCurveCount < 20)
        {
            size_t easingIndex = getValueIndex(aInput);
            T_parameter yValue = easeFromIndex(aInput, easingIndex * 3);
            size_t insertIndex = (easingIndex + 1) * 3 - 1;
            mOnCurveCount++;
            for (int i = mOnCurveCount * 3 - 2; i >= insertIndex + 3; i--)
            {
                mXValues.at(i) = mXValues.at(i - 3);
                mYValues.at(i) = mYValues.at(i - 3);
            }
            mXValues.at(insertIndex) = aInput;
            mXValues.at(insertIndex + 1) = aInput;
            mXValues.at(insertIndex + 2) = aInput;
            mYValues.at(insertIndex) = yValue;
            mYValues.at(insertIndex + 1) = yValue;
            mYValues.at(insertIndex + 2) = yValue;

            return insertIndex + 1;
        }

        return -1;
    }

    void removePoint(size_t aIndex)
    {
        if (mOnCurveCount > 2)
        {
            for (int i = aIndex - 1; i < mOnCurveCount * 3 - 2; i++)
            {
                mXValues.at(i) = mXValues.at(i + 3);
                mYValues.at(i) = mYValues.at(i + 3);
            }
            mOnCurveCount--;
        }
    }

    void changePoint(size_t index, math::Position<2, T_parameter> aPosition)
    {
        if (index == 0 || index == (mOnCurveCount * 3) - 2 - 1)
        {
            return;
        }

        switch (index % 3)
        {
        case 0:
        {
            float oldX = mXValues.at(index);
            float oldY = mYValues.at(index);
            if (index > 0)
            {
                mXValues.at(index - 1) =
                    std::clamp((mXValues.at(index - 1) - oldX) + aPosition.x(),
                               T_parameter{0}, T_parameter{1});
                mYValues.at(index - 1) =
                    std::clamp((mYValues.at(index - 1) - oldY) + aPosition.y(),
                               T_parameter{0}, T_parameter{1});
            }

            if (index < mXValues.size() - 1)
            {
                mXValues.at(index + 1) =
                    std::clamp((mXValues.at(index + 1) - oldX) + aPosition.x(),
                               T_parameter{0}, T_parameter{1});
                mYValues.at(index + 1) =
                    std::clamp((mYValues.at(index + 1) - oldY) + aPosition.y(),
                               T_parameter{0}, T_parameter{1});
            }
            break;
        }
        case 1:
        {
            float onX = mXValues.at(index - 1);
            aPosition.x() = std::max(onX, aPosition.x());
            break;
        }
        case 2:
        {
            float onX = mXValues.at(index + 1);
            aPosition.x() = std::min(onX, aPosition.x());
            break;
        }
        }
        mXValues.at(index) =
            std::clamp(aPosition.x(), T_parameter{0}, T_parameter{1});
        mYValues.at(index) =
            std::clamp(aPosition.y(), T_parameter{0}, T_parameter{1});
    }

    std::vector<math::Position<2, float>> getKnots() const
    {
        std::vector<math::Position<2, float>> ret;

        for (int i = 0; i < mOnCurveCount - 1; i++)
        {
            ret.push_back({mXValues.at(i * 3), mYValues.at(i * 3)});
            ret.push_back({mXValues.at(i * 3 + 1), mYValues.at(i * 3 + 1)});
            ret.push_back({mXValues.at(i * 3 + 2), mYValues.at(i * 3 + 2)});
        }

        ret.push_back({mXValues.at((mOnCurveCount - 1) * 3),
                       mYValues.at((mOnCurveCount - 1) * 3)});

        return ret;
    }

    std::array<T_parameter, 3> getCubicRoots(T_parameter pa,
                                             T_parameter pb,
                                             T_parameter pc,
                                             T_parameter pd) const
    {
        T_parameter a = -pa + (3.f * pb) - (3.f * pc) + pd;
        T_parameter b = (3.f * pa) - (6.f * pb) + (3.f * pc);
        T_parameter c = (-3.f * pa) + (3.f * pb);
        T_parameter d = pa;

        if (epsilonCompare(a, 0.f))
        {
            if (epsilonCompare(b, 0.f))
            {
                if (epsilonCompare(c, 0.f))
                {
                    return {};
                }
                return {-c / b};
            }
            T_parameter q = std::sqrt((b * b) - (4.f * a * c));
            T_parameter twoA = 2.f * a;
            return {(-b + q) / twoA, (-b - q) / twoA};
        }

        b /= a;
        c /= a;
        d /= a;

        T_parameter p = ((3.f * c) - (b * b)) / 3.f;
        T_parameter p3 = p / 3.f;
        T_parameter q = ((2.f * b * b * b) - (9.f * b * c) + (27.f * d)) / 27.f;
        T_parameter q2 = q / 2.f;
        T_parameter discrimant = (q2 * q2) + (p3 * p3 * p3);
        T_parameter root;
        T_parameter u;
        T_parameter v;
        int index = 0;
        std::array<T_parameter, 3> result;

        // We need to do everything because sometimes the polynomial
        // has multiple root but only one in 0 -- 1
        if (discrimant < 0)
        {
            T_parameter mp3 = -p3;
            T_parameter mp33 = mp3 * mp3 * mp3;
            T_parameter r = std::sqrt(mp33);
            T_parameter t = -q2 / r;
            T_parameter cosphi = t < -1.f ? -1.f : t > 1.f ? 1. : t;
            T_parameter phi = std::acos(cosphi);
            T_parameter t1 = cubeRoot(r) * 2.f;

            root = (t1 * std::cos(phi / 3.f)) - (b / 3.f);
            if (root >= 0.f && root <= 1.f)
            {
                result.at(index++) = root;
            }
            root = (t1 * std::cos((phi + 2.f * math::pi<float>) / 3.f))
                   - (b / 3.f);
            if (root >= 0.f && root <= 1.f)
            {
                result.at(index++) = root;
            }
            root = (t1 * std::cos((phi + 4.f * math::pi<float>) / 3.f))
                   - (b / 3.f);
            if (root >= 0.f && root <= 1.f)
            {
                result.at(index++) = root;
            }
            return result;
        }

        if (discrimant == 0)
        {
            u = q2 < 0 ? cubeRoot(-q2) : -cubeRoot(q2);
            root = 2.f * u - b / 3.f;
            if (root >= 0.f && root <= 1.f)
            {
                result.at(index++) = root;
            }
            root = -u - b / 3.f;
            if (root >= 0.f && root <= 1.f)
            {
                result.at(index++) = root;
            }
        }

        T_parameter sd = std::sqrt(discrimant);
        u = cubeRoot(-q2 + sd);
        v = cubeRoot(q2 + sd);
        root = u - v - b / 3.f;

        if (root >= 0.f && root <= 1.f)
        {
            result.at(index++) = root;
        }

        return result;
    }
};

template <class T_parameter>
struct CubicSpline
{
    CubicSpline(std::vector<T_parameter> && aXValues,
                std::vector<T_parameter> && aYValues,
                T_parameter aX0Prime = T_parameter{0},
                T_parameter aXNPrime = T_parameter{0}) :
        xValues{std::move(aXValues)},
        yValues{std::move(aYValues)},
        x0Prime{aX0Prime},
        xNPrime{aXNPrime}
    {
        computeSecDerivative();
    }

    CubicSpline(std::vector<math::Position<2, float>> aValues,
                T_parameter aX0Prime = T_parameter{0},
                T_parameter aXNPrime = T_parameter{0}) :
        x0Prime{aX0Prime}, xNPrime{aXNPrime}
    {
        xValues.reserve(aValues.size());
        yValues.reserve(aValues.size());

        for (auto value : aValues)
        {
            xValues.emplace_back(value.x());
            yValues.emplace_back(value.y());
        }
        computeSecDerivative();
    }

    std::vector<math::Position<2, float>> getKnots() const
    {
        std::vector<math::Position<2, float>> ret;

        for (int i = 0; i < xValues.size(); i++)
        {
            ret.push_back({xValues.at(i), yValues.at(i)});
        }

        return ret;
    }

    void computeSecDerivative()
    {
        assert(xValues.size() == yValues.size());
        assert(xValues.size() > 2);

        size_t valLength = xValues.size();
        for (int i = 0; i < valLength - 1; i++)
        {
            assert(xValues.at(i) < xValues.at(i + 1));
        }

        std::vector<T_parameter> cPrime(valLength, 0);
        std::vector<T_parameter> yPrimePrime(valLength, 0);

        T_parameter newX = xValues.at(1);
        T_parameter newY = yValues.at(1);
        T_parameter cj = xValues.at(1) - xValues.at(0);
        T_parameter newDj = (yValues.at(1) - yValues.at(0)) / cj;

        if (x0Prime == T_parameter{0})
        {
            cPrime.at(0) = T_parameter{0};
            yPrimePrime.at(0) = T_parameter{0};
        }
        else
        {
            cPrime.at(0) = T_parameter{0.5};
            yPrimePrime.at(0) = T_parameter{3 * (newDj - x0Prime) / cj};
        }

        for (int i = 1; i < valLength - 1; i++)
        {
            T_parameter oldX = newX;
            T_parameter oldY = newY;

            T_parameter aj = cj;
            T_parameter oldDj = newDj;

            newX = xValues.at(i + 1);
            newY = yValues.at(i + 1);

            cj = newX - oldX;
            newDj = (newY - oldY) / cj;
            T_parameter bj = T_parameter{2} * (cj + aj);
            T_parameter invDenom =
                T_parameter{1} / (bj - (aj * cPrime.at(i - 1)));
            T_parameter dj = T_parameter{6} * (newDj - oldDj);

            yPrimePrime.at(i) = (dj - aj * yPrimePrime.at(i - 1)) * invDenom;
            cPrime.at(i) = cj * invDenom;
        }

        size_t lastElementIndex = valLength - 1;
        if (xNPrime == T_parameter{0})
        {
            cPrime.at(lastElementIndex) = 0;
            yPrimePrime.at(lastElementIndex) = 0;
        }
        else
        {
            T_parameter aj = cj;
            T_parameter oldDj = newDj;

            cj = 0;
            newDj = xNPrime;
            T_parameter bj = T_parameter{2} * (cj + aj);
            T_parameter invDenom =
                T_parameter{1} / (bj - aj * cPrime.at(lastElementIndex - 1));
            T_parameter dj = T_parameter{6} * (newDj - oldDj);

            yPrimePrime.at(lastElementIndex) =
                (dj - aj * yPrimePrime.at(lastElementIndex - 1)) * invDenom;
            cPrime.at(lastElementIndex) = cj * invDenom;
        }

        for (int i = (int) valLength - 2; i >= 0; i--)
        {
            yPrimePrime.at(i) =
                yPrimePrime.at(i) - cPrime.at(i) * yPrimePrime.at(i + 1);
        }

        fppValues = yPrimePrime;
    }

    T_parameter ease(T_parameter aInput) const
    {
        size_t valueIndex = getValueIndex(aInput);
        return interpSpline(
            aInput, xValues.at(valueIndex), xValues.at(valueIndex + 1),
            yValues.at(valueIndex), yValues.at(valueIndex + 1),
            fppValues.at(valueIndex), fppValues.at(valueIndex + 1));
    }

    static constexpr T_parameter oneSixth = T_parameter{1} / T_parameter{6};

    size_t getValueIndex(T_parameter aInput) const
    {
        size_t index = 0;

        while (index + 1 < xValues.size() && xValues.at(index + 1) < aInput)
        {
            index++;
        }

        return index;
    }

    int addPoint(T_parameter aValue) { return -1; }

    void changePoint(size_t index, math::Position<2, T_parameter> aPoint) {}

    std::vector<T_parameter> xValues;
    std::vector<T_parameter> yValues;
    // double derivatives values
    std::vector<T_parameter> fppValues;
    T_parameter x0Prime;
    T_parameter xNPrime;
};
} // namespace detail
} // namespace snac
} // namespace ad
