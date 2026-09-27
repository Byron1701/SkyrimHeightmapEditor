#pragma once

#include "terrain/HeightField.h"

class TerrainViewport
{
public:
    void draw(
        const HeightField& heightField,
        bool wireframe);

    float yaw = 0.75f;
    float pitch = 0.75f;
    float distance = 2.8f;
    float panX = 0.0f;
    float panY = 0.0f;
};
