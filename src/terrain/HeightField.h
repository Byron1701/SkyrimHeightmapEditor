#pragma once

#include <cstddef>
#include <vector>

class HeightField
{
public:
    HeightField() = default;
    HeightField(std::size_t width, std::size_t height);

    std::size_t width() const { return width_; }
    std::size_t height() const { return height_; }

    float& at(std::size_t x, std::size_t y);
    float at(std::size_t x, std::size_t y) const;

    const std::vector<float>& values() const { return values_; }

    float minimum() const;
    float maximum() const;

    void clear();

private:
    std::size_t width_ = 0;
    std::size_t height_ = 0;
    std::vector<float> values_;
};
