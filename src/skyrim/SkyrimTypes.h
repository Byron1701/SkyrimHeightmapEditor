#pragma once

#include <cstdint>
#include <string>

struct Worldspace
{
    std::uint32_t formId = 0;
    std::string editorId;
};

struct Cell
{
    std::uint32_t formId = 0;
    int gridX = 0;
    int gridY = 0;
};

struct Land
{
    std::uint32_t formId = 0;
    int cellX = 0;
    int cellY = 0;
};
