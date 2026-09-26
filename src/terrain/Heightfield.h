#pragma once

#include <cstddef>
#include <vector>

class HeightField
{
public:
    HeightField() = default;

    HeightField(
        std::size_t width,
        std::size_t height);

    std::size_t width() const
    {
        return width_;
    }

    std::size_t height() const
    {
        return height_;
    }

    float get(
        std::size_t x,
        std::size_t y) const;

    void set(
        std::size_t x,
        std::size_t y,
        float value);

    void clear(
        float value = 0.0f);

private:
    std::size_t width_ = 0;
    std::size_t height_ = 0;

    std::vector<float> values_;
};
