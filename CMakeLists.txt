cmake_minimum_required(VERSION 3.20)
project(The_Ham_Sandwich VERSION 1.0.0 LANGUAGES CXX C)

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

add_executable(The_Ham_Sandwich
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
        src/Utils/ParallelAreaLoader.cpp
        src/Utils/ParallelAreaLoader.h
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
        src/Area/TerrainTexture.cpp
        src/Area/TerrainTexture.h
        src/Area/TerrainShader.cpp
        src/Area/TerrainShader.h
        src/UI/UI_Tables.cpp
        src/UI/UI_Tables.h
        src/UI/UI_AreaInfo.cpp
        src/UI/UI_AreaInfo.h
        src/export/FBXExport.cpp
        src/export/FBXExport.h
        src/Database/Tbl.cpp
        src/Database/Tbl.h
        src/Database/TblReader.h
        src/Database/TblRecords.h
        src/Database/Definitions/ZoneCompletion.h
        src/Database/Definitions/XpPerLevel.h
        src/Database/Definitions/VisualEffect.h
        src/Database/Definitions/WorldZone.h
        src/Database/Definitions/WorldWaterWake.h
        src/Database/Definitions/WorldWaterType.h
        src/Database/Definitions/WorldWaterLayer.h
        src/Database/Definitions/WorldWaterFog.h
        src/Database/Definitions/WorldWaterEnvironment.h
        src/Database/Definitions/WorldSocket.h
        src/Database/Definitions/WorldSky.h
        src/Database/Definitions/WorldLocation2.h
        src/Database/Definitions/WorldLayer.h
        src/Database/Definitions/WorldClutter.h
        src/Database/Definitions/World.h
        src/Database/Definitions/WordFilterAlt.h
        src/Database/Definitions/WorldFilter.h
        src/Database/Definitions/Wind.h
        src/Database/Definitions/WaterSurfaceEffect.h
        src/Database/Definitions/WindSpawn.h
        src/Database/Definitions/TradeskillBonus.h
        src/Database/Definitions/TradeskillCatalyst.h
        src/Database/Definitions/TradeskillCatalystOrdering.h
        src/Database/Definitions/TradeskillHarvestingInfo.h
        src/Database/Definitions/TradeskillMaterial.h
        src/Database/Definitions/TradeskillMaterialCategory.h
        src/Database/Definitions/TradeskillProficiency.h
        src/Database/Definitions/TradeskillSchematic2.h
        src/Database/Definitions/TradeskillTalentTier.h
        src/Database/Definitions/TradeskillTier.h
        src/Database/Definitions/Tutorial.h
        src/Database/Definitions/TutorialAnchor.h
        src/Database/Definitions/TutorialLayout.h
        src/Database/Definitions/TutorialPage.h
        src/Database/Definitions/UnitProperty2.h
        src/Database/Definitions/UnitRace.h
        src/Database/Definitions/UnitVehicle.h
        src/Database/Definitions/VeteranTier.h
        src/Database/Definitions/VirtualItem.h
        src/Database/Definitions/Vital.h
        src/Database/Definitions/SpellLevel.h
        src/Database/Definitions/SpellPhase.h
        src/Database/Definitions/Spline2.h
        src/Database/Definitions/Spline2Node.h
        src/Database/Definitions/StoreDisplayInfo.h
        src/Database/Definitions/StoreKeyword.h
        src/Database/Definitions/StoreLink.h
        src/Database/Definitions/StoryPanel.h
        src/Database/Definitions/TargetGroup.h
        src/Database/Definitions/TargetMarker.h
        src/Database/Definitions/TaxiNode.h
        src/Database/Definitions/TaxiRoute.h
        src/Database/Definitions/TelegraphDamage.h
        src/Database/Definitions/TicketCategory.h
        src/Database/Definitions/TicketSubCategory.h
        src/Database/Definitions/TrackingSlot.h
        src/Database/Definitions/Tradeskill.h
        src/Database/Definitions/TradeskillAchievementLayout.h
        src/Database/Definitions/TradeskillAchievementAward.h
        src/Database/Definitions/TradeskillAchievementReward.h
        src/Database/Definitions/TradeskillAdditive.h
        src/Database/Definitions/Spell4Thresholds.h
        src/Database/Definitions/Spell4TierRequirements.h
        src/Database/Definitions/Spell4ValidTargets.h
        src/Database/Definitions/Spell4Visual.h
        src/Database/Definitions/Spell4VisualGroup.h
        src/Database/Definitions/SpellCooldown.h
        src/Database/Definitions/SpellEffectType.h
        src/Database/Definitions/Spell4GroupList.h
        src/Database/Definitions/Spell4HitResults.h
        src/Database/Definitions/Spell4Modification.h
        src/Database/Definitions/Spell4Prerequisites.h
        src/Database/Definitions/Spell4Reagent.h
        src/Database/Definitions/Spell4Runner.h
        src/Database/Definitions/Spell4ServiceTokenCost.h
        src/Database/Definitions/Spell4SpellTypes.h
        src/Database/Definitions/Spell4StackGroup.h
        src/Database/Definitions/Spell4Tag.h
        src/Database/Definitions/Spell4TargetAngle.h
        src/Database/Definitions/Spell4TargetMechanics.h
        src/Database/Definitions/Spell4Telegraph.h
        src/Database/Definitions/Spell4.h
        src/Database/Definitions/SoundImpactEvents.h
        src/Database/Definitions/SoundMusicSet.h
        src/Database/Definitions/SoundParameter.h
        src/Database/Definitions/SoundReplace.h
        src/Database/Definitions/SoundReplaceDescription.h
        src/Database/Definitions/SoundStates.h
        src/Database/Definitions/SoundSwitch.h
        src/Database/Definitions/SoundUIContext.h
        src/Database/Definitions/SoundZoneKit.h
        src/Database/Definitions/RewardTrackRewards.h
        src/Database/Definitions/Salvage.h
        src/Database/Definitions/SalvageException.h
        src/Database/Definitions/SkyCloudSet.h
        src/Database/Definitions/SoundBank.h
        src/Database/Definitions/SoundCombatLoop.h
        src/Database/Definitions/SoundContext.h
        src/Database/Definitions/SoundDirectionalAmbience.h
        src/Database/Definitions/SoundEnvironment.h
        src/Database/Definitions/SoundEvent.h
        src/UI/UI_ChunkTextures.cpp
        src/UI/UI_ChunkTextures.h
        src/export/GLBExport.cpp
        src/export/GLBExport.h
        src/export/TerrainExport.cpp
        src/export/TerrainExport.h
        src/export/M3Export.cpp
        src/export/M3Export.h
        src/resource.h
        src/resources.rc
)

target_include_directories(The_Ham_Sandwich PRIVATE
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

target_link_libraries(The_Ham_Sandwich PRIVATE imgui glad glm zlibstatic liblzma stb_lib)

if(WIN32)
    target_link_libraries(The_Ham_Sandwich PRIVATE opengl32)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(The_Ham_Sandwich PRIVATE OpenGL::GL)
endif()

add_custom_command(TARGET The_Ham_Sandwich POST_BUILD
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