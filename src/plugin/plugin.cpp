#include "plugin/Plugin.h"

#include <algorithm>

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
