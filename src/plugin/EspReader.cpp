#include "plugin/EspReader.h"

#include <zlib.h>

#include <fstream>
#include <iterator>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{
constexpr std::size_t RecordHeaderSize = 24;
constexpr std::size_t SubRecordHeaderSize = 6;

// Skyrim/TES record flag indicating compressed record data.
constexpr std::uint32_t CompressedFlag = 0x00040000u;

void requireRange(
    std::size_t offset,
    std::size_t length,
    std::size_t total,
    const char* description)
{
    if (offset > total ||
        length > total - offset)
    {
        std::ostringstream stream;

        stream << "Invalid "
               << description
               << " range at 0x"
               << std::hex
               << offset;

        throw std::runtime_error(
            stream.str());
    }
}

std::uint16_t readU16(
    const std::vector<std::uint8_t>& data,
    std::size_t offset)
{
    requireRange(
        offset,
        2,
        data.size(),
        "uint16");

    return static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(
            data[offset]) |
        static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(
                data[offset + 1]) << 8));
}

std::uint32_t readU32(
    const std::vector<std::uint8_t>& data,
    std::size_t offset)
{
    requireRange(
        offset,
        4,
        data.size(),
        "uint32");

    return
        static_cast<std::uint32_t>(
            data[offset]) |
        (static_cast<std::uint32_t>(
            data[offset + 1]) << 8) |
        (static_cast<std::uint32_t>(
            data[offset + 2]) << 16) |
        (static_cast<std::uint32_t>(
            data[offset + 3]) << 24);
}

std::vector<std::uint8_t> decompressRecord(
    const std::vector<std::uint8_t>& data,
    std::size_t offset,
    std::size_t size)
{
    /*
     * Skyrim compressed record data is:
     *
     *   uint32 uncompressed size
     *   zlib stream
     *
     * The record header's data-size includes both fields.
     */
    if (size < 4)
    {
        throw std::runtime_error(
            "Compressed record is too small to contain its uncompressed size.");
    }

    const std::uint32_t expectedSize =
        readU32(data, offset);

    if (expectedSize == 0)
    {
        return {};
    }

    requireRange(
        offset + 4,
        size - 4,
        data.size(),
        "compressed record payload");

    std::vector<std::uint8_t> output(
        static_cast<std::size_t>(expectedSize));

    uLongf outputSize =
        static_cast<uLongf>(output.size());

    const Bytef* compressedData =
        reinterpret_cast<const Bytef*>(
            data.data() + offset + 4);

    const uLong compressedSize =
        static_cast<uLong>(size - 4);

    const int result =
        ::uncompress(
            reinterpret_cast<Bytef*>(output.data()),
            &outputSize,
            compressedData,
            compressedSize);

    if (result != Z_OK)
    {
        std::ostringstream stream;

        stream << "Could not decompress record payload (zlib error "
               << result << ").";

        throw std::runtime_error(stream.str());
    }

    if (outputSize != output.size())
    {
        throw std::runtime_error(
            "Compressed record decompressed to an unexpected size.");
    }

    return output;
}

std::string readType(
    const std::vector<std::uint8_t>& data,
    std::size_t offset)
{
    requireRange(
        offset,
        4,
        data.size(),
        "record/subrecord signature");

    return std::string(
        reinterpret_cast<const char*>(
            data.data() + offset),
        4);
}

std::vector<SubRecord> parseSubRecords(
    const std::vector<std::uint8_t>& data,
    std::size_t dataOffset,
    std::size_t dataSize)
{
    const std::size_t end =
        dataOffset + dataSize;

    std::size_t cursor =
        dataOffset;

    std::vector<SubRecord> subRecords;

    while (cursor < end)
    {
        requireRange(
            cursor,
            SubRecordHeaderSize,
            end,
            "subrecord header");

        const std::size_t subHeaderOffset =
            cursor;

        std::string subType =
            readType(data, cursor);

        const std::uint16_t rawSize =
            readU16(data, cursor + 4);

        cursor += SubRecordHeaderSize;

        std::size_t actualSize =
            rawSize;

        /*
         * XXXX is an extended-size marker. It applies to the
         * immediately following subrecord.
         */
        if (subType == "XXXX")
        {
            if (rawSize != 4)
            {
                throw std::runtime_error(
                    "Malformed XXXX subrecord.");
            }

            requireRange(
                cursor,
                4,
                end,
                "XXXX payload");

            actualSize =
                readU32(data, cursor);

            cursor += 4;

            requireRange(
                cursor,
                SubRecordHeaderSize,
                end,
                "subrecord following XXXX");

            subType =
                readType(data, cursor);

            cursor += SubRecordHeaderSize;
        }

        requireRange(
            cursor,
            actualSize,
            end,
            "subrecord payload");

        SubRecord subRecord;

        subRecord.type =
            subType;

        subRecord.offset =
            subHeaderOffset;

        subRecord.dataOffset =
            cursor;

        subRecord.size =
            actualSize;

        subRecord.rawSize =
            rawSize;

        subRecord.data.assign(
            data.begin() +
                static_cast<std::ptrdiff_t>(cursor),
            data.begin() +
                static_cast<std::ptrdiff_t>(
                    cursor + actualSize));

        subRecords.push_back(
            std::move(subRecord));

        cursor += actualSize;
    }

    return subRecords;
}

Record parseRecord(
    const std::vector<std::uint8_t>& data,
    std::size_t headerOffset,
    std::size_t dataOffset,
    std::size_t dataSize)
{
    Record record;

    record.type =
        readType(data, headerOffset);

    record.headerOffset =
        headerOffset;

    record.dataOffset =
        dataOffset;
    record.dataSize =
        dataSize;
    record.endOffset =
        dataOffset + dataSize;

    record.subRecords =
        parseSubRecords(
            data,
            dataOffset,
            dataSize);

    return record;
}

void parseContainer(
    const std::vector<std::uint8_t>& data,
    std::size_t offset,
    std::size_t end,
    std::vector<Record>& records,
    std::uint32_t parentWorldspaceFormId = 0,
    std::uint32_t parentCellFormId = 0,
    std::int32_t containingGroupType = -1,
    std::uint32_t containingGroupLabel = 0,
    std::size_t containingGroupHeaderOffset = std::numeric_limits<std::size_t>::max(),
    std::size_t containingGroupEndOffset = std::numeric_limits<std::size_t>::max())
{
    while (offset < end)
    {
        requireRange(
            offset,
            RecordHeaderSize,
            end,
            "record header");

        const std::string type =
            readType(data, offset);

        const std::uint32_t size =
            readU32(data, offset + 4);

        if (type == "GRUP")
        {
            if (size < RecordHeaderSize)
            {
                throw std::runtime_error(
                    "Malformed GRUP with size smaller than its header.");
            }

            const std::size_t groupEnd =
                offset + static_cast<std::size_t>(size);

            requireRange(
                offset,
                static_cast<std::size_t>(size),
                end,
                "GRUP");

            /*
             * GRUP header:
             *   +00 signature
             *   +04 size
             *   +08 group label
             *   +0C group type
             *   +10 timestamp
             *   +12 unknown
             *   +14 version
             *   +16 unknown
             *
             * For the terrain hierarchy we care about:
             *   type 1 = worldspace children; label is WRLD FormID.
             *   type 6 = cell children; label is CELL FormID.
             *
             * Other group types inherit the enclosing context.
             */
            const std::uint32_t groupLabel =
                readU32(data, offset + 8);

            const std::uint32_t groupType =
                readU32(data, offset + 12);

            std::uint32_t childWorldspace =
                parentWorldspaceFormId;

            std::uint32_t childCell =
                parentCellFormId;

            if (groupType == 1)
            {
                childWorldspace = groupLabel;
                childCell = 0;
            }
            else if (groupType == 6)
            {
                childCell = groupLabel;
            }

            parseContainer(
                data,
                offset + RecordHeaderSize,
                groupEnd,
                records,
                childWorldspace,
                childCell,
                static_cast<std::int32_t>(groupType),
                groupLabel,
                offset,
                groupEnd);

            offset = groupEnd;
            continue;
        }

        requireRange(
            offset + RecordHeaderSize,
            size,
            end,
            "record payload");

        Record record;

        record.type = type;
        record.flags = readU32(data, offset + 8);
        record.formId = readU32(data, offset + 12);
        record.timestamp = readU16(data, offset + 16);
        record.vcs1 = readU16(data, offset + 18);
        record.version = readU16(data, offset + 20);
        record.unknown = readU16(data, offset + 22);
        record.headerOffset = offset;
        record.dataOffset = offset + RecordHeaderSize;
        record.dataSize = size;
        record.endOffset = record.dataOffset + size;
        record.compressed = (record.flags & CompressedFlag) != 0;
        record.parentWorldspaceFormId = parentWorldspaceFormId;
        record.parentCellFormId = parentCellFormId;
        record.groupType = containingGroupType;
        record.groupLabel = containingGroupLabel;
        record.groupHeaderOffset = containingGroupHeaderOffset;
        record.groupEndOffset = containingGroupEndOffset;

        if (record.compressed)
        {
            record.payload.assign(
                data.begin() +
                    static_cast<std::ptrdiff_t>(
                        record.dataOffset),
                data.begin() +
                    static_cast<std::ptrdiff_t>(
                        record.dataOffset + size));

            const std::vector<std::uint8_t> decompressed =
                decompressRecord(
                    data,
                    record.dataOffset,
                    size);

            Record parsed = record;

            parsed.subRecords =
                parseSubRecords(
                    decompressed,
                    0,
                    decompressed.size());

            /*
             * Subrecord offsets in a decompressed payload are relative
             * to that payload. Keep the original record header offset
             * and original data offset in the Record itself; the
             * SubRecord offsets are diagnostic offsets into the
             * decompressed representation.
             */
            parsed.compressed = true;
            parsed.parentWorldspaceFormId =
                parentWorldspaceFormId;
            parsed.parentCellFormId =
                parentCellFormId;
            parsed.groupType = containingGroupType;
            parsed.groupLabel = containingGroupLabel;
            parsed.groupHeaderOffset = containingGroupHeaderOffset;
            parsed.groupEndOffset = containingGroupEndOffset;
            record = std::move(parsed);
        }
        else
        {
            Record parsed =
                parseRecord(
                    data,
                    offset,
                    record.dataOffset,
                    size);

            parsed.flags = record.flags;
            parsed.formId = record.formId;
            parsed.timestamp = record.timestamp;
            parsed.vcs1 = record.vcs1;
            parsed.version = record.version;
            parsed.unknown = record.unknown;
            parsed.compressed = false;
            parsed.parentWorldspaceFormId =
                parentWorldspaceFormId;
            parsed.parentCellFormId =
                parentCellFormId;
            record = std::move(parsed);
        }

        records.push_back(std::move(record));

        offset +=
            RecordHeaderSize +
            static_cast<std::size_t>(size);
    }
}
}

Plugin EspReader::read(
    const std::filesystem::path& path)
{
    std::ifstream file(
        path,
        std::ios::binary);

    if (!file)
    {
        throw std::runtime_error(
            "Could not open plugin: " +
            path.string());
    }

    std::vector<std::uint8_t> data(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    if (data.size() < RecordHeaderSize)
    {
        throw std::runtime_error(
            "File is too small to contain "
            "a TES record.");
    }

    Plugin plugin;
    plugin.path = path.string();
    plugin.filename = path.filename().string();

    parseContainer(
        data,
        0,
        data.size(),
        plugin.records);

    return plugin;
}
