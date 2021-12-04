cmake_minimum_required(VERSION 3.17)
project(wkTerrainSync)

set(CMAKE_CXX_STANDARD 20)

add_library(hacklib STATIC
        lib/hacklib/ExeFile.cpp lib/hacklib/ExeFile_WIN32.cpp
        lib/hacklib/PatternScanner.cpp lib/hacklib/Memory_WIN32.cpp)

link_directories(lib)
include_directories(include lib/hacklib)

configure_file(
        resource.rc.in
        resource.rc
)
add_definitions(-DLIBRARY_EXPORTS)

add_library(wkTerrainSync SHARED
        src/dllmain.cpp
        src/Config.cpp src/Config.h
        src/WaLibc.cpp src/WaLibc.h
        src/DevConsole.cpp src/DevConsole.h
        src/Hooks.cpp src/Hooks.h
        src/TerrainList.cpp src/TerrainList.h
        src/Packets.cpp src/Packets.h
        src/LobbyChat.cpp src/LobbyChat.h
        src/Protocol.cpp src/Protocol.h
        src/Replay.cpp src/Replay.h
        src/Frontend.cpp src/Frontend.h
        src/BitmapImage.cpp src/BitmapImage.h
        src/W2App.cpp src/W2App.h
        src/Sprites.cpp src/Sprites.h
        src/Utils.cpp src/Utils.h
        src/MapGenerator.cpp src/MapGenerator.h
        src/FrontendDialogs.cpp src/FrontendDialogs.h
        src/Missions.cpp src/Missions.h
        src/Scheme.cpp src/Scheme.h
        src/Exports.cpp src/Exports.h
        src/Debugf.h
        src/Water.cpp src/Water.h
        src/Threads.cpp src/Threads.h
        ${CMAKE_CURRENT_BINARY_DIR}/resource.rc
)

set_property(TARGET wkTerrainSync hacklib PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(wkTerrainSync hacklib capstone Polyhook_2 version)
