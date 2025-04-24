#pragma once


#include <array>
#include <limits>
#include <vector>

#include <cassert>


namespace ad {
namespace snac {


#define TMA   class T_value, std::size_t N_universeSize, class T_index
#define TMA_D class T_value, std::size_t N_universeSize, class T_index = std::size_t
#define TMP   T_value, N_universeSize, T_index


// TODO Ad 2024/02/13: Move to Handy library
template <TMA_D>
class SparseSet
{
    // The type used to represent indices must be large enough for the biggest possible index in the universe.
    static_assert(std::numeric_limits<T_index>::max() >= (N_universeSize - 1));

    struct DenseValue
    {
        DenseValue(T_index aIndex, T_value aValue) :
            mBackIndex{aIndex},
            mValue{std::move(aValue)}
        {}

        T_value & get()
        { return mValue; }

        const T_value & get() const
        { return mValue; }

        operator T_value &()
        { return get(); }

        operator const T_value &() const
        { return get(); }

        T_index id() const
        { return mBackIndex; }

        T_index mBackIndex;
        T_value mValue;
    };
    
    using DenseStore = typename std::vector<DenseValue>;
    using Iterator = typename DenseStore::iterator;
    using Const_Iterator = typename DenseStore::const_iterator;

public:
    void insert(T_index aIndex, T_value aValue);

    void clear();

    bool contains(T_index aIndex) const;

    T_value & at(T_index aIndex);
    const T_value & at(T_index aIndex) const;

    T_value & operator[](T_index aIndex);
    const T_value & operator[](T_index aIndex) const;

    T_index size() const
    { assert(mSize == mDense.size()); return mSize; }

    Iterator begin()
    { return mDense.begin(); }
    Iterator end()
    { return mDense.end(); }

    Const_Iterator begin() const
    { return mDense.begin(); }
    Const_Iterator end() const
    { return mDense.end(); }
    Const_Iterator cbegin() const
    { return begin(); }
    Const_Iterator cend() const
    { return end(); }

private:
    // Is this member required, is not it equivalent to mDense.size()? 
    // (Maybe intended to ease moving away to anothe dense store.)
    T_index mSize{0};
    std::array<T_index, N_universeSize> mSparse;
    std::vector<DenseValue> mDense;
};


template <TMA>
void SparseSet<TMP>::insert(T_index aIndex, T_value aValue)
{
    assert(aIndex < N_universeSize);

    mDense.emplace_back(aIndex, std::move(aValue));
    mSparse[aIndex] = mSize; // set sparse[universe_index] to element index in dense storage.
    ++mSize;
}


template <TMA>
void SparseSet<TMP>::clear()
{
    mSize = 0;
    // Note: we could do without this clear, but it simplifies iteration
    mDense.clear();
}


template <TMA>
bool SparseSet<TMP>::contains(T_index aIndex) const
{
    assert(aIndex < N_universeSize);

    const T_index denseIndex = mSparse[aIndex];
    return (denseIndex < mSize) && (mDense[denseIndex].mBackIndex == aIndex);
}


template <TMA>
T_value & SparseSet<TMP>::at(T_index aIndex)
{
    assert(aIndex < N_universeSize);

    return mDense.at(mSparse[aIndex]).get();
}


template <TMA>
const T_value & SparseSet<TMP>::at(T_index aIndex) const
{
    assert(aIndex < N_universeSize);

    return mDense.at(mSparse[aIndex]).get();
}


template <TMA>
T_value & SparseSet<TMP>::operator[](T_index aIndex)
{
    assert(aIndex < N_universeSize);

    return mDense[mSparse[aIndex]].get();
}


template <TMA>
const T_value & SparseSet<TMP>::operator[](T_index aIndex) const
{
    assert(aIndex < N_universeSize);

    return mDense[mSparse[aIndex]].get();
}


#undef TMA
#undef TMA_D
#undef TMP


} // namespace snac
} // namespace ad
