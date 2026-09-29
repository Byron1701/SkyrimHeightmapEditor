#pragma once

#include "skyrim/SubRecord.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

struct Record
{
    std::string type;

    std::uint32_t flags = 0;
    std::uint32_t formId = 0;

    // Skyrim record header fields at file offsets +0x10..+0x17.
    std::uint16_t timestamp = 0;
    std::uint16_t vcs1 = 0;
    std::uint16_t version = 0;
    std::uint16_t unknown = 0;

    // Absolute file offsets. These are byte offsets from the beginning
    // of the .esp/.esm/.esl file, not offsets within a GRUP.
    std::size_t headerOffset = 0;
    std::size_t dataOffset = 0;
    std::size_t endOffset = 0;

    // The value from the record header's uint32 data-size field.
    std::size_t dataSize = 0;

    bool compressed = false;

    // Immediate containing GRUP information. A record at the top level
    // has groupType == -1; otherwise these describe the GRUP immediately
    // containing the record.
    std::int32_t groupType = -1;
    std::uint32_t groupLabel = 0;
    std::size_t groupHeaderOffset =
        std::numeric_limits<std::size_t>::max();
    std::size_t groupEndOffset =
        std::numeric_limits<std::size_t>::max();

    // GRUP context retained while flattening the record tree.
    // These are the enclosing WRLD/CELL FormIDs needed to associate LAND
    // records with their actual worldspace/cell.
    std::uint32_t parentWorldspaceFormId = 0;
    std::uint32_t parentCellFormId = 0;

    std::vector<SubRecord> subRecords;

    // Retains the original record data exactly as stored on disk when the
    // record is compressed. For uncompressed records this remains empty.
    std::vector<std::uint8_t> payload;
};
