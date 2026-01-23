#pragma once
#include "M3Common.h"
#include "../Archive.h"
#include "../tex/tex.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <string>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class M3Render {
public:
    static void SetDevice(ID3D11Device* device, ID3D11DeviceContext* context);
    static void InitSharedResources();

    M3Render(const M3ModelData& data, const ArchivePtr& arc, bool highestLodOnly = false, bool skipTextures = false);

    void setTextureSRV(size_t index, ComPtr<ID3D11ShaderResourceView> srv);
    ~M3Render();

    void render(const XMMATRIX& view, const XMMATRIX& proj);
    void render(const XMMATRIX& view, const XMMATRIX& proj, const XMMATRIX& model);
    void renderGlm(const glm::mat4& view, const glm::mat4& proj, const glm::mat4& model);

    size_t getSubmeshCount() const;
    const M3Submesh& getSubmesh(size_t i) const;
    const std::vector<M3Submesh>& getAllSubmeshes() const { return submeshes; }
    const std::vector<M3Material>& getAllMaterials() const { return materials; }
    const std::vector<M3Bone>& getAllBones() const { return bones; }
    const std::vector<M3Texture>& getAllTextures() const { return textures; }
    const std::vector<ComPtr<ID3D11ShaderResourceView>>& getGLTextures() const { return mTextureSRVs; }
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

    const M3Geometry& getGeometry() const { return geometry; }
    const std::vector<M3Vertex>& getVertices() const { return geometry.vertices; }
    const std::vector<uint32_t>& getIndices() const { return geometry.indices; }

    void setModelName(const std::string& name) { modelName = name; }
    const std::string& getModelName() const { return modelName; }
    const std::vector<M3SubmeshGroup>& getSubmeshGroups() const { return submeshGroups; }

    bool hasTexturesLoaded() const { return texturesLoaded; }
    bool hasPendingTextures() const { return nextTextureToLoad < pendingTexturePaths.size(); }
    size_t getPendingTextureCount() const { return pendingTexturePaths.size(); }
    void queueTexturesForLoading();
    bool uploadNextTexture(const ArchivePtr& arc);

    void setActiveVariant(int variantIndex);
    int getActiveVariant() const { return activeVariant; }

    void setShowSkeleton(bool show) { showSkeleton = show; }
    bool getShowSkeleton() const { return showSkeleton; }
    void renderSkeleton(const XMMATRIX& view, const XMMATRIX& proj);

    void setSelectedSubmesh(int idx) { selectedSubmesh = idx; }
    int getSelectedSubmesh() const { return selectedSubmesh; }
    int rayPickSubmesh(const XMFLOAT3& rayOrigin, const XMFLOAT3& rayDir) const;

    void setSelectedBone(int idx) { selectedBone = idx; }
    int getSelectedBone() const { return selectedBone; }
    int rayPickBone(const XMFLOAT3& rayOrigin, const XMFLOAT3& rayDir) const;

    void playAnimation(int index);
    void stopAnimation();
    void pauseAnimation();
    void resumeAnimation();
    int getPlayingAnimation() const { return playingAnimation; }
    bool isAnimationPlaying() const { return playingAnimation >= 0; }
    bool isAnimationPaused() const { return animationPaused; }
    void updateAnimation(float deltaTime);
    float getAnimationTime() const { return animationTime; }
    float getAnimationDuration() const;

private:
    static ID3D11Device* sDevice;
    static ID3D11DeviceContext* sContext;
    static ComPtr<ID3D11VertexShader> sSharedVS;
    static ComPtr<ID3D11PixelShader> sSharedPS;
    static ComPtr<ID3D11InputLayout> sSharedLayout;
    static ComPtr<ID3D11VertexShader> sSkeletonVS;
    static ComPtr<ID3D11PixelShader> sSkeletonPS;
    static ComPtr<ID3D11InputLayout> sSkeletonLayout;
    static ComPtr<ID3D11SamplerState> sSharedSampler;
    static ComPtr<ID3D11RasterizerState> sRasterState;
    static ComPtr<ID3D11DepthStencilState> sDepthState;
    static ComPtr<ID3D11BlendState> sBlendState;
    static bool sShadersInitialized;

    static std::unordered_map<std::string, ComPtr<ID3D11ShaderResourceView>> sTextureSRVCache;
    static std::mutex sTextureCacheMutex;

    ComPtr<ID3D11Buffer> mVertexBuffer;
    ComPtr<ID3D11Buffer> mIndexBuffer;
    ComPtr<ID3D11Buffer> mConstantBuffer;
    ComPtr<ID3D11Buffer> mBoneBuffer;
    ComPtr<ID3D11Buffer> mSkeletonVB;
    ComPtr<ID3D11Buffer> mSkeletonCB;
    ComPtr<ID3D11ShaderResourceView> mFallbackWhiteSRV;

    std::vector<M3Submesh> submeshes;
    std::vector<M3Material> materials;
    std::vector<M3Bone> bones;
    std::vector<M3Texture> textures;
    std::vector<M3ModelAnimation> animations;
    std::vector<ComPtr<ID3D11ShaderResourceView>> mTextureSRVs;
    M3Geometry geometry;

    std::vector<int> materialSelectedVariant;
    std::vector<uint8_t> submeshVisible;
    std::vector<int> submeshVariantOverride;

    std::vector<glm::mat4> effectiveBindGlobal;
    std::vector<glm::mat4> inverseEffectiveBindGlobal;
    std::vector<glm::vec3> bindLocalScale;
    std::vector<glm::quat> bindLocalRotation;
    std::vector<glm::vec3> bindLocalTranslation;
    std::vector<bool> boneAtOrigin;
    std::vector<glm::mat4> worldTransforms;
    std::vector<XMMATRIX> boneMatrices;
    size_t mSkeletonVBSize = 0;

    std::string modelName;
    std::vector<M3SubmeshGroup> submeshGroups;
    int activeVariant = -1;
    bool showSkeleton = false;
    int selectedSubmesh = -1;
    int selectedBone = -1;
    int playingAnimation = -1;
    float animationTime = 0.0f;
    bool animationPaused = false;

    bool texturesLoaded = false;
    std::vector<std::string> pendingTexturePaths;
    size_t nextTextureToLoad = 0;
    ArchivePtr archiveRef;

    void precomputeBoneData();
    void loadTextures(const M3ModelData& data, const ArchivePtr& arc);
    ComPtr<ID3D11ShaderResourceView> loadTextureFromArchive(const ArchivePtr& arc, const std::string& path);
    ComPtr<ID3D11ShaderResourceView> createFallbackWhite();
    ID3D11ShaderResourceView* resolveDiffuseTexture(uint16_t materialId, int variant) const;

    struct alignas(16) M3ConstantBuffer {
        XMMATRIX model;
        XMMATRIX view;
        XMMATRIX projection;
        XMFLOAT3 highlightColor;
        float highlightMix;
        int useSkinning;
        XMFLOAT3 pad;
    };

    struct alignas(16) BoneMatrixBuffer {
        XMMATRIX bones[200];
    };

    struct alignas(16) SkeletonCB {
        XMMATRIX view;
        XMMATRIX projection;
    };
};