#include "M3Render.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <codecvt>
#include <locale>
#include <algorithm>
#include <cmath>
#include <set>

const char* m3VertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aBoneWeights;
layout (location = 4) in uvec4 aBoneIndices;
uniform mat4 model, view, projection;
uniform mat4 boneMatrices[200];
uniform int useSkinning;
out vec3 Normal;
out vec2 TexCoord;
void main() {
    vec4 pos = vec4(aPos, 1.0);
    vec3 norm = aNormal;
    if (useSkinning == 1) {
        mat4 skinMatrix = boneMatrices[aBoneIndices.x] * aBoneWeights.x
                       + boneMatrices[aBoneIndices.y] * aBoneWeights.y
                       + boneMatrices[aBoneIndices.z] * aBoneWeights.z
                       + boneMatrices[aBoneIndices.w] * aBoneWeights.w;
        pos = skinMatrix * pos;
        norm = mat3(skinMatrix) * norm;
    }
    gl_Position = projection * view * model * pos;
    Normal = norm;
    TexCoord = aTexCoord;
}
)";

const char* m3FragSrc = R"(
#version 330 core
out vec4 FragColor;
in vec3 Normal;
in vec2 TexCoord;
uniform sampler2D diffTexture;
void main() {
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(normalize(Normal), lightDir), 0.3);
    vec3 texColor = texture(diffTexture, TexCoord).rgb;
    FragColor = vec4(texColor * diff, 1.0);
}
)";

const char* skeletonVertSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
uniform mat4 view, projection;
out vec3 vColor;
void main() {
    gl_Position = projection * view * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

const char* skeletonFragSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor, 1.0);
}
)";

static unsigned int CompileShader(GLenum type, const char* src) {
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    return s;
}

struct RenderVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec4 boneWeights;
    glm::uvec4 boneIndices;
};

M3Render::M3Render(const M3ModelData& data, const ArchivePtr& arc) {
    if (!data.success) return;

    submeshes = data.geometry.submeshes;
    materials = data.materials;
    bones = data.bones;
    textures = data.textures;
    animations = data.animations;
    submeshGroups = data.submeshGroups;

    materialSelectedVariant.assign(materials.size(), 0);
    submeshVisible.assign(submeshes.size(), 1);
    submeshVariantOverride.assign(submeshes.size(), -1);

    std::set<uint8_t> uniqueGroups;
    for (const auto& sm : submeshes) {
        if (sm.groupId != 255) uniqueGroups.insert(sm.groupId);
    }
    if (!uniqueGroups.empty()) {
        setActiveVariant(*uniqueGroups.begin());
    }

    loadTextures(data, arc);
    fallbackWhiteTex = createFallbackWhite();

    std::vector<RenderVertex> renderVerts;
    renderVerts.reserve(data.geometry.vertices.size());
    for (const auto& v : data.geometry.vertices) {
        RenderVertex rv;
        rv.position = v.position;
        rv.normal = v.normal;
        rv.uv = v.uv1;
        rv.boneWeights = v.boneWeights;
        rv.boneIndices = v.boneIndices;
        renderVerts.push_back(rv);
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, renderVerts.size() * sizeof(RenderVertex), renderVerts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, data.geometry.indices.size() * sizeof(uint32_t), data.geometry.indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), (void*)offsetof(RenderVertex, normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), (void*)offsetof(RenderVertex, uv));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), (void*)offsetof(RenderVertex, boneWeights));

    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(4, 4, GL_UNSIGNED_INT, sizeof(RenderVertex), (void*)offsetof(RenderVertex, boneIndices));

    setupShader();
    setupSkeletonShader();
}

M3Render::~M3Render() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
    if (shaderProgram) glDeleteProgram(shaderProgram);
    for (unsigned int tid : glTextures) if (tid != 0) glDeleteTextures(1, &tid);
    if (fallbackWhiteTex) glDeleteTextures(1, &fallbackWhiteTex);
    if (skeletonVAO) glDeleteVertexArrays(1, &skeletonVAO);
    if (skeletonVBO) glDeleteBuffers(1, &skeletonVBO);
    if (skeletonProgram) glDeleteProgram(skeletonProgram);
}

void M3Render::setupShader() {
    unsigned int v = CompileShader(GL_VERTEX_SHADER, m3VertexSrc);
    unsigned int f = CompileShader(GL_FRAGMENT_SHADER, m3FragSrc);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, v);
    glAttachShader(shaderProgram, f);
    glLinkProgram(shaderProgram);
    glDeleteShader(v);
    glDeleteShader(f);
}

void M3Render::setupSkeletonShader() {
    unsigned int v = CompileShader(GL_VERTEX_SHADER, skeletonVertSrc);
    unsigned int f = CompileShader(GL_FRAGMENT_SHADER, skeletonFragSrc);
    skeletonProgram = glCreateProgram();
    glAttachShader(skeletonProgram, v);
    glAttachShader(skeletonProgram, f);
    glLinkProgram(skeletonProgram);
    glDeleteShader(v);
    glDeleteShader(f);

    glGenVertexArrays(1, &skeletonVAO);
    glGenBuffers(1, &skeletonVBO);
}

void M3Render::loadTextures(const M3ModelData& data, const ArchivePtr& arc) {
    glTextures.clear();
    glTextures.reserve(data.textures.size());
    for (const auto& tex : data.textures) {
        glTextures.push_back(loadTextureFromArchive(arc, tex.path));
    }
}

unsigned int M3Render::createFallbackWhite() {
    unsigned int tid = 0;
    glGenTextures(1, &tid);
    glBindTexture(GL_TEXTURE_2D, tid);
    uint32_t px = 0xFFFFFFFFu;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &px);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return tid;
}

unsigned int M3Render::loadTextureFromArchive(const ArchivePtr& arc, const std::string& path) {
    if (!arc || path.empty()) return 0;

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    std::wstring wp = conv.from_bytes(path);
    if (wp.find(L".tex") == std::wstring::npos) wp += L".tex";

    auto entry = arc->getByPath(wp);
    if (!entry) {
        std::replace(wp.begin(), wp.end(), L'/', L'\\');
        entry = arc->getByPath(wp);
    }
    if (!entry) return 0;

    std::vector<uint8_t> buffer;
    arc->getFileData(std::dynamic_pointer_cast<FileEntry>(entry), buffer);
    if (buffer.empty()) return 0;

    Tex::File tf;
    if (!tf.readFromMemory(buffer.data(), buffer.size())) return 0;

    Tex::ImageRGBA img;
    if (!tf.decodeLargestMipToRGBA(img)) return 0;

    unsigned int tid = 0;
    glGenTextures(1, &tid);
    glBindTexture(GL_TEXTURE_2D, tid);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width, img.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.rgba.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return tid;
}

unsigned int M3Render::resolveDiffuseTexture(uint16_t materialId, int variant) const {
    if (materialId >= materials.size()) return fallbackWhiteTex;
    const auto& m = materials[materialId];
    if (m.variants.empty()) return fallbackWhiteTex;

    int v = variant;
    if (v < 0) v = 0;
    if (v >= (int)m.variants.size()) v = (int)m.variants.size() - 1;

    int idx = m.variants[v].textureIndexA;
    if (idx >= 0 && idx < (int)glTextures.size()) {
        unsigned int tid = glTextures[idx];
        if (tid != 0) return tid;
    }
    return fallbackWhiteTex;
}

void M3Render::render(const glm::mat4& view, const glm::mat4& proj) {
    if (!shaderProgram || !VAO) return;

    glDisable(GL_BLEND);

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(proj));

    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(glGetUniformLocation(shaderProgram, "diffTexture"), 0);

    bool useSkinning = (playingAnimation >= 0 && !boneMatrices.empty());
    glUniform1i(glGetUniformLocation(shaderProgram, "useSkinning"), useSkinning ? 1 : 0);

    if (useSkinning && !boneMatrices.empty()) {
        GLint loc = glGetUniformLocation(shaderProgram, "boneMatrices");
        glUniformMatrix4fv(loc, (GLsizei)std::min(boneMatrices.size(), (size_t)200), GL_FALSE, glm::value_ptr(boneMatrices[0]));
    }

    glBindVertexArray(VAO);

    for (size_t i = 0; i < submeshes.size(); ++i) {
        if (i < submeshVisible.size() && submeshVisible[i] == 0) continue;

        const auto& sm = submeshes[i];

        int variant = 0;
        if (i < submeshVariantOverride.size() && submeshVariantOverride[i] >= 0) {
            variant = submeshVariantOverride[i];
        } else if (sm.materialID < materialSelectedVariant.size()) {
            variant = materialSelectedVariant[sm.materialID];
        }

        unsigned int tid = resolveDiffuseTexture(sm.materialID, variant);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tid);

        glDrawElementsBaseVertex(
            GL_TRIANGLES,
            sm.indexCount,
            GL_UNSIGNED_INT,
            (void*)(uintptr_t)(sm.startIndex * 4),
            sm.startVertex
        );
    }
}

size_t M3Render::getSubmeshCount() const { return submeshes.size(); }
const M3Submesh& M3Render::getSubmesh(size_t i) const { return submeshes[i]; }

size_t M3Render::getMaterialCount() const { return materials.size(); }
size_t M3Render::getMaterialVariantCount(size_t materialId) const {
    if (materialId >= materials.size()) return 0;
    return materials[materialId].variants.size();
}

int M3Render::getMaterialSelectedVariant(size_t materialId) const {
    if (materialId >= materialSelectedVariant.size()) return 0;
    return materialSelectedVariant[materialId];
}

void M3Render::setMaterialSelectedVariant(size_t materialId, int variant) {
    if (materialId >= materialSelectedVariant.size()) return;
    int maxV = (int)getMaterialVariantCount(materialId);
    if (maxV <= 0) { materialSelectedVariant[materialId] = 0; return; }
    if (variant < 0) variant = 0;
    if (variant >= maxV) variant = maxV - 1;
    materialSelectedVariant[materialId] = variant;
}

bool M3Render::getSubmeshVisible(size_t submeshId) const {
    if (submeshId >= submeshVisible.size()) return true;
    return submeshVisible[submeshId] != 0;
}

void M3Render::setSubmeshVisible(size_t submeshId, bool v) {
    if (submeshId >= submeshVisible.size()) return;
    submeshVisible[submeshId] = v ? 1 : 0;
}

int M3Render::getSubmeshVariantOverride(size_t submeshId) const {
    if (submeshId >= submeshVariantOverride.size()) return -1;
    return submeshVariantOverride[submeshId];
}

void M3Render::setSubmeshVariantOverride(size_t submeshId, int variantOrMinus1) {
    if (submeshId >= submeshVariantOverride.size()) return;
    submeshVariantOverride[submeshId] = variantOrMinus1;
}

void M3Render::setActiveVariant(int variantIndex) {
    activeVariant = variantIndex;

    for (size_t i = 0; i < submeshes.size(); ++i) {
        uint8_t gid = submeshes[i].groupId;
        if (gid == 255) {
            submeshVisible[i] = 1;
        } else if (variantIndex < 0) {
            submeshVisible[i] = 1;
        } else {
            submeshVisible[i] = (gid == (uint8_t)variantIndex) ? 1 : 0;
        }
    }
}

void M3Render::playAnimation(int index) {
    if (index < 0 || index >= (int)animations.size()) {
        stopAnimation();
        return;
    }
    playingAnimation = index;
    animationTime = 0.0f;
    animationPaused = false;
}

void M3Render::stopAnimation() {
    playingAnimation = -1;
    animationTime = 0.0f;
    animationPaused = false;
    boneMatrices.clear();
}

void M3Render::pauseAnimation() {
    animationPaused = true;
}

void M3Render::resumeAnimation() {
    animationPaused = false;
}

float M3Render::getAnimationDuration() const {
    if (playingAnimation < 0 || playingAnimation >= (int)animations.size()) return 0.0f;
    const auto& anim = animations[playingAnimation];
    return (anim.timestampEnd - anim.timestampStart) / 1000.0f;
}

static glm::vec3 interpolateScale(const M3AnimationTrack& track, float timeMs) {
    if (track.keyframes.empty()) return glm::vec3(1.0f);
    const M3KeyFrame* prev = &track.keyframes[0];
    const M3KeyFrame* next = prev;
    for (size_t i = 0; i < track.keyframes.size(); ++i) {
        if (track.keyframes[i].timestamp <= timeMs) {
            prev = &track.keyframes[i];
            next = (i + 1 < track.keyframes.size()) ? &track.keyframes[i + 1] : prev;
        } else {
            next = &track.keyframes[i];
            break;
        }
    }
    float t = 0.0f;
    if (next != prev && next->timestamp != prev->timestamp) {
        t = (timeMs - prev->timestamp) / (float)(next->timestamp - prev->timestamp);
        t = glm::clamp(t, 0.0f, 1.0f);
    }
    return glm::mix(prev->scale, next->scale, t);
}

static glm::quat interpolateRotation(const M3AnimationTrack& track, float timeMs) {
    if (track.keyframes.empty()) return glm::quat(1, 0, 0, 0);
    const M3KeyFrame* prev = &track.keyframes[0];
    const M3KeyFrame* next = prev;
    for (size_t i = 0; i < track.keyframes.size(); ++i) {
        if (track.keyframes[i].timestamp <= timeMs) {
            prev = &track.keyframes[i];
            next = (i + 1 < track.keyframes.size()) ? &track.keyframes[i + 1] : prev;
        } else {
            next = &track.keyframes[i];
            break;
        }
    }
    float t = 0.0f;
    if (next != prev && next->timestamp != prev->timestamp) {
        t = (timeMs - prev->timestamp) / (float)(next->timestamp - prev->timestamp);
        t = glm::clamp(t, 0.0f, 1.0f);
    }
    return glm::slerp(prev->rotation, next->rotation, t);
}

static glm::vec3 interpolateTranslation(const M3AnimationTrack& track, float timeMs) {
    if (track.keyframes.empty()) return glm::vec3(0.0f);
    const M3KeyFrame* prev = &track.keyframes[0];
    const M3KeyFrame* next = prev;
    for (size_t i = 0; i < track.keyframes.size(); ++i) {
        if (track.keyframes[i].timestamp <= timeMs) {
            prev = &track.keyframes[i];
            next = (i + 1 < track.keyframes.size()) ? &track.keyframes[i + 1] : prev;
        } else {
            next = &track.keyframes[i];
            break;
        }
    }
    float t = 0.0f;
    if (next != prev && next->timestamp != prev->timestamp) {
        t = (timeMs - prev->timestamp) / (float)(next->timestamp - prev->timestamp);
        t = glm::clamp(t, 0.0f, 1.0f);
    }
    return glm::mix(prev->translation, next->translation, t);
}

void M3Render::updateAnimation(float deltaTime) {
    if (playingAnimation < 0 || playingAnimation >= (int)animations.size()) {
        boneMatrices.clear();
        return;
    }

    const auto& anim = animations[playingAnimation];
    float duration = (anim.timestampEnd - anim.timestampStart) / 1000.0f;

    if (duration > 0.0f && !animationPaused) {
        animationTime += deltaTime;
        if (animationTime >= duration) {
            animationTime = std::fmod(animationTime, duration);
        }
    }

    float currentTimeMs = anim.timestampStart + animationTime * 1000.0f;

    boneMatrices.resize(bones.size());
    std::vector<glm::mat4> worldTransforms(bones.size());

    for (size_t i = 0; i < bones.size(); ++i) {
        const auto& bone = bones[i];

        glm::vec3 scale(1.0f);
        glm::quat rotation(1, 0, 0, 0);
        glm::vec3 translation = bone.position;

        for (int t = 0; t <= 2; ++t) {
            if (!bone.tracks[t].keyframes.empty()) {
                scale = interpolateScale(bone.tracks[t], currentTimeMs);
                break;
            }
        }

        for (int t = 4; t <= 5; ++t) {
            if (!bone.tracks[t].keyframes.empty()) {
                rotation = interpolateRotation(bone.tracks[t], currentTimeMs);
                break;
            }
        }

        if (!bone.tracks[6].keyframes.empty()) {
            translation = interpolateTranslation(bone.tracks[6], currentTimeMs);
        }

        glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
        glm::mat4 R = glm::mat4_cast(rotation);
        glm::mat4 T = glm::translate(glm::mat4(1.0f), translation);
        glm::mat4 localTransform = T * R * S;

        if (bone.parentId >= 0 && bone.parentId < (int)bones.size()) {
            worldTransforms[i] = worldTransforms[bone.parentId] * localTransform;
        } else {
            worldTransforms[i] = localTransform;
        }

        boneMatrices[i] = worldTransforms[i] * bone.inverseGlobalMatrix;
    }
}

void M3Render::renderSkeleton(const glm::mat4& view, const glm::mat4& proj) {
    if (!showSkeleton || bones.empty() || !skeletonProgram || !skeletonVAO) return;

    struct SkeletonVertex {
        glm::vec3 pos;
        glm::vec3 color;
    };

    std::vector<SkeletonVertex> verts;
    verts.reserve(bones.size() * 8);

    std::vector<glm::vec3> boneWorldPos(bones.size());

    if (playingAnimation >= 0 && !boneMatrices.empty()) {
        for (size_t i = 0; i < bones.size(); ++i) {
            glm::mat4 worldTransform = boneMatrices[i] * bones[i].globalMatrix;
            boneWorldPos[i] = glm::vec3(worldTransform[3]);
        }
    } else {
        for (size_t i = 0; i < bones.size(); ++i) {
            boneWorldPos[i] = glm::vec3(bones[i].globalMatrix[3]);
        }
    }

    for (size_t i = 0; i < bones.size(); ++i) {
        const auto& bone = bones[i];
        glm::vec3 bonePos = boneWorldPos[i];

        float ptSize = 0.02f;
        glm::vec3 jointColor(1.0f, 1.0f, 0.0f);
        verts.push_back({bonePos + glm::vec3(-ptSize, 0, 0), jointColor});
        verts.push_back({bonePos + glm::vec3(ptSize, 0, 0), jointColor});
        verts.push_back({bonePos + glm::vec3(0, -ptSize, 0), jointColor});
        verts.push_back({bonePos + glm::vec3(0, ptSize, 0), jointColor});
        verts.push_back({bonePos + glm::vec3(0, 0, -ptSize), jointColor});
        verts.push_back({bonePos + glm::vec3(0, 0, ptSize), jointColor});

        if (bone.parentId >= 0 && bone.parentId < (int)bones.size()) {
            glm::vec3 parentPos = boneWorldPos[bone.parentId];
            glm::vec3 boneColor(0.0f, 1.0f, 1.0f);
            verts.push_back({bonePos, boneColor});
            verts.push_back({parentPos, boneColor});
        }
    }

    if (verts.empty()) return;

    glBindVertexArray(skeletonVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skeletonVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(SkeletonVertex), verts.data(), GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SkeletonVertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SkeletonVertex), (void*)offsetof(SkeletonVertex, color));

    glUseProgram(skeletonProgram);
    glUniformMatrix4fv(glGetUniformLocation(skeletonProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(skeletonProgram, "projection"), 1, GL_FALSE, glm::value_ptr(proj));

    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_LINES, 0, (GLsizei)verts.size());
    glEnable(GL_DEPTH_TEST);
}