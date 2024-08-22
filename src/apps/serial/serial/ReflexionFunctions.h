#pragma once

#include "math/MatrixBase.h"

namespace serial {
template <class T_derived,
          int N_rows,
          int N_cols,
          class T_number,
          class T_witness>
void describeTo(T_witness & w,
                ad::math::MatrixBase<T_derived, N_rows, N_cols, T_number> & v)
{
    w & v.mStore;
};
}
