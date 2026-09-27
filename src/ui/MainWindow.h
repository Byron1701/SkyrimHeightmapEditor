#pragma once

#include "plugin/Plugin.h"
#include "terrain/TerrainViewport.h"
#include "terrain/HeightField.h"

#include <string>
#include <unordered_map>

class MainWindow
{
public:
    explicit MainWindow(Plugin& plugin);

    void draw();

    bool shouldExit() const
    {
        return exitRequested_;
    }

private:
    void drawMenuBar();
    void drawPluginPanel();
    void drawRecordPanel();
    void drawTerrainPanel();
    void rebuildTerrainCache();
    void drawWorldIndexPanel();
    void openPlugin();

    Plugin& plugin_;

    char pathBuffer_[1024]{};

    std::string status_ =
        "No plugin loaded.";

    int selectedRecord_ = -1;
    int selectedWorldspace_ = -1;
    int selectedCell_ = -1;
    bool exitRequested_ = false;
    bool terrainWireframe_ = false;
    TerrainViewport terrainViewport_;
    std::unordered_map<std::size_t, HeightField> terrainCache_;
};
