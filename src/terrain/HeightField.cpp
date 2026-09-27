#include "terrain/HeightField.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

HeightField::HeightField(
    std::size_t width,
    std::size_t height)
    : width_(width),
      height_(height),
      values_(width * height, 0.0f)
{
}

float& HeightField::at(
    std::size_t x,
    std::size_t y)
{
    if (x >= width_ || y >= height_)
        throw std::out_of_range("HeightField coordinate out of range.");

    return values_[y * width_ + x];
}

float HeightField::at(
    std::size_t x,
    std::size_t y) const
{
    if (x >= width_ || y >= height_)
        throw std::out_of_range("HeightField coordinate out of range.");

    return values_[y * width_ + x];
}

float HeightField::minimum() const
{
    if (values_.empty())
        return 0.0f;

    return *std::min_element(
        values_.begin(),
        values_.end());
}

float HeightField::maximum() const
{
    if (values_.empty())
        return 0.0f;

    return *std::max_element(
        values_.begin(),
        values_.end());
}

void HeightField::clear()
{
    width_ = 0;
    height_ = 0;
    values_.clear();
}
