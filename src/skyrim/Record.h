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

    // GRUP context retained while flattening the record tree.
    // groupType follows the Bethesda GRUP type definitions; groupLabel
    // is the raw 4-byte label from the GRUP header.
    std::int32_t groupType = -1;
    std::uint32_t groupLabel = 0;
    std::uint32_t parentWorldspaceFormId = 0;
    std::uint32_t parentCellFormId = 0;

    std::vector<SubRecord> subRecords;

    // Retains the original compressed record payload.
    std::vector<std::uint8_t> payload;
};
