#include "color.h"

ColorTransform::ColorTransform(const float* in_matrix, const int32_t& in_hint, const bool& in_dirty)
    : hint(in_hint) , dirty(in_dirty)
{
    const int32_t dim = 4;
    matrix.resize(dim);
    for (auto& row : matrix)
    {
        row.resize(dim);
    }

    for (int32_t i = 0; i < dim; ++i)
    {
        for (int32_t j = 0; j < dim; ++j)
        {
            matrix[i][j] = in_matrix[i * dim + j];
        }
    }
}
