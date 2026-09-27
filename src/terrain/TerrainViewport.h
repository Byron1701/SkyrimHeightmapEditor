#pragma once

#include "terrain/HeightField.h"

#include <cstdint>
#include <vector>

struct TerrainWorldCell
{
    int gridX = 0;
    int gridY = 0;
    std::uint32_t formId = 0;
    const HeightField* heightField = nullptr;
};

class TerrainViewport
{
public:
    void resetView();

    void draw(
        const HeightField& heightField,
        bool wireframe);

    void drawWorldspace(
        const std::vector<TerrainWorldCell>& cells,
        bool wireframe);

    void drawHeightfield16(
        const HeightField& heightField);

    ~TerrainViewport();

    float yaw = 0.75f;
    float pitch = 0.75f;
    float distance = 12000.0f;
    float panX = 0.0f;
    float panY = 0.0f;

private:
    std::uint32_t heightmapTexture_ = 0;
    std::uint32_t heightmapPreviewTexture_ = 0;

    std::uint32_t terrainProgram_ = 0;
    std::uint32_t terrainVao_ = 0;
    std::uint32_t terrainVbo_ = 0;
    std::uint32_t terrainEbo_ = 0;
    std::uint32_t terrainFramebuffer_ = 0;
    std::uint32_t terrainColorTexture_ = 0;
    std::uint32_t terrainDepthBuffer_ = 0;
    int terrainFramebufferWidth_ = 0;
    int terrainFramebufferHeight_ = 0;
    std::size_t terrainIndexCount_ = 0;
    std::vector<std::uint64_t> terrainMeshSignature_;

    void ensureTerrainRenderer();
    void ensureTerrainFramebuffer(int width, int height);
    void rebuildTerrainMesh(const std::vector<TerrainWorldCell>& cells);
    void renderTerrainGpu(
        const std::vector<TerrainWorldCell>& cells,
        bool wireframe,
        int width,
        int height);
};
