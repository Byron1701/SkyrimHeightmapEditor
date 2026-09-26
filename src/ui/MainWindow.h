#pragma once

#include "plugin/Plugin.h"

#include <string>

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
    void openPlugin();

    Plugin& plugin_;

    char pathBuffer_[1024]{};

    std::string status_ =
        "No plugin loaded.";

    int selectedRecord_ = -1;
    bool exitRequested_ = false;
};
