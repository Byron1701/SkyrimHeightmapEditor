#include "ui/MainWindow.h"

#include "plugin/EspReader.h"
#include "skyrim/LandDecoder.h"

#include <imgui.h>

#include <cstdio>
#include <exception>
#include <filesystem>
#include <sstream>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#endif

MainWindow::MainWindow(Plugin& plugin)
    : plugin_(plugin)
{
}

void MainWindow::draw()
{
    ImGui::SetNextWindowPos(
        ImVec2(0, 0),
        ImGuiCond_Always);

    ImGui::SetNextWindowSize(
        ImGui::GetIO().DisplaySize,
        ImGuiCond_Always);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_MenuBar;

    ImGui::Begin(
        "Main",
        nullptr,
        flags);

    drawMenuBar();

    if (!plugin_.loaded())
    {
        ImGui::Spacing();

        ImGui::TextWrapped(
            "Open a Skyrim .esp or .esm to inspect its records.");

        ImGui::TextWrapped(
            "The terrain viewport will be added after LAND "
            "decoding has been implemented.");

        ImGui::Separator();

        ImGui::TextUnformatted(
            "Plugin path:");

        ImGui::SameLine();

        ImGui::InputText(
            "##pluginpath",
            pathBuffer_,
            sizeof(pathBuffer_));

        ImGui::SameLine();

        if (ImGui::Button("Open"))
            openPlugin();

        ImGui::Spacing();

        ImGui::TextWrapped(
            "%s",
            status_.c_str());
    }
    else
    {
        drawPluginPanel();

        ImGui::Separator();

        drawWorldIndexPanel();

        ImGui::Separator();

        drawTerrainPanel();

        ImGui::Separator();

        drawRecordPanel();
    }

    ImGui::Separator();

    ImGui::TextUnformatted(
        status_.c_str());

    ImGui::End();
}

void MainWindow::drawMenuBar()
{
    if (!ImGui::BeginMenuBar())
        return;

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Open..."))
            openPlugin();

        if (ImGui::MenuItem(
                "Close",
                nullptr,
                false,
                plugin_.loaded()))
        {
            plugin_ = Plugin{};
            selectedRecord_ = -1;
            selectedWorldspace_ = -1;
            selectedCell_ = -1;
            status_ = "Plugin closed.";
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Exit"))
            exitRequested_ = true;

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View"))
    {
        ImGui::MenuItem(
            "Terrain viewport",
            nullptr,
            false,
            false);

        ImGui::MenuItem(
            "Record inspector",
            nullptr,
            true,
            false);

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Terrain"))
    {
        ImGui::MenuItem(
            "Heightfield editing",
            nullptr,
            false,
            false);

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help"))
    {
        ImGui::TextUnformatted(
            "Skyrim Heightmap Editor");

        ImGui::TextUnformatted(
            "Initial ESP/ESM parser build");

        ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
}

void MainWindow::openPlugin()
{
#ifdef _WIN32
    OPENFILENAMEA dialog{};
    char filename[MAX_PATH]{};

    dialog.lStructSize =
        sizeof(dialog);

    dialog.lpstrFile =
        filename;

    dialog.nMaxFile =
        MAX_PATH;

    dialog.lpstrFilter =
        "Skyrim Plugins (*.esp;*.esm;*.esl)\0"
        "*.esp;*.esm;*.esl\0"
        "All Files (*.*)\0"
        "*.*\0";

    dialog.nFilterIndex = 1;

    dialog.Flags =
        OFN_PATHMUSTEXIST |
        OFN_FILEMUSTEXIST |
        OFN_NOCHANGEDIR;

    if (!GetOpenFileNameA(&dialog))
        return;

    std::snprintf(
        pathBuffer_,
        sizeof(pathBuffer_),
        "%s",
        filename);
#endif

    if (pathBuffer_[0] == '\0')
    {
        status_ =
            "Enter a plugin path first.";

        return;
    }

    try
    {
        plugin_ =
            EspReader::read(
                std::filesystem::path(
                    pathBuffer_));

        selectedRecord_ = -1;
        selectedWorldspace_ = -1;
        selectedCell_ = -1;
        plugin_.rebuildWorldIndex();

        std::ostringstream stream;

        stream << "Loaded "
               << plugin_.filename
               << " ("
               << plugin_.records.size()
               << " records).";

        status_ =
            stream.str();
    }
    catch (const std::exception& error)
    {
        plugin_ = Plugin{};
        selectedRecord_ = -1;
        selectedWorldspace_ = -1;
        selectedCell_ = -1;

        status_ =
            std::string("Error: ") +
            error.what();
    }
}

void MainWindow::drawPluginPanel()
{
    ImGui::Text(
        "Plugin: %s",
        plugin_.filename.c_str());

    ImGui::Text(
        "Records: %zu",
        plugin_.records.size());

    ImGui::SameLine();

    ImGui::Text(
        "| WRLD: %zu  CELL: %zu  LAND: %zu",
        plugin_.countRecords("WRLD"),
        plugin_.countRecords("CELL"),
        plugin_.countRecords("LAND"));
}

void MainWindow::drawWorldIndexPanel()
{
    ImGui::TextUnformatted("Worldspace / Cell / LAND index");

    ImGui::Text(
        "Worldspaces: %zu  Cells: %zu  LAND: %zu",
        plugin_.worldspaces.size(),
        plugin_.cells.size(),
        plugin_.lands.size());

    if (ImGui::BeginTabBar("WorldIndexTabs"))
    {
        if (ImGui::BeginTabItem("Worldspaces"))
        {
            const float leftWidth =
                ImGui::GetContentRegionAvail().x * 0.35f;

            ImGui::BeginChild(
                "WorldspaceList",
                ImVec2(leftWidth, 180.0f),
                true);

            for (std::size_t i = 0;
                 i < plugin_.worldspaces.size();
                 ++i)
            {
                const WorldspaceIndex& worldspace =
                    plugin_.worldspaces[i];

                char label[128]{};

                std::snprintf(
                    label,
                    sizeof(label),
                    "%08X  %s",
                    worldspace.formId,
                    worldspace.editorId.empty()
                        ? "<no EDID>"
                        : worldspace.editorId.c_str());

                if (ImGui::Selectable(
                        label,
                        selectedWorldspace_ ==
                            static_cast<int>(i)))
                {
                    selectedWorldspace_ =
                        static_cast<int>(i);
                    selectedCell_ = -1;
                }
            }

            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild(
                "WorldspaceDetails",
                ImVec2(0, 180.0f),
                true);

            if (selectedWorldspace_ >= 0 &&
                selectedWorldspace_ <
                    static_cast<int>(
                        plugin_.worldspaces.size()))
            {
                const WorldspaceIndex& worldspace =
                    plugin_.worldspaces[
                        static_cast<std::size_t>(
                            selectedWorldspace_)];

                ImGui::Text(
                    "Worldspace: %08X",
                    worldspace.formId);

                ImGui::Text(
                    "Editor ID: %s",
                    worldspace.editorId.empty()
                        ? "<none>"
                        : worldspace.editorId.c_str());

                ImGui::Text(
                    "Cells indexed: %zu",
                    worldspace.cellRecordIndices.size());

                ImGui::Text(
                    "LAND records indexed: %zu",
                    worldspace.landRecordIndices.size());
            }
            else
            {
                ImGui::TextUnformatted(
                    "Select a worldspace.");
            }

            ImGui::EndChild();

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Cells"))
        {
            const float leftWidth =
                ImGui::GetContentRegionAvail().x * 0.45f;

            ImGui::BeginChild(
                "CellList",
                ImVec2(leftWidth, 180.0f),
                true);

            for (std::size_t i = 0;
                 i < plugin_.cells.size();
                 ++i)
            {
                const CellIndex& cell =
                    plugin_.cells[i];

                char label[128]{};

                if (cell.hasCoordinates)
                {
                    std::snprintf(
                        label,
                        sizeof(label),
                        "%d, %d  %08X%s",
                        cell.gridX,
                        cell.gridY,
                        cell.formId,
                        cell.landRecordIndex != static_cast<std::size_t>(-1)
                            ? "  [LAND]"
                            : "");
                }
                else
                {
                    std::snprintf(
                        label,
                        sizeof(label),
                        "?, ?  %08X%s",
                        cell.formId,
                        cell.landRecordIndex != static_cast<std::size_t>(-1)
                            ? "  [LAND]"
                            : "");
                }

                if (ImGui::Selectable(
                        label,
                        selectedCell_ ==
                            static_cast<int>(i)))
                {
                    selectedCell_ =
                        static_cast<int>(i);
                    selectedRecord_ =
                        static_cast<int>(
                            cell.recordIndex);
                }
            }

            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild(
                "CellDetails",
                ImVec2(0, 180.0f),
                true);

            if (selectedCell_ >= 0 &&
                selectedCell_ <
                    static_cast<int>(
                        plugin_.cells.size()))
            {
                const CellIndex& cell =
                    plugin_.cells[
                        static_cast<std::size_t>(
                            selectedCell_)];

                ImGui::Text(
                    "Cell Form ID: %08X",
                    cell.formId);

                ImGui::Text(
                    "Worldspace: %08X",
                    cell.worldspaceFormId);

                if (cell.hasCoordinates)
                {
                    ImGui::Text(
                        "Grid: %d, %d",
                        cell.gridX,
                        cell.gridY);
                }
                else
                {
                    ImGui::TextUnformatted(
                        "Grid: unavailable (no XCLC)");
                }

                if (cell.landRecordIndex != static_cast<std::size_t>(-1))
                {
                    const Record& land =
                        plugin_.records[
                            cell.landRecordIndex];

                    ImGui::Text(
                        "LAND: %08X",
                        land.formId);

                    ImGui::Text(
                        "LAND file offset: 0x%zX",
                        land.headerOffset);
                }
                else
                {
                    ImGui::TextUnformatted(
                        "LAND: not indexed");
                }
            }
            else
            {
                ImGui::TextUnformatted(
                    "Select a cell.");
            }

            ImGui::EndChild();

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

void MainWindow::drawTerrainPanel()
{
    ImGui::TextUnformatted("Selected LAND heightfield");

    if (selectedCell_ < 0 ||
        selectedCell_ >= static_cast<int>(plugin_.cells.size()))
    {
        ImGui::TextUnformatted("Select a cell with an indexed LAND record.");
        return;
    }

    const CellIndex& cell =
        plugin_.cells[static_cast<std::size_t>(selectedCell_)];

    if (cell.landRecordIndex == static_cast<std::size_t>(-1))
    {
        ImGui::TextUnformatted("The selected cell has no indexed LAND record.");
        return;
    }

    const Record& land =
        plugin_.records[cell.landRecordIndex];

    const LandDecodeResult decoded =
        LandDecoder::decode(land);

    if (!decoded.valid)
    {
        ImGui::TextWrapped(
            "LAND %08X: %s",
            land.formId,
            decoded.error.c_str());
        return;
    }

    ImGui::Text(
        "LAND: %08X    Grid: %d, %d",
        land.formId,
        cell.gridX,
        cell.gridY);

    ImGui::Text(
        "Heightfield: %zu x %zu",
        decoded.heightField.width(),
        decoded.heightField.height());

    ImGui::Text(
        "VHGT offset: %.3f",
        decoded.offset);

    ImGui::Text(
        "Elevation range: %.3f to %.3f",
        decoded.heightField.minimum(),
        decoded.heightField.maximum());

    const std::size_t centre =
        decoded.heightField.width() / 2;

    ImGui::Text(
        "Centre sample: %.3f",
        decoded.heightField.at(centre, centre));

    if (ImGui::BeginTable(
            "HeightSamples",
            5,
            ImGuiTableFlags_Borders |
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_SizingStretchSame))
    {
        ImGui::TableSetupColumn("X");
        ImGui::TableSetupColumn("Y");
        ImGui::TableSetupColumn("Height");
        ImGui::TableSetupColumn("X");
        ImGui::TableSetupColumn("Y");
        ImGui::TableHeadersRow();

        const std::size_t last =
            decoded.heightField.width() - 1;

        const std::size_t samples[4][2] =
        {
            {0, 0},
            {last, 0},
            {0, last},
            {last, last}
        };

        for (const auto& sample : samples)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%zu", sample[0]);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%zu", sample[1]);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.3f",
                decoded.heightField.at(
                    sample[0], sample[1]));
            ImGui::TableSetColumnIndex(3);
            ImGui::TextUnformatted("");
            ImGui::TableSetColumnIndex(4);
            ImGui::TextUnformatted("");
        }

        ImGui::EndTable();
    }
}

void MainWindow::drawRecordPanel()
{
    const float leftWidth =
        ImGui::GetContentRegionAvail().x *
        0.35f;

    ImGui::BeginChild(
        "RecordList",
        ImVec2(leftWidth, -35.0f),
        true);

    for (std::size_t i = 0;
         i < plugin_.records.size();
         ++i)
    {
        const Record& record =
            plugin_.records[i];

        char label[96]{};

        std::snprintf(
            label,
            sizeof(label),
            "%s  %08X%s",
            record.type.c_str(),
            record.formId,
            record.compressed
                ? " [compressed]"
                : "");

        if (ImGui::Selectable(
                label,
                selectedRecord_ ==
                    static_cast<int>(i)))
        {
            selectedRecord_ =
                static_cast<int>(i);
        }
    }

    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild(
        "RecordInspector",
        ImVec2(0, -35.0f),
        true);

    if (selectedRecord_ >= 0 &&
        selectedRecord_ <
            static_cast<int>(
                plugin_.records.size()))
    {
        const Record& record =
            plugin_.records[
                static_cast<std::size_t>(
                    selectedRecord_)];

        ImGui::Text(
            "Record: %s",
            record.type.c_str());

        ImGui::Text(
            "Form ID: %08X",
            record.formId);

        ImGui::Text(
            "Flags: %08X",
            record.flags);

        ImGui::Text(
            "File offset: 0x%zX",
            record.headerOffset);

        ImGui::Text(
            "Subrecords: %zu",
            record.subRecords.size());

        ImGui::Text(
            "Compressed: %s",
            record.compressed
                ? "yes"
                : "no");

        ImGui::Separator();

        ImGui::TextUnformatted(
            "Subrecords");

        if (record.compressed)
        {
            ImGui::TextWrapped(
                "This record is compressed. Its original "
                "compressed payload is retained, but its "
                "subrecords are not yet decompressed.");
        }
        else
        {
            for (const SubRecord& sub :
                 record.subRecords)
            {
                ImGui::BulletText(
                    "%s  (%zu bytes, offset 0x%zX)",
                    sub.type.c_str(),
                    sub.size,
                    sub.offset);
            }
        }
    }
    else
    {
        ImGui::TextWrapped(
            "Select a record to inspect it.");
    }

    ImGui::EndChild();
}
