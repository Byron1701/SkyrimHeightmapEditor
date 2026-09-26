# SkyrimHeightmapEditor

A lightweight Skyrim landscape editor.

The first iteration deliberately concentrates on the file/data architecture and a read-only ESP/ESM parser. Terrain editing will be added after the plugin representation has been validated against real Skyrim files.

## Current functionality

- C++17/CMake application.
- Dear ImGui + GLFW/OpenGL desktop UI.
- Skyrim ESP/ESM record-header parsing.
- Subrecord parsing, including `XXXX` extended subrecords.
- Record flags, Form IDs and file offsets.
- Detection of compressed records.
- Basic WRLD/CELL/LAND record statistics.
- Separate `HeightField` abstraction for future terrain editing.
- No modification of source plugins yet.

## Build

Requirements:

- CMake 3.20 or newer.
- C++17 compiler.
- Git.

Configure:

    cmake -S . -B build

Build:

    cmake --build build --config Release

GLFW and Dear ImGui are downloaded by CMake using FetchContent.

## Architecture

    UI
      |
      v
    App
      |
      +--> Plugin
      |      |
      |      +--> EspReader
      |      |
      |      +--> Record / SubRecord
      |
      +--> Skyrim data types
      |
      +--> Terrain / HeightField

The terrain representation is deliberately independent of Skyrim's binary serialization. This should allow the eventual LAND decoder/editor to operate on a generic heightfield before converting changes back into LAND records.
