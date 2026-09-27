#pragma once

#include "skyrim/Record.h"
#include "terrain/HeightField.h"

#include <string>

struct LandDecodeResult
{
    HeightField heightField;
    float offset = 0.0f;
    bool valid = false;
    std::string error;
};

class LandDecoder
{
public:
    static LandDecodeResult decode(
        const Record& record);
};
