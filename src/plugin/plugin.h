#pragma once

#include "skyrim/Record.h"

#include <cstddef>
#include <string>
#include <vector>

struct Plugin
{
    std::string path;
    std::string filename;
    std::vector<Record> records;

    bool loaded() const
    {
        return !path.empty();
    }

    std::size_t countRecords(
        const std::string& type) const;

    std::size_t countSubRecords(
        const std::string& type) const;
};
