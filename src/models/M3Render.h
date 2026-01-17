#pragma once
#include "M3Common.h"
#include "../Archive.h"
#include "../tex/tex.h"
#include <glad/glad.h>
#include <vector>
#include <memory>

class M3Render {
public:
    M3Render(const M3ModelData& data, const ArchivePtr& arc);
    ~M3Render();

    void render(const glm::mat4& view, const glm::mat4& proj);

    size_t getSubmeshCount() const;
    const M3Submesh& getSubmesh(size_t i) const;
    const std::vector<M3Submesh>& getAllSubmeshes() const { return submeshes; }
    const std::vector<M3Material>& getAllMaterials() const { return materials; }
    const std::vector<M3Bone>& getAllBones() const { return bones; }
    const std::vector<M3Texture>& getAllTextures() const { return textures; }
    const std::vector<M3ModelAnimation>& getAllAnimations() const { return animations; }

    size_t getMaterialCount() const;
    size_t getMaterialVariantCount(size_t materialId) const;
    int getMaterialSelectedVariant(size_t materialId) const;
    void setMaterialSelectedVariant(size_t materialId, int variant);

    bool getSubmeshVisible(size_t submeshId) const;
    void setSubmeshVisible(size_t submeshId, bool v);

    int getSubmeshVariantOverride(size_t submeshId) const;
    void setSubmeshVariantOverride(size_t submeshId, int variantOrMinus1);

    size_t getBoneCount() const { return bones.size(); }
    const M3Bone& getBone(size_t i) const { return bones[i]; }

    size_t getAnimationCount() const { return animations.size(); }
    const M3ModelAnimation& getAnimation(size_t i) const { return animations[i]; }

    void setModelName(const std::string& name) { modelName = name; }
    const std::string& getModelName() const { return modelName; }
    const std::vector<unsigned int>& getGLTextures() const { return glTextures; }

private:
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    unsigned int shaderProgram = 0;

    std::vector<M3Submesh> submeshes;
    std::vector<M3Material> materials;
    std::vector<M3Bone> bones;
    std::vector<M3Texture> textures;
    std::vector<M3ModelAnimation> animations;
    std::vector<unsigned int> glTextures;

    std::vector<int> materialSelectedVariant;
    std::vector<uint8_t> submeshVisible;
    std::vector<int> submeshVariantOverride;

    std::string modelName;

    unsigned int fallbackWhiteTex = 0;

    void setupShader();
    void loadTextures(const M3ModelData& data, const ArchivePtr& arc);
    unsigned int loadTextureFromArchive(const ArchivePtr& arc, const std::string& path);
    unsigned int createFallbackWhite();
    unsigned int resolveDiffuseTexture(uint16_t materialId, int variant) const;
};