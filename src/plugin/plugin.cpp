#include "plugin/Plugin.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <unordered_map>
#include <utility>

namespace
{
constexpr std::size_t InvalidIndex =
    std::numeric_limits<std::size_t>::max();

std::uint32_t readU32(const SubRecord& sub)
{
    if (sub.data.size() < 4)
        return 0;

    return
        static_cast<std::uint32_t>(sub.data[0]) |
        (static_cast<std::uint32_t>(sub.data[1]) << 8) |
        (static_cast<std::uint32_t>(sub.data[2]) << 16) |
        (static_cast<std::uint32_t>(sub.data[3]) << 24);
}

std::int32_t readI32(const SubRecord& sub)
{
    return static_cast<std::int32_t>(readU32(sub));
}

std::string readString(const SubRecord& sub)
{
    const auto nul =
        std::find(
            sub.data.begin(),
            sub.data.end(),
            static_cast<std::uint8_t>(0));

    return std::string(
        reinterpret_cast<const char*>(sub.data.data()),
        static_cast<std::size_t>(
            std::distance(sub.data.begin(), nul)));
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

bool readCellCoordinates(
    const Record& record,
    int& x,
    int& y)
{
    const SubRecord* xclc =
        findSubRecord(record, "XCLC");

    if (xclc == nullptr ||
        xclc->data.size() < 8)
    {
        return false;
    }

    const std::uint32_t rawX =
        static_cast<std::uint32_t>(xclc->data[0]) |
        (static_cast<std::uint32_t>(xclc->data[1]) << 8) |
        (static_cast<std::uint32_t>(xclc->data[2]) << 16) |
        (static_cast<std::uint32_t>(xclc->data[3]) << 24);

    const std::uint32_t rawY =
        static_cast<std::uint32_t>(xclc->data[4]) |
        (static_cast<std::uint32_t>(xclc->data[5]) << 8) |
        (static_cast<std::uint32_t>(xclc->data[6]) << 16) |
        (static_cast<std::uint32_t>(xclc->data[7]) << 24);

    x = static_cast<int>(
        static_cast<std::int32_t>(rawX));

    y = static_cast<int>(
        static_cast<std::int32_t>(rawY));

    return true;
}

std::string readEditorId(const Record& record)
{
    const SubRecord* edid =
        findSubRecord(record, "EDID");

    return edid != nullptr
        ? readString(*edid)
        : std::string();
}
}

std::size_t Plugin::countRecords(
    const std::string& type) const
{
    return static_cast<std::size_t>(
        std::count_if(
            records.begin(),
            records.end(),
            [&](const Record& record)
            {
                return record.type == type;
            }));
}

std::size_t Plugin::countSubRecords(
    const std::string& type) const
{
    std::size_t count = 0;

    for (const Record& record : records)
    {
        count += static_cast<std::size_t>(
            std::count_if(
                record.subRecords.begin(),
                record.subRecords.end(),
                [&](const SubRecord& subRecord)
                {
                    return subRecord.type == type;
                }));
    }

    return count;
}

void Plugin::rebuildWorldIndex()
{
    worldspaces.clear();
    cells.clear();
    lands.clear();

    std::unordered_map<std::uint32_t, std::size_t>
        worldspaceByFormId;

    std::unordered_map<std::uint32_t, std::size_t>
        cellByFormId;

    for (std::size_t i = 0; i < records.size(); ++i)
    {
        const Record& record = records[i];

        if (record.type == "WRLD")
        {
            WorldspaceIndex worldspace;
            worldspace.formId = record.formId;
            worldspace.editorId = readEditorId(record);
            worldspace.recordIndex = i;

            worldspaceByFormId[worldspace.formId] =
                worldspaces.size();

            worldspaces.push_back(
                std::move(worldspace));
        }
    }

    for (std::size_t i = 0; i < records.size(); ++i)
    {
        const Record& record = records[i];

        if (record.type == "CELL")
        {
            CellIndex cell;
            cell.formId = record.formId;
            cell.worldspaceFormId =
                record.parentWorldspaceFormId;
            cell.recordIndex = i;

            cell.hasCoordinates =
                readCellCoordinates(
                    record,
                    cell.gridX,
                    cell.gridY);

            cellByFormId[cell.formId] =
                cells.size();

            cells.push_back(cell);

            const auto worldIt =
                worldspaceByFormId.find(
                    cell.worldspaceFormId);

            if (worldIt != worldspaceByFormId.end())
            {
                worldspaces[
                    worldIt->second]
                    .cellRecordIndices.push_back(i);
            }
        }
    }

    for (std::size_t i = 0; i < records.size(); ++i)
    {
        const Record& record = records[i];

        if (record.type != "LAND")
            continue;

        LandIndex land;
        land.formId = record.formId;
        land.cellFormId = record.parentCellFormId;
        land.worldspaceFormId =
            record.parentWorldspaceFormId;
        land.recordIndex = i;

        land.hasCoordinates =
            readCellCoordinates(
                record,
                land.cellX,
                land.cellY);

        const auto cellIt =
            cellByFormId.find(land.cellFormId);

        if (cellIt != cellByFormId.end())
        {
            CellIndex& cell =
                cells[cellIt->second];

            cell.landRecordIndex = i;

            if (!land.hasCoordinates)
            {
                land.cellX = cell.gridX;
                land.cellY = cell.gridY;
                land.hasCoordinates =
                    cell.hasCoordinates;
            }
        }

        lands.push_back(land);

        const auto worldIt =
            worldspaceByFormId.find(
                land.worldspaceFormId);

        if (worldIt != worldspaceByFormId.end())
        {
            worldspaces[
                worldIt->second]
                .landRecordIndices.push_back(i);
        }
    }
}
