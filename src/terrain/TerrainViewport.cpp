#include "terrain/TerrainViewport.h"

#include <imgui.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_TEXTURE_SWIZZLE_RGBA
#define GL_TEXTURE_SWIZZLE_RGBA 0x8E46
#endif
#ifndef GL_R16
#define GL_R16 0x822A
#endif
#ifndef GL_RED
#define GL_RED 0x1903
#endif


namespace
{
constexpr float LandVertexSpacing = 128.0f;
constexpr float LandCellSize = 4096.0f;
constexpr float HeightMap16Bias = 32768.0f;
constexpr float HeightMap16Scale = 1.0f / 8.0f;

struct Vec3
{
    float x;
    float y;
    float z;
};

struct Triangle
{
    Vec3 a;
    Vec3 b;
    Vec3 c;
    float depth;
};

Vec3 operator-(const Vec3& a, const Vec3& b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 cross(const Vec3& a, const Vec3& b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

float length(const Vec3& v)
{
    return std::sqrt(
        v.x * v.x +
        v.y * v.y +
        v.z * v.z);
}

Vec3 normalise(const Vec3& v)
{
    const float l = length(v);

    if (l <= 0.000001f)
        return {0.0f, 1.0f, 0.0f};

    return {
        v.x / l,
        v.y / l,
        v.z / l
    };
}

Vec3 rotate(
    const Vec3& p,
    float yaw,
    float pitch)
{
    const float cy = std::cos(yaw);
    const float sy = std::sin(yaw);
    const float cp = std::cos(pitch);
    const float sp = std::sin(pitch);

    const float x =
        p.x * cy -
        p.z * sy;

    const float z =
        p.x * sy +
        p.z * cy;

    return {
        x,
        p.y * cp - z * sp,
        p.y * sp + z * cp
    };
}

ImVec2 project(
    const Vec3& p,
    float distance,
    const ImVec2& centre,
    float scale,
    const ImVec2& pan)
{
    const float cameraZ =
        distance + p.z;

    const float perspective =
        cameraZ > 1.0f
            ? scale / cameraZ
            : scale;

    return {
        centre.x +
            pan.x +
            p.x * perspective,
        centre.y +
            pan.y -
            p.y * perspective
    };
}

ImU32 shade(const Vec3& normal)
{
    const Vec3 light =
        normalise({-0.45f, 0.8f, 0.55f});

    const float diffuse =
        std::max(
            0.0f,
            normal.x * light.x +
            normal.y * light.y +
            normal.z * light.z);

    const int value =
        static_cast<int>(
            45.0f + diffuse * 175.0f);

    return IM_COL32(
        value,
        value,
        value,
        255);
}
}


std::uint16_t encodeAbsoluteHeight16(float height)
{
    const float value =
        std::round(
            height * HeightMap16Scale +
            HeightMap16Bias);

    if (value <= 0.0f)
        return 0;

    if (value >= 65535.0f)
        return 65535;

    return static_cast<std::uint16_t>(value);
}

void handleViewportInput(
    float& yaw,
    float& pitch,
    float& distance,
    float& panX,
    float& panY)
{
    ImGuiIO& io = ImGui::GetIO();

    if (!ImGui::IsItemHovered())
        return;

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        yaw += io.MouseDelta.x * 0.012f;
        pitch += io.MouseDelta.y * 0.012f;
        pitch = std::clamp(pitch, -1.45f, 1.45f);
    }

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
    {
        panX += io.MouseDelta.x;
        panY += io.MouseDelta.y;
    }

    if (std::abs(io.MouseWheel) > 0.0f)
    {
        distance *= std::pow(0.85f, io.MouseWheel);
        distance = std::clamp(distance, 1000.0f, 100000.0f);
    }
}

void TerrainViewport::draw(
    const HeightField& heightField,
    bool wireframe)
{
    if (heightField.width() < 2 ||
        heightField.height() < 2)
    {
        ImGui::TextUnformatted(
            "Heightfield is too small to render.");
        return;
    }

    ImGui::TextUnformatted(
        "Left drag: orbit    Right drag: pan    "
        "Mouse wheel: zoom");

    ImGui::SameLine();

    ImGui::Text(
        "| Wireframe: %s",
        wireframe ? "on" : "off");

    const ImVec2 available =
        ImGui::GetContentRegionAvail();

    const float viewportWidth =
        std::max(300.0f, available.x);

    const float viewportHeight =
        std::max(300.0f, available.y);

    ImGui::InvisibleButton(
        "Terrain3DViewport",
        ImVec2(viewportWidth, viewportHeight),
        ImGuiButtonFlags_MouseButtonLeft |
        ImGuiButtonFlags_MouseButtonRight);

    const ImVec2 min =
        ImGui::GetItemRectMin();

    const ImVec2 max =
        ImGui::GetItemRectMax();

    const ImVec2 centre(
        (min.x + max.x) * 0.5f,
        (min.y + max.y) * 0.5f);

    ImDrawList* drawList =
        ImGui::GetWindowDrawList();

    drawList->AddRectFilled(
        min,
        max,
        IM_COL32(24, 26, 30, 255));

    handleViewportInput(yaw, pitch, distance, panX, panY);

    const std::size_t width =
        heightField.width();

    const std::size_t height =
        heightField.height();

    const float xCentre =
        static_cast<float>(width - 1) * 0.5f;

    const float zCentre =
        static_cast<float>(height - 1) * 0.5f;

    auto vertex =
        [&](std::size_t x, std::size_t y)
        {
            const Vec3 world{
                (static_cast<float>(x) - xCentre) *
                    LandVertexSpacing,
                heightField.at(x, y),
                (static_cast<float>(y) - zCentre) *
                    LandVertexSpacing
            };

            return rotate(
                world,
                yaw,
                pitch);
        };

    const float scale =
        650.0f;

    std::vector<Triangle> triangles;
    triangles.reserve(
        (width - 1) *
        (height - 1) *
        2);

    for (std::size_t y = 0; y + 1 < height; ++y)
    {
        for (std::size_t x = 0; x + 1 < width; ++x)
        {
            const Vec3 v00 = vertex(x, y);
            const Vec3 v10 = vertex(x + 1, y);
            const Vec3 v01 = vertex(x, y + 1);
            const Vec3 v11 = vertex(x + 1, y + 1);

            triangles.push_back({
                v00,
                v10,
                v01,
                (v00.z + v10.z + v01.z) / 3.0f
            });

            triangles.push_back({
                v10,
                v11,
                v01,
                (v10.z + v11.z + v01.z) / 3.0f
            });
        }
    }

    std::sort(
        triangles.begin(),
        triangles.end(),
        [](const Triangle& a, const Triangle& b)
        {
            return a.depth < b.depth;
        });

    for (const Triangle& triangle : triangles)
    {
        const ImVec2 a =
            project(
                triangle.a,
                distance,
                centre,
                scale,
                ImVec2(panX, panY));

        const ImVec2 b =
            project(
                triangle.b,
                distance,
                centre,
                scale,
                ImVec2(panX, panY));

        const ImVec2 c =
            project(
                triangle.c,
                distance,
                centre,
                scale,
                ImVec2(panX, panY));

        const Vec3 normal =
            normalise(
                cross(
                    triangle.b - triangle.a,
                    triangle.c - triangle.a));

        if (!wireframe)
        {
            drawList->AddTriangleFilled(
                a,
                b,
                c,
                shade(normal));
        }

        if (wireframe)
        {
            const ImU32 line =
                IM_COL32(150, 155, 165, 210);

            drawList->AddLine(a, b, line);
            drawList->AddLine(b, c, line);
            drawList->AddLine(c, a, line);
        }
    }

    drawList->AddRect(
        min,
        max,
        IM_COL32(100, 105, 115, 255));
}


void TerrainViewport::drawWorldspace(
    const std::vector<TerrainWorldCell>& cells,
    bool wireframe)
{
    ImGui::TextUnformatted(
        "Left drag: orbit    Right drag: pan    Mouse wheel: zoom");

    ImGui::SameLine();

    ImGui::Text(
        "| Cells: %zu | Wireframe: %s",
        cells.size(),
        wireframe ? "on" : "off");

    const ImVec2 available = ImGui::GetContentRegionAvail();
    const float viewportWidth = std::max(300.0f, available.x);
    const float viewportHeight = std::max(300.0f, available.y);

    ImGui::InvisibleButton(
        "TerrainWorldspaceViewport",
        ImVec2(viewportWidth, viewportHeight),
        ImGuiButtonFlags_MouseButtonLeft |
        ImGuiButtonFlags_MouseButtonRight);

    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    const ImVec2 centre(
        (min.x + max.x) * 0.5f,
        (min.y + max.y) * 0.5f);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(min, max, IM_COL32(24, 26, 30, 255));

    handleViewportInput(yaw, pitch, distance, panX, panY);

    int minGridX = std::numeric_limits<int>::max();
    int maxGridX = std::numeric_limits<int>::min();
    int minGridY = std::numeric_limits<int>::max();
    int maxGridY = std::numeric_limits<int>::min();

    for (const TerrainWorldCell& cell : cells)
    {
        if (cell.heightField == nullptr)
            continue;

        minGridX = std::min(minGridX, cell.gridX);
        maxGridX = std::max(maxGridX, cell.gridX);
        minGridY = std::min(minGridY, cell.gridY);
        maxGridY = std::max(maxGridY, cell.gridY);
    }

    if (minGridX > maxGridX || minGridY > maxGridY)
    {
        ImGui::TextUnformatted("No terrain cells have coordinates.");
        drawList->AddRect(min, max, IM_COL32(100, 105, 115, 255));
        return;
    }

    const float gridCentreX =
        (static_cast<float>(minGridX) + static_cast<float>(maxGridX)) * 0.5f;
    const float gridCentreY =
        (static_cast<float>(minGridY) + static_cast<float>(maxGridY)) * 0.5f;

    const float extentX =
        (static_cast<float>(maxGridX - minGridX) + 1.0f) * LandCellSize;
    const float extentZ =
        (static_cast<float>(maxGridY - minGridY) + 1.0f) * LandCellSize;

    const float scale =
        std::clamp(
            650.0f *
                (8192.0f / std::max(8192.0f, std::max(extentX, extentZ))),
            90.0f,
            650.0f);

    std::vector<Triangle> triangles;

    std::size_t triangleCount = 0;
    for (const TerrainWorldCell& cell : cells)
    {
        if (cell.heightField == nullptr)
            continue;

        const HeightField& hf = *cell.heightField;
        if (hf.width() >= 2 && hf.height() >= 2)
        {
            triangleCount +=
                (hf.width() - 1) * (hf.height() - 1) * 2;
        }
    }
    triangles.reserve(triangleCount);

    for (const TerrainWorldCell& cell : cells)
    {
        if (cell.heightField == nullptr)
            continue;

        const HeightField& hf = *cell.heightField;
        if (hf.width() < 2 || hf.height() < 2)
            continue;

        const float xCentre = static_cast<float>(hf.width() - 1) * 0.5f;
        const float zCentre = static_cast<float>(hf.height() - 1) * 0.5f;

        const float cellOriginX =
            (static_cast<float>(cell.gridX) - gridCentreX) * LandCellSize;
        const float cellOriginZ =
            (static_cast<float>(cell.gridY) - gridCentreY) * LandCellSize;

        auto vertex = [&](std::size_t x, std::size_t y)
        {
            const Vec3 world{
                cellOriginX +
                    (static_cast<float>(x) - xCentre) * LandVertexSpacing,
                hf.at(x, y),
                cellOriginZ +
                    (static_cast<float>(y) - zCentre) * LandVertexSpacing
            };

            return rotate(world, yaw, pitch);
        };

        for (std::size_t y = 0; y + 1 < hf.height(); ++y)
        {
            for (std::size_t x = 0; x + 1 < hf.width(); ++x)
            {
                const Vec3 v00 = vertex(x, y);
                const Vec3 v10 = vertex(x + 1, y);
                const Vec3 v01 = vertex(x, y + 1);
                const Vec3 v11 = vertex(x + 1, y + 1);

                triangles.push_back({
                    v00, v10, v01,
                    (v00.z + v10.z + v01.z) / 3.0f
                });

                triangles.push_back({
                    v10, v11, v01,
                    (v10.z + v11.z + v01.z) / 3.0f
                });
            }
        }
    }

    std::sort(
        triangles.begin(),
        triangles.end(),
        [](const Triangle& a, const Triangle& b)
        {
            return a.depth < b.depth;
        });

    for (const Triangle& triangle : triangles)
    {
        const ImVec2 a = project(
            triangle.a, distance, centre, scale, ImVec2(panX, panY));
        const ImVec2 b = project(
            triangle.b, distance, centre, scale, ImVec2(panX, panY));
        const ImVec2 c = project(
            triangle.c, distance, centre, scale, ImVec2(panX, panY));

        if (!wireframe)
        {
            const Vec3 normal =
                normalise(cross(
                    triangle.b - triangle.a,
                    triangle.c - triangle.a));

            drawList->AddTriangleFilled(a, b, c, shade(normal));
        }
        else
        {
            const ImU32 line = IM_COL32(150, 155, 165, 210);
            drawList->AddLine(a, b, line);
            drawList->AddLine(b, c, line);
            drawList->AddLine(c, a, line);
        }
    }

    drawList->AddRect(min, max, IM_COL32(100, 105, 115, 255));
}

void TerrainViewport::drawHeightfield16(
    const HeightField& heightField)
{
    if (heightField.width() == 0 || heightField.height() == 0)
    {
        ImGui::TextUnformatted("Heightfield is empty.");
        return;
    }

    ImGui::TextUnformatted("16-bit absolute grayscale");
    ImGui::TextUnformatted(
        "Black = -262144 GU; white = 262136 GU; one 16-bit step = 8 GU.");

    const std::size_t width = heightField.width();
    const std::size_t height = heightField.height();

    std::vector<std::uint16_t> pixels(width * height);

    for (std::size_t y = 0; y < height; ++y)
    {
        for (std::size_t x = 0; x < width; ++x)
        {
            pixels[y * width + x] =
                encodeAbsoluteHeight16(heightField.at(x, y));
        }
    }

    const float mapSize =
        std::clamp(ImGui::GetContentRegionAvail().x, 260.0f, 520.0f);

    if (heightmapTexture_ == 0)
        glGenTextures(1, &heightmapTexture_);

    glBindTexture(GL_TEXTURE_2D, heightmapTexture_);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    const GLint swizzle[] = {
        GL_RED, GL_RED, GL_RED, GL_ONE
    };

    glTexParameteriv(
        GL_TEXTURE_2D,
        GL_TEXTURE_SWIZZLE_RGBA,
        swizzle);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R16,
        static_cast<GLsizei>(width),
        static_cast<GLsizei>(height),
        0,
        GL_RED,
        GL_UNSIGNED_SHORT,
        pixels.data());

    glBindTexture(GL_TEXTURE_2D, 0);

    ImGui::Image(
        static_cast<ImTextureID>(
            static_cast<std::uintptr_t>(heightmapTexture_)),
        ImVec2(mapSize, mapSize));

    const std::size_t centreX = width / 2;
    const std::size_t centreY = height / 2;
    const float centreHeight =
        heightField.at(centreX, centreY);
    const std::uint16_t centreValue =
        encodeAbsoluteHeight16(centreHeight);

    ImGui::Text(
        "Centre: %.3f GU -> 0x%04X (%u)",
        centreHeight,
        centreValue,
        static_cast<unsigned int>(centreValue));

    ImGui::Text(
        "Stored range: %.3f to %.3f GU",
        heightField.minimum(),
        heightField.maximum());
}

TerrainViewport::~TerrainViewport()
{
    if (heightmapTexture_ != 0)
    {
        glDeleteTextures(1, &heightmapTexture_);
        heightmapTexture_ = 0;
    }
}
