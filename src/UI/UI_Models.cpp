#define NOMINMAX
#include "UI_Models.h"
#include "imgui.h"
#include "../models/M3Render.h"
#include "../models/M3Common.h"
#include "../export/M3Export.h"
#include "UI.h"
#include "UI_Globals.h"
#include "ImGuiFileDialog.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <d3d11.h>
#include <DirectXMath.h>
#include <algorithm>
#include <set>
#include <thread>
#include <atomic>
#include <mutex>

namespace UI_Models
{
    static std::string FormatModelName(const std::string& name)
    {
        std::string result = name;
        if (result.size() > 3 && result.substr(result.size() - 3) == ".m3")
            result = result.substr(0, result.size() - 3);
        std::replace(result.begin(), result.end(), '_', ' ');
        return result;
    }

    static int selectedTextureIndex = -1;
    static bool showTexturePopup = false;
    static bool showUVs = false;

    static bool sExportingFBX = false;
    static bool sExportingGLB = false;
    static std::atomic<bool> sExportInProgress{false};
    static std::atomic<int> sExportProgress{0};
    static std::atomic<int> sExportTotal{100};
    static std::string sExportStatus;
    static std::mutex sExportMutex;
    static M3Export::ExportResult sExportResult;
    static bool sShowExportResult = false;
    static float sNotificationTimer = 0.0f;
    static std::string sNotificationMessage;
    static bool sNotificationSuccess = true;

    static void HandleModelPicking(AppState& state, M3Render* render)
    {
        if (!render) return;
        if (ImGui::GetIO().WantCaptureMouse) return;
        if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) return;

        ImVec2 mousePos = ImGui::GetMousePos();
        ImGuiViewport* vp = ImGui::GetMainViewport();

        float ndcX = (2.0f * (mousePos.x - vp->Pos.x) / vp->Size.x) - 1.0f;
        float ndcY = 1.0f - (2.0f * (mousePos.y - vp->Pos.y) / vp->Size.y);

        glm::mat4 invProj = glm::inverse(gProjMatrix);
        glm::mat4 invView = glm::inverse(gViewMatrix);

        glm::vec4 rayClip(ndcX, ndcY, -1.0f, 1.0f);
        glm::vec4 rayEye = invProj * rayClip;
        rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
        glm::vec3 rayWorld = glm::normalize(glm::vec3(invView * rayEye));
        glm::vec3 rayOrigin = glm::vec3(invView[3]);

        DirectX::XMFLOAT3 dxRayOrigin(rayOrigin.x, rayOrigin.y, rayOrigin.z);
        DirectX::XMFLOAT3 dxRayDir(rayWorld.x, rayWorld.y, rayWorld.z);
        int hit = render->rayPickSubmesh(dxRayOrigin, dxRayDir);
        render->setSelectedSubmesh(hit);

        if (hit >= 0)
        {
            const auto& submeshes = render->getAllSubmeshes();
            const auto& materials = render->getAllMaterials();

            if ((size_t)hit < submeshes.size())
            {
                uint16_t matId = submeshes[hit].materialID;
                if (matId < materials.size() && !materials[matId].variants.empty())
                {
                    int varIdx = render->getMaterialSelectedVariant(matId);
                    if (varIdx < 0 || varIdx >= (int)materials[matId].variants.size())
                        varIdx = 0;
                    int texIdx = materials[matId].variants[varIdx].textureIndexA;
                    if (texIdx >= 0)
                    {
                        selectedTextureIndex = texIdx;
                        showTexturePopup = true;
                        showUVs = true;
                    }
                }
            }
        }
    }

    void Draw(AppState& state)
    {
        if (!state.show_models_window) return;

        M3Render* render = state.m3Render.get();
        if (!render) return;

        HandleModelPicking(state, render);

        std::string windowTitle = "Model";
        if (!render->getModelName().empty())
            windowTitle = FormatModelName(render->getModelName());

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float windowWidth = 300.0f;
        float windowX = viewport->Pos.x + viewport->Size.x - windowWidth - 10.0f;
        float windowY = viewport->Pos.y + 40.0f;
        float windowHeight = viewport->Pos.y + viewport->Size.y - windowY - 10.0f;

        ImGui::SetNextWindowPos(ImVec2(windowX, windowY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.9f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

        if (ImGui::Begin(windowTitle.c_str(), &state.show_models_window, flags))
        {
            float exportHeight = 35.0f;
            float childHeight = ImGui::GetContentRegionAvail().y - exportHeight;

            ImGui::BeginChild("##ModelContent", ImVec2(0, childHeight), false);

            size_t submeshCount = render->getSubmeshCount();
            size_t materialCount = render->getMaterialCount();
            size_t boneCount = render->getBoneCount();
            size_t animCount = render->getAnimationCount();
            size_t texCount = render->getAllTextures().size();

            ImGui::Text("Submeshes: %zu  Materials: %zu", submeshCount, materialCount);
            ImGui::Text("Bones: %zu  Animations: %zu  Textures: %zu", boneCount, animCount, texCount);

            ImGui::Separator();

            bool showSkel = render->getShowSkeleton();
            if (ImGui::Checkbox("Show Skeleton", &showSkel))
                render->setShowSkeleton(showSkel);

            std::set<uint8_t> uniqueGroups;
            for (size_t i = 0; i < submeshCount; ++i)
            {
                uint8_t gid = render->getSubmesh(i).groupId;
                if (gid != 255) uniqueGroups.insert(gid);
            }

            if (uniqueGroups.size() > 1)
            {
                ImGui::Separator();
                ImGui::Text("Variant:");
                int currentVariant = render->getActiveVariant();
                char preview[64];
                if (currentVariant == -1) snprintf(preview, sizeof(preview), "All");
                else snprintf(preview, sizeof(preview), "%d", currentVariant);

                if (ImGui::BeginCombo("##variant_combo", preview))
                {
                    bool selAll = (currentVariant == -1);
                    if (ImGui::Selectable("All", selAll))
                    {
                        render->setActiveVariant(-1);
                        for (size_t i = 0; i < submeshCount; ++i)
                            render->setSubmeshVisible(i, true);
                    }

                    for (uint8_t gid : uniqueGroups)
                    {
                        bool sel = (currentVariant == (int)gid);
                        char label[32];
                        snprintf(label, sizeof(label), "%d", gid);
                        if (ImGui::Selectable(label, sel))
                            render->setActiveVariant((int)gid);
                    }

                    ImGui::EndCombo();
                }
                if (ImGui::CollapsingHeader("Animations"))
                {
                    const auto& anims = render->getAllAnimations();
                    int playingIdx = render->getPlayingAnimation();

                    for (size_t i = 0; i < anims.size(); ++i)
                    {
                        const auto& anim = anims[i];
                        bool isPlaying = (playingIdx == (int)i);
                        float duration = (anim.timestampEnd - anim.timestampStart) / 1000.0f;

                        char label[64];
                        snprintf(label, sizeof(label), "%s Anim %zu (%.2fs)###anim%zu", isPlaying ? ">" : " ", i, duration, i);

                        if (isPlaying) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
                        if (ImGui::Selectable(label, isPlaying))
                        {
                            if (isPlaying) render->stopAnimation();
                            else render->playAnimation((int)i);
                        }
                        if (isPlaying) ImGui::PopStyleColor();
                    }
                }

                if (ImGui::CollapsingHeader("Submeshes"))
                {
                    for (size_t i = 0; i < submeshCount; ++i)
                    {
                        const auto& sm = render->getSubmesh(i);
                        ImGui::PushID(static_cast<int>(i));

                        bool visible = render->getSubmeshVisible(i);
                        if (ImGui::Checkbox("##vis", &visible))
                            render->setSubmeshVisible(i, visible);
                        ImGui::SameLine();

                        if (ImGui::TreeNode((void*)(intptr_t)i, "Submesh %zu (Group %d)", i, sm.groupId))
                        {
                            ImGui::Text("Material: %d  Indices: %u  Vertices: %u", sm.materialID, sm.indexCount, sm.vertexCount);
                            ImGui::TreePop();
                        }
                        ImGui::PopID();
                    }
                }

                if (ImGui::CollapsingHeader("Materials"))
                {
                    for (size_t i = 0; i < materialCount; ++i)
                    {
                        ImGui::PushID(static_cast<int>(i));
                        if (ImGui::TreeNode((void*)(intptr_t)i, "Material %zu", i))
                        {
                            int currentVar = render->getMaterialSelectedVariant(i);
                            int variantCount = (int)render->getMaterialVariantCount(i);
                            if (variantCount > 1)
                            {
                                if (ImGui::SliderInt("Variant", &currentVar, 0, variantCount - 1))
                                    render->setMaterialSelectedVariant(i, currentVar);
                            }
                            ImGui::Text("Variants: %d", variantCount);
                            ImGui::TreePop();
                        }
                        ImGui::PopID();
                    }
                }

                if (ImGui::CollapsingHeader("Textures"))
                {
                    const auto& textures = render->getAllTextures();
                    const auto& srvTextures = render->getGLTextures();
                    float thumbSize = 48.0f;
                    float windowWidth = ImGui::GetContentRegionAvail().x;
                    int columns = std::max(1, (int)(windowWidth / (thumbSize + 8.0f)));

                    for (size_t i = 0; i < textures.size(); ++i)
                    {
                        const auto& tex = textures[i];
                        ImGui::PushID(static_cast<int>(i));
                        ID3D11ShaderResourceView* srv = (i < srvTextures.size()) ? srvTextures[i].Get() : nullptr;

                        if (srv != nullptr)
                        {
                            if (ImGui::ImageButton("##texbtn", reinterpret_cast<ImTextureID>(srv), ImVec2(thumbSize, thumbSize)))
                            {
                                selectedTextureIndex = static_cast<int>(i);
                                showTexturePopup = true;
                            }
                            if (ImGui::IsItemHovered())
                            {
                                ImGui::BeginTooltip();
                                ImGui::Text("[%zu] %s", i, tex.path.c_str());
                                ImGui::EndTooltip();
                            }
                        }
                        else
                        {
                            ImGui::Button("N/A", ImVec2(thumbSize, thumbSize));
                        }

                        if ((i + 1) % columns != 0 && i + 1 < textures.size())
                            ImGui::SameLine();
                        ImGui::PopID();
                    }
                }

                if (ImGui::CollapsingHeader("Bones"))
                {
                    const auto& bones = render->getAllBones();
                    int selectedBone = render->getSelectedBone();

                    if (selectedBone >= 0)
                    {
                        if (ImGui::Button("Deselect"))
                            render->setSelectedBone(-1);
                        ImGui::Separator();
                    }

                    for (size_t i = 0; i < bones.size(); ++i)
                    {
                        const auto& bone = bones[i];
                        ImGui::PushID(static_cast<int>(i));

                        bool isSelected = (selectedBone == static_cast<int>(i));

                        if (isSelected)
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));

                        char label[128];
                        snprintf(label, sizeof(label), "[%d] %s", bone.id, bone.name.c_str());

                        if (ImGui::Selectable(label, isSelected))
                        {
                            render->setSelectedBone(isSelected ? -1 : static_cast<int>(i));
                        }

                        if (isSelected)
                            ImGui::PopStyleColor();

                        if (ImGui::IsItemHovered())
                        {
                            ImGui::BeginTooltip();
                            ImGui::Text("Parent: %d", bone.parentId);
                            ImGui::Text("Flags: 0x%04X", bone.flags);
                            ImGui::EndTooltip();
                        }

                        ImGui::PopID();
                    }
                }
                ImGui::EndChild();

                ImGui::Separator();
                bool exportDisabled = sExportInProgress.load();
                if (exportDisabled) ImGui::BeginDisabled();

                if (ImGui::Button("Export FBX", ImVec2(130, 0)))
                {
                    std::string suggestedName = M3Export::GetSuggestedFilename(render) + ".fbx";
                    IGFD::FileDialogConfig config;
                    config.path = ".";
                    config.fileName = suggestedName;
                    config.flags = ImGuiFileDialogFlags_ConfirmOverwrite;
                    ImGuiFileDialog::Instance()->OpenDialog("ExportFBXDlg", "Export FBX", ".fbx", config);
                    sExportingFBX = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Export GLB", ImVec2(130, 0)))
                {
                    std::string suggestedName = M3Export::GetSuggestedFilename(render) + ".glb";
                    IGFD::FileDialogConfig config;
                    config.path = ".";
                    config.fileName = suggestedName;
                    config.flags = ImGuiFileDialogFlags_ConfirmOverwrite;
                    ImGuiFileDialog::Instance()->OpenDialog("ExportGLBDlg", "Export GLB", ".glb", config);
                    sExportingGLB = true;
                }

                if (exportDisabled) ImGui::EndDisabled();
            }
            ImGui::End();
            ImGui::PopStyleVar();

            if (showTexturePopup && selectedTextureIndex >= 0)
            {
                const auto& textures = render->getAllTextures();
                const auto& srvTextures = render->getGLTextures();

                if (selectedTextureIndex < (int)textures.size())
                {
                    const auto& tex = textures[selectedTextureIndex];
                    std::string popupTitle = tex.path.empty() ? "Texture " + std::to_string(selectedTextureIndex) : tex.path;

                    ImGui::SetNextWindowSize(ImVec2(520, 580), ImGuiCond_FirstUseEver);
                    if (ImGui::Begin(popupTitle.c_str(), &showTexturePopup))
                    {
                        ImGui::Checkbox("Show UVs", &showUVs);
                        ImGui::Separator();

                        ID3D11ShaderResourceView* srv = (selectedTextureIndex < (int)srvTextures.size()) ? srvTextures[selectedTextureIndex].Get() : nullptr;
                        if (srv != nullptr)
                        {
                            ImVec2 avail = ImGui::GetContentRegionAvail();
                            float maxDim = std::min(avail.x, avail.y - 60.0f);

                            ImVec2 imgPos = ImGui::GetCursorScreenPos();
                            ImGui::Image(reinterpret_cast<ImTextureID>(srv), ImVec2(maxDim, maxDim));

                            if (showUVs)
                            {
                                ImDrawList* drawList = ImGui::GetWindowDrawList();
                                const auto& vertices = render->getVertices();
                                const auto& indices = render->getIndices();
                                const auto& submeshes = render->getAllSubmeshes();
                                const auto& materials = render->getAllMaterials();

                                ImU32 uvColor = IM_COL32(0, 255, 0, 180);

                                for (size_t si = 0; si < submeshes.size(); ++si)
                                {
                                    if (!render->getSubmeshVisible(si)) continue;

                                    const auto& sm = submeshes[si];

                                    if (sm.materialID >= materials.size()) continue;
                                    const auto& mat = materials[sm.materialID];
                                    if (mat.variants.empty()) continue;

                                    int varIdx = render->getMaterialSelectedVariant(sm.materialID);
                                    if (varIdx < 0 || varIdx >= (int)mat.variants.size()) varIdx = 0;
                                    const auto& variant = mat.variants[varIdx];

                                    if (variant.textureIndexA != selectedTextureIndex &&
                                        variant.textureIndexB != selectedTextureIndex)
                                        continue;

                                    for (uint32_t i = 0; i + 2 < sm.indexCount; i += 3)
                                    {
                                        uint32_t i0 = indices[sm.startIndex + i] + sm.startVertex;
                                        uint32_t i1 = indices[sm.startIndex + i + 1] + sm.startVertex;
                                        uint32_t i2 = indices[sm.startIndex + i + 2] + sm.startVertex;

                                        if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size())
                                            continue;

                                        const auto& v0 = vertices[i0];
                                        const auto& v1 = vertices[i1];
                                        const auto& v2 = vertices[i2];

                                        ImVec2 p0(imgPos.x + v0.uv1.x * maxDim, imgPos.y + (1.0f - v0.uv1.y) * maxDim);
                                        ImVec2 p1(imgPos.x + v1.uv1.x * maxDim, imgPos.y + (1.0f - v1.uv1.y) * maxDim);
                                        ImVec2 p2(imgPos.x + v2.uv1.x * maxDim, imgPos.y + (1.0f - v2.uv1.y) * maxDim);

                                        drawList->AddLine(p0, p1, uvColor, 1.0f);
                                        drawList->AddLine(p1, p2, uvColor, 1.0f);
                                        drawList->AddLine(p2, p0, uvColor, 1.0f);
                                    }
                                }
                            }
                        }
                        ImGui::Separator();
                        ImGui::Text("Path: %s", tex.path.c_str());
                        ImGui::Text("Type: %s (%d)", tex.textureType.c_str(), tex.type);
                    }
                    ImGui::End();

                    if (!showTexturePopup)
                        selectedTextureIndex = -1;
                }
                else
                {
                    showTexturePopup = false;
                    selectedTextureIndex = -1;
                }
            }

            if (render->isAnimationPlaying())
            {
                ImGuiViewport* vp = ImGui::GetMainViewport();
                float modelInfoX = vp->Pos.x + vp->Size.x - 300.0f - 10.0f;
                float playbackWidth = 180.0f;
                float popupX = modelInfoX - playbackWidth - 10.0f;
                float popupY = vp->Pos.y + 40.0f;

                ImGui::SetNextWindowPos(ImVec2(popupX, popupY), ImGuiCond_FirstUseEver);
                ImGui::SetNextWindowBgAlpha(0.9f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);

                ImGuiWindowFlags popupFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;

                bool showPlayback = true;
                if (ImGui::Begin("Playback", &showPlayback, popupFlags))
                {
                    float duration = render->getAnimationDuration();
                    float currentTime = render->getAnimationTime();
                    bool isPaused = render->isAnimationPaused();

                    ImGui::ProgressBar(duration > 0 ? currentTime / duration : 0, ImVec2(160, 14));

                    float btnWidth = 50.0f;
                    if (isPaused)
                    {
                        if (ImGui::Button("Play", ImVec2(btnWidth, 0)))
                            render->resumeAnimation();
                    }
                    else
                    {
                        if (ImGui::Button("Pause", ImVec2(btnWidth, 0)))
                            render->pauseAnimation();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Stop", ImVec2(btnWidth, 0)))
                        render->stopAnimation();
                }
                ImGui::End();
                ImGui::PopStyleVar();

                if (!showPlayback)
                    render->stopAnimation();
            }

            if (ImGuiFileDialog::Instance()->Display("ExportFBXDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
            {
                if (ImGuiFileDialog::Instance()->IsOk())
                {
                    std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
                    std::string dirPath = ImGuiFileDialog::Instance()->GetCurrentPath();
                    std::string fileName = ImGuiFileDialog::Instance()->GetCurrentFileName();

                    size_t extPos = fileName.rfind('.');
                    std::string exportName = (extPos != std::string::npos) ? fileName.substr(0, extPos) : fileName;

                    sExportInProgress = true;
                    sExportProgress = 0;
                    sExportTotal = 100;
                    sExportStatus = "Starting export...";

                    std::thread([render, dirPath, exportName]() {
                        M3Export::ExportSettings settings;
                        settings.outputPath = dirPath;
                        settings.customName = exportName;
                        settings.activeVariant = render->getActiveVariant();
                        settings.exportTextures = true;
                        settings.exportAnimations = true;
                        settings.exportSkeleton = true;

                        auto result = M3Export::ExportToFBX(render, gPendingModelArchive, settings,
                            [](int cur, int total, const std::string& status) {
                                sExportProgress = cur;
                                sExportTotal = total;
                                std::lock_guard<std::mutex> lock(sExportMutex);
                                sExportStatus = status;
                            });

                        {
                            std::lock_guard<std::mutex> lock(sExportMutex);
                            sExportResult = result;
                        }
                        sExportInProgress = false;
                        sShowExportResult = true;
                    }).detach();
                }
                ImGuiFileDialog::Instance()->Close();
                sExportingFBX = false;
            }

            if (ImGuiFileDialog::Instance()->Display("ExportGLBDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
            {
                if (ImGuiFileDialog::Instance()->IsOk())
                {
                    std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
                    std::string dirPath = ImGuiFileDialog::Instance()->GetCurrentPath();
                    std::string fileName = ImGuiFileDialog::Instance()->GetCurrentFileName();

                    size_t extPos = fileName.rfind('.');
                    std::string exportName = (extPos != std::string::npos) ? fileName.substr(0, extPos) : fileName;

                    sExportInProgress = true;
                    sExportProgress = 0;
                    sExportTotal = 100;
                    sExportStatus = "Starting export...";

                    std::thread([render, dirPath, exportName]() {
                        M3Export::ExportSettings settings;
                        settings.outputPath = dirPath;
                        settings.customName = exportName;
                        settings.activeVariant = render->getActiveVariant();
                        settings.exportTextures = true;
                        settings.exportAnimations = true;
                        settings.exportSkeleton = true;

                        auto result = M3Export::ExportToGLB(render, gPendingModelArchive, settings,
                            [](int cur, int total, const std::string& status) {
                                sExportProgress = cur;
                                sExportTotal = total;
                                std::lock_guard<std::mutex> lock(sExportMutex);
                                sExportStatus = status;
                            });

                        {
                            std::lock_guard<std::mutex> lock(sExportMutex);
                            sExportResult = result;
                        }
                        sExportInProgress = false;
                        sShowExportResult = true;
                    }).detach();
                }
                ImGuiFileDialog::Instance()->Close();
                sExportingGLB = false;
            }

            if (sExportInProgress.load())
            {
                ImGuiViewport* vp = ImGui::GetMainViewport();
                ImVec2 center(vp->Pos.x + vp->Size.x * 0.5f, vp->Pos.y + vp->Size.y * 0.5f);
                ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                ImGui::SetNextWindowBgAlpha(0.95f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);

                ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

                if (ImGui::Begin("##ExportProgress", nullptr, flags))
                {
                    std::string status;
                    {
                        std::lock_guard<std::mutex> lock(sExportMutex);
                        status = sExportStatus;
                    }

                    ImGui::Text("Exporting...");
                    ImGui::Text("%s", status.c_str());
                    float progress = sExportTotal > 0 ? (float)sExportProgress.load() / (float)sExportTotal : 0.0f;
                    ImGui::ProgressBar(progress, ImVec2(250, 20));
                }
                ImGui::End();
                ImGui::PopStyleVar();
            }

            if (sShowExportResult)
            {
                M3Export::ExportResult result;
                {
                    std::lock_guard<std::mutex> lock(sExportMutex);
                    result = sExportResult;
                }
                sNotificationSuccess = result.success;
                sNotificationMessage = result.success ? "Export successful!" : ("Export failed: " + result.errorMessage);
                sNotificationTimer = 3.0f;
                sShowExportResult = false;
            }

            if (sNotificationTimer > 0.0f)
            {
                sNotificationTimer -= ImGui::GetIO().DeltaTime;
                float alpha = std::min(1.0f, sNotificationTimer);

                ImGuiViewport* vp = ImGui::GetMainViewport();
                ImVec2 center(vp->Pos.x + vp->Size.x * 0.5f, vp->Pos.y + 60.0f);

                ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.0f));
                ImGui::SetNextWindowBgAlpha(0.85f * alpha);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

                ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize;

                if (ImGui::Begin("##ExportNotification", nullptr, flags))
                {
                    if (sNotificationSuccess)
                        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%s", sNotificationMessage.c_str());
                    else
                        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", sNotificationMessage.c_str());
                }
                ImGui::End();
                ImGui::PopStyleVar(2);
            }
        }
    }
}