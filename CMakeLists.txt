cmake_minimum_required(VERSION 3.20)
project(Wildstar_Studio_2 VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(CMAKE_WARN_DEPRECATED OFF)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON)

include(FetchContent)

FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
        glad
        GIT_REPOSITORY https://github.com/libigl/libigl-glad.git
        GIT_TAG        09b4969
)
cmake_policy(SET CMP0169 OLD)
FetchContent_GetProperties(glad)
if(NOT glad_POPULATED)
    FetchContent_Populate(glad)
endif()

if(NOT TARGET glad)
    add_library(glad STATIC "${glad_SOURCE_DIR}/src/glad.c")
    target_include_directories(glad PUBLIC "${glad_SOURCE_DIR}/include")
endif()

FetchContent_Declare(
        zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG        v1.3
)
FetchContent_MakeAvailable(zlib)

FetchContent_Declare(
        liblzma
        GIT_REPOSITORY https://github.com/xz-mirror/xz.git
        GIT_TAG        v5.4.4
)
FetchContent_MakeAvailable(liblzma)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.3.8
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        docking
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
        ImGuiFileDialog
        GIT_REPOSITORY https://github.com/aiekick/ImGuiFileDialog.git
        GIT_TAG        v0.6.7
)
FetchContent_MakeAvailable(ImGuiFileDialog)

FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG        master
)
FetchContent_MakeAvailable(stb)

set(IMGUI_DIR ${imgui_SOURCE_DIR})
add_library(imgui STATIC
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw glad)

add_library(stb_lib INTERFACE)
target_include_directories(stb_lib INTERFACE ${stb_SOURCE_DIR})

add_definitions(-DLZMA_API_STATIC)

add_executable(Wildstar_Studio_2
        src/main.cpp
        src/Archive.cpp
        src/Archive.h
        src/UI/UI.cpp
        src/UI/UI.h
        src/UI/splashscreen.cpp
        src/UI/splashscreen.h
        src/About/about.cpp
        src/About/about.h
        src/settings/Settings.cpp
        src/settings/Settings.h
        src/Area/AreaFile.cpp
        src/Area/AreaFile.h
        src/Area/AreaRender.cpp
        src/Area/AreaRender.h
        src/Utils/BinStream.h
        ${ImGuiFileDialog_SOURCE_DIR}/ImGuiFileDialog.cpp
        src/tex/tex.cpp
        src/tex/tex.h
        src/tex/texjpeg.cpp
        src/tex/texjpeg.h
        src/Area/DataTable.cpp
        src/Area/DataTable.h
        src/UI/camera.cpp
        src/UI/design.cpp
        src/models/M3Common.h
        src/models/M3Loader.cpp
        src/models/M3Loader.h
        src/models/M3Render.cpp
        src/models/M3Render.h
        src/UI/UI_Models.cpp
        src/UI/UI_Models.h
        src/UI/UI_Globals.cpp
        src/UI/UI_Globals.h
        src/UI/UI_Utils.cpp
        src/UI/UI_Utils.h
        src/UI/UI_Selection.cpp
        src/UI/UI_Selection.h
        src/UI/UI_RenderWorld.cpp
        src/UI/UI_RenderWorld.h
        src/UI/UI_FileTree.cpp
        src/UI/UI_FileTree.h
        src/UI/UI_AreaTab.cpp
        src/UI/UI_AreaTab.h
        src/UI/UI_ModelTab.cpp
        src/UI/UI_ModelTab.h
)

target_include_directories(Wildstar_Studio_2 PRIVATE
        src/UI
        src/Settings
        src/About
        src/Area
        src/Utils
        ${zlib_SOURCE_DIR}
        ${zlib_BINARY_DIR}
        ${liblzma_SOURCE_DIR}/src/liblzma/api
        ${ImGuiFileDialog_SOURCE_DIR}
)

target_link_libraries(Wildstar_Studio_2 PRIVATE imgui glad glm zlibstatic liblzma stb_lib)

if(WIN32)
    target_link_libraries(Wildstar_Studio_2 PRIVATE opengl32)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(Wildstar_Studio_2 PRIVATE OpenGL::GL)
endif()

add_custom_command(TARGET Wildstar_Studio_2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Assets
        ${CMAKE_CURRENT_BINARY_DIR}/Assets
        COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_CURRENT_BINARY_DIR}/assets
        COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_CURRENT_BINARY_DIR}/assets/fonts
        COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_CURRENT_BINARY_DIR}/assets/icons
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Assets/Fonts
        ${CMAKE_CURRENT_BINARY_DIR}/assets/fonts
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Assets/Icons
        ${CMAKE_CURRENT_BINARY_DIR}/assets/icons
)