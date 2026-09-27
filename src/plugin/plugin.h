#pragma once

#include "skyrim/Record.h"
#include "skyrim/SkyrimTypes.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct WorldspaceIndex
{
    std::uint32_t formId = 0;
    std::string editorId;
    std::size_t recordIndex = 0;
    std::vector<std::size_t> cellRecordIndices;
    std::vector<std::size_t> landRecordIndices;
};

struct CellIndex
{
    std::uint32_t formId = 0;
    std::uint32_t worldspaceFormId = 0;
    int gridX = 0;
    int gridY = 0;
    bool hasCoordinates = false;
    std::size_t recordIndex = 0;
    std::size_t landRecordIndex = static_cast<std::size_t>(-1);
};

struct LandIndex
{
    std::uint32_t formId = 0;
    std::uint32_t cellFormId = 0;
    std::uint32_t worldspaceFormId = 0;
    int cellX = 0;
    int cellY = 0;
    bool hasCoordinates = false;
    std::size_t recordIndex = 0;
};

struct Plugin
{
    std::string path;
    std::string filename;
    std::vector<Record> records;

    std::vector<WorldspaceIndex> worldspaces;
    std::vector<CellIndex> cells;
    std::vector<LandIndex> lands;

    bool loaded() const
    {
        return !path.empty();
    }

    std::size_t countRecords(
        const std::string& type) const;

    std::size_t countSubRecords(
        const std::string& type) const;

    void rebuildWorldIndex();
};
