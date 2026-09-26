#include "terrain/HeightField.h"

#include <algorithm>
#include <stdexcept>

HeightField::HeightField(
    std::size_t width,
    std::size_t height)
    : width_(width),
      height_(height),
      values_(width * height, 0.0f)
{
}

float HeightField::get(
    std::size_t x,
    std::size_t y) const
{
    if (x >= width_ ||
        y >= height_)
    {
        throw std::out_of_range(
            "HeightField coordinate outside bounds.");
    }

    return values_[y * width_ + x];
}

void HeightField::set(
    std::size_t x,
    std::size_t y,
    float value)
{
    if (x >= width_ ||
        y >= height_)
    {
        throw std::out_of_range(
            "HeightField coordinate outside bounds.");
    }

    values_[y * width_ + x] =
        value;
}

void HeightField::clear(
    float value)
{
    std::fill(
        values_.begin(),
        values_.end(),
        value);
}
