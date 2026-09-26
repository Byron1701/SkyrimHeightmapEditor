#pragma once

#include "skyrim/SubRecord.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct Record
{
    std::string type;

    std::uint32_t flags = 0;
    std::uint32_t formId = 0;

    std::uint16_t timestamp = 0;
    std::uint16_t vcs1 = 0;
    std::uint16_t version = 0;
    std::uint16_t unknown = 0;

    std::size_t headerOffset = 0;
    std::size_t dataOffset = 0;

    bool compressed = false;

    std::vector<SubRecord> subRecords;

    // Retains the original compressed record payload.
    std::vector<std::uint8_t> payload;
};
