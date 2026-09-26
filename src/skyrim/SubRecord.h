#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct SubRecord
{
    std::string type;

    std::size_t offset = 0;
    std::size_t dataOffset = 0;

    std::size_t size = 0;
    std::uint16_t rawSize = 0;

    std::vector<std::uint8_t> data;
};
