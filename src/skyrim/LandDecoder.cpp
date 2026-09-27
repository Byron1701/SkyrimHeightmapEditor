#include "skyrim/LandDecoder.h"

#include <cmath>
#include <cstdint>
#include <cstring>

namespace
{
constexpr std::size_t GridSize = 33;
constexpr std::size_t HeightDataCount = GridSize * GridSize;
constexpr std::size_t MinimumVHGTSize = 4 + HeightDataCount + 3;
constexpr float HeightScale = 8.0f;

std::int8_t readI8(std::uint8_t value)
{
    return static_cast<std::int8_t>(value);
}

float readFloat32LE(const std::vector<std::uint8_t>& data)
{
    std::uint32_t bits =
        static_cast<std::uint32_t>(data[0]) |
        (static_cast<std::uint32_t>(data[1]) << 8) |
        (static_cast<std::uint32_t>(data[2]) << 16) |
        (static_cast<std::uint32_t>(data[3]) << 24);

    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

const SubRecord* findSubRecord(
    const Record& record,
    const char* type)
{
    for (const SubRecord& sub : record.subRecords)
    {
        if (sub.type == type)
            return &sub;
    }

    return nullptr;
}
}

LandDecodeResult LandDecoder::decode(
    const Record& record)
{
    LandDecodeResult result;

    if (record.type != "LAND")
    {
        result.error = "Record is not a LAND record.";
        return result;
    }

    if (record.compressed)
    {
        result.error =
            "LAND record is compressed; VHGT is not "
            "available until record decompression is implemented.";
        return result;
    }

    const SubRecord* vhgt =
        findSubRecord(record, "VHGT");

    if (vhgt == nullptr)
    {
        result.error = "LAND record has no VHGT subrecord.";
        return result;
    }

    if (vhgt->data.size() < MinimumVHGTSize)
    {
        result.error =
            "VHGT subrecord is too small: expected at least "
            "1096 bytes.";
        return result;
    }

    result.offset =
        readFloat32LE(vhgt->data);

    if (!std::isfinite(result.offset))
    {
        result.error = "VHGT offset is not a finite float.";
        return result;
    }

    result.heightField =
        HeightField(GridSize, GridSize);

    /*
     * VHGT stores signed byte differences, not absolute heights.
     *
     * The first vertex is:
     *
     *     Offset + delta * 8
     *
     * The first vertex of every subsequent row is relative to
     * the first vertex of the preceding row. Remaining vertices
     * in a row are relative to the immediately preceding vertex.
     *
     * The resulting values are Skyrim world Z coordinates.
     */
    float rowStart = 0.0f;
    float previous = 0.0f;

    std::size_t cursor = 4;

    for (std::size_t y = 0; y < GridSize; ++y)
    {
        for (std::size_t x = 0; x < GridSize; ++x)
        {
            const std::int8_t delta =
                readI8(vhgt->data[cursor++]);

            if (x == 0)
            {
                rowStart +=
                    static_cast<float>(delta) *
                    HeightScale;

                previous = rowStart;
            }
            else
            {
                previous +=
                    static_cast<float>(delta) *
                    HeightScale;
            }

            result.heightField.at(x, y) =
                result.offset + previous;
        }
    }

    result.valid = true;
    return result;
}
