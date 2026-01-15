#pragma once
#include "M3Common.h"
#include "../Archive.h"
#include "../tex/tex.h"
#include <glad/glad.h>
#include <vector>
#include <memory>
#include <windows.h>

class M3Render {
public:
    M3Render(const M3ModelData& data, const ArchivePtr& arc);
    ~M3Render();

    void render(const glm::mat4& view, const glm::mat4& proj);

    size_t getSubmeshCount() const;
    const M3Submesh& getSubmesh(size_t i) const;

    const std::vector<M3Submesh>& getAllSubmeshes() const { return submeshes; }
    const std::vector<M3MaterialData>& getAllMaterials() const { return materials; }

    size_t getMaterialCount() const;
    size_t getMaterialVariantCount(size_t materialId) const;

    int getMaterialSelectedVariant(size_t materialId) const;
    void setMaterialSelectedVariant(size_t materialId, int variant);

    bool getSubmeshVisible(size_t submeshId) const;
    void setSubmeshVisible(size_t submeshId, bool v);

    int getSubmeshVariantOverride(size_t submeshId) const;
    void setSubmeshVariantOverride(size_t submeshId, int variantOrMinus1);

private:
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    unsigned int shaderProgram = 0;

    std::vector<M3Submesh> submeshes;
    std::vector<M3MaterialData> materials;
    std::vector<unsigned int> glTextures;

    std::vector<int> materialSelectedVariant;
    std::vector<uint8_t> submeshVisible;
    std::vector<int> submeshVariantOverride;

    unsigned int fallbackWhiteTex = 0;

    void setupShader();
    void loadTextures(const M3ModelData& data, const ArchivePtr& arc);
    unsigned int loadTextureFromArchive(const ArchivePtr& arc, const std::string& path);
    unsigned int createFallbackWhite();
    unsigned int resolveDiffuseTexture(uint16_t materialId, int variant) const;
};
