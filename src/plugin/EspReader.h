#pragma once

#include "plugin/Plugin.h"

#include <filesystem>

class EspReader
{
public:
    static Plugin read(
        const std::filesystem::path& path);
};
