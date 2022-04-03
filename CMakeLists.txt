cmake_minimum_required(VERSION 3.17)
project(wkTerrainSync
        VERSION "1.2.5.0"
        DESCRIPTION "wkTerrain with automatic data synchronization and extra features"
        )

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(lib/hacklib-patternscanner)

set(POLYHOOK_BUILD_STATIC_RUNTIME)
add_subdirectory(lib/PolyHook_2_0)
include_directories(include lib/hacklib-patternscanner lib/PolyHook_2_0 lib/PolyHook_2_0/capstone/include)
link_directories(lib)

set(PRE_CONFIGURE_FILE "cmake/git.cc.in")
set(POST_CONFIGURE_FILE "${CMAKE_CURRENT_BINARY_DIR}/gen/git.cc")
include(cmake/git_watcher.cmake)

add_library(git STATIC ${POST_CONFIGURE_FILE})
target_include_directories(git PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} cmake)

add_dependencies(git check_git)
configure_file(cmake/git.h ${CMAKE_CURRENT_BINARY_DIR}/gen/git.h)

configure_file(resource.rc.in resource.rc)

configure_file(cmake/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/gen/version.h)
add_definitions(-DLIBRARY_EXPORTS)
add_library(${PROJECT_NAME} SHARED
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
add_dependencies(${PROJECT_NAME} PolyHook_2)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/gen)
set_property(TARGET ${PROJECT_NAME} hacklib-patternscanner git PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_link_libraries(${PROJECT_NAME} hacklib-patternscanner PolyHook_2 version git libz-static)
