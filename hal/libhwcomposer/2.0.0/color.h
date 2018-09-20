#ifndef HWC_COLOR_H_
#define HWC_COLOR_H_

// In this file, including any file under libhwcompsoer folder is prohibited!!!
#include <vector>
#include <utils/RefBase.h>
#include <utils/LightRefBase.h>

struct ColorTransform : public android::LightRefBase<ColorTransform>
{
    ColorTransform(const float* in_matrix, const int32_t& in_hint, const bool& in_dirty);

    std::vector<std::vector<float> > matrix;

    int32_t hint;

    bool dirty;
};

#endif // HWC_COLOR_H_
