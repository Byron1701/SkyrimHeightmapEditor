#include "terrain/TerrainViewport.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace
{
constexpr float LandVertexSpacing = 128.0f;

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

    ImGuiIO& io = ImGui::GetIO();

    if (ImGui::IsItemHovered())
    {
        if (ImGui::IsMouseDragging(
                ImGuiMouseButton_Left))
        {
            yaw +=
                io.MouseDelta.x * 0.012f;

            pitch +=
                io.MouseDelta.y * 0.012f;

            pitch =
                std::clamp(
                    pitch,
                    -1.45f,
                    1.45f);
        }

        if (ImGui::IsMouseDragging(
                ImGuiMouseButton_Right))
        {
            panX += io.MouseDelta.x;
            panY += io.MouseDelta.y;
        }

        if (std::abs(io.MouseWheel) > 0.0f)
        {
            distance *=
                std::pow(
                    0.85f,
                    io.MouseWheel);

            distance =
                std::clamp(
                    distance,
                    800.0f,
                    12000.0f);
        }
    }

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
        1150.0f;

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
                pan);

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

        const ImU32 line =
            wireframe
                ? IM_COL32(150, 155, 165, 210)
                : IM_COL32(65, 68, 74, 100);

        drawList->AddLine(a, b, line);
        drawList->AddLine(b, c, line);
        drawList->AddLine(c, a, line);
    }

    drawList->AddRect(
        min,
        max,
        IM_COL32(100, 105, 115, 255));
}
