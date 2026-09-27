#include "terrain/TerrainViewport.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace
{
struct Vec3
{
    float x;
    float y;
    float z;
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

    return {v.x / l, v.y / l, v.z / l};
}

ImVec2 project(
    const Vec3& p,
    float yaw,
    float pitch,
    float distance,
    const ImVec2& centre,
    float scale)
{
    const float cy = std::cos(yaw);
    const float sy = std::sin(yaw);
    const float cp = std::cos(pitch);
    const float sp = std::sin(pitch);

    const float x1 = p.x * cy - p.z * sy;
    const float z1 = p.x * sy + p.z * cy;

    const float y1 = p.y * cp - z1 * sp;
    const float z2 = p.y * sp + z1 * cp;

    const float cameraDistance =
        distance + z2;

    const float perspective =
        cameraDistance > 0.05f
            ? scale / cameraDistance
            : scale / 0.05f;

    return {
        centre.x + x1 * perspective,
        centre.y - y1 * perspective
    };
}

ImU32 shade(
    const Vec3& normal)
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
        "Left drag: orbit    Mouse wheel: zoom    "
        "Right drag: pan");

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
            // Keep panning deliberately modest for this
            // first renderer; it will be replaced by a
            // proper camera once the terrain geometry is
            // validated.
            // The current viewport remains centred.
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
                    0.35f,
                    20.0f);
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

    const float minHeight =
        heightField.minimum();

    const float maxHeight =
        heightField.maximum();

    const float heightRange =
        std::max(
            1.0f,
            maxHeight - minHeight);

    const float horizontalScale =
        1.0f / static_cast<float>(
            std::max(width, height));

    const float scale =
        620.0f *
        horizontalScale;

    auto vertex =
        [&](std::size_t x, std::size_t y)
        {
            const float worldX =
                (static_cast<float>(x) - xCentre);

            const float worldZ =
                (static_cast<float>(y) - zCentre);

            const float worldY =
                (heightField.at(x, y) - minHeight) /
                heightRange *
                verticalScale;

            return Vec3{
                worldX,
                worldY,
                worldZ
            };
        };

    /*
     * Draw back-to-front by rows. At this stage this is a
     * deliberately lightweight software projection rather
     * than a separate OpenGL terrain renderer.
     */
    for (std::size_t y = 0; y + 1 < height; ++y)
    {
        for (std::size_t x = 0; x + 1 < width; ++x)
        {
            const Vec3 v00 = vertex(x, y);
            const Vec3 v10 = vertex(x + 1, y);
            const Vec3 v01 = vertex(x, y + 1);
            const Vec3 v11 = vertex(x + 1, y + 1);

            const Vec3 n0 =
                normalise(
                    cross(
                        v10 - v00,
                        v01 - v00));

            const Vec3 n1 =
                normalise(
                    cross(
                        v11 - v10,
                        v01 - v10));

            const ImVec2 p00 =
                project(
                    v00, yaw, pitch,
                    distance, centre, scale);

            const ImVec2 p10 =
                project(
                    v10, yaw, pitch,
                    distance, centre, scale);

            const ImVec2 p01 =
                project(
                    v01, yaw, pitch,
                    distance, centre, scale);

            const ImVec2 p11 =
                project(
                    v11, yaw, pitch,
                    distance, centre, scale);

            if (!wireframe)
            {
                drawList->AddTriangleFilled(
                    p00, p10, p01, shade(n0));

                drawList->AddTriangleFilled(
                    p10, p11, p01, shade(n1));
            }

            const ImU32 line =
                IM_COL32(85, 90, 98, 180);

            drawList->AddLine(
                p00, p10, line);

            drawList->AddLine(
                p10, p11, line);

            drawList->AddLine(
                p11, p01, line);

            drawList->AddLine(
                p01, p00, line);

            if (!wireframe)
            {
                // Internal diagonal is useful when
                // validating triangulation but visually
                // subdued in shaded mode.
                drawList->AddLine(
                    p10, p01,
                    IM_COL32(65, 68, 74, 90));
            }
        }
    }

    drawList->AddRect(
        min,
        max,
        IM_COL32(100, 105, 115, 255));
}
