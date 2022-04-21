#pragma once
#include "glm/glm.hpp"
#include "hsk_glTF_declares.hpp"
#include <tinygltf/tiny_gltf.h>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace hsk {
    struct Material : public SceneComponent
    {
      public:
        enum EAlphaMode
        {
            Opaque,
            Mask,
            Blend
        };

        struct TexCoordSets
        {
            uint8_t BaseColor         = 0;
            uint8_t MetallicRoughness = 0;
            uint8_t Normal            = 0;
            uint8_t Occlusion         = 0;
            uint8_t Emissive          = 0;
        };

        // inline std::string_view Name() const { return mName; }
        // inline std::string&     Name() { return mName; }
        // inline Material&        Name(std::string_view name)
        // {
        //     mName = name;
        //     return *this;
        // }
        // inline EAlphaMode AlphaMode() const { return mAlphaMode; }
        // inline Material&  AlphaMode(EAlphaMode alphamode)
        // {
        //     mAlphaMode = alphamode;
        //     return *this;
        // }
        // inline double    AlphaCutoff() const { return mAlphaCutoff; }
        // inline Material& AlphaCutoff(double alphacutoff)
        // {
        //     mAlphaCutoff = alphacutoff;
        //     return *this;
        // }
        // inline double    IsDoubleSided() const { return mIsDoubleSided; }
        // inline Material& IsDoubleSided(bool doublesided)
        // {
        //     mIsDoubleSided = doublesided;
        //     return *this;
        // }
        // inline const glm::vec4& BaseColorFactor() const { return mBaseColorFactor; }
        // inline glm::vec4&       BaseColorFactor() { return mBaseColorFactor; }
        // inline Material&        BaseColorFactor(const glm::vec4& basecolor)
        // {
        //     mBaseColorFactor = basecolor;
        //     return *this;
        // }
        // inline const Texture* BaseColorTexture() const { return mBaseColorTexture; }
        // inline Texture*       BaseColorTexture() { return mBaseColorTexture; }
        // inline Material&      BaseColorTexture(Texture* texture)
        // {
        //     mBaseColorTexture = texture;
        //     return *this;
        // }
        // inline double    MetallicFactor() const { return mMetallicFactor; }
        // inline double&   MetallicFactor() { return mMetallicFactor; }
        // inline Material& MetallicFactor(double value)
        // {
        //     mMetallicFactor = value;
        //     return *this;
        // }
        // inline double    RoughnessFactor() const { return mRoughnessFactor; }
        // inline double&   RoughnessFactor() { return mRoughnessFactor; }
        // inline Material& RoughnessFactor(double value)
        // {
        //     mRoughnessFactor = value;
        //     return *this;
        // }
        // inline const Texture* MetallicRoughnessTexture() const { return mMetallicRoughnessTexture; }
        // inline Texture*       MetallicRoughnessTexture() { return mMetallicRoughnessTexture; }
        // inline Material&      MetallicRoughnessTexture(Texture* texture)
        // {
        //     mMetallicRoughnessTexture = texture;
        //     return *this;
        // }
        // inline const glm::vec3& EmissiveFactor() const { return mEmissiveFactor; }
        // inline glm::vec3&       EmissiveFactor() { return mEmissiveFactor; }
        // inline Material&        EmissiveFactor(const glm::vec3& value)
        // {
        //     mEmissiveFactor = value;
        //     return *this;
        // }
        // inline const Texture* EmissiveTexture() const { return mEmissiveTexture; }
        // inline Texture*       EmissiveTexture() { return mEmissiveTexture; }
        // inline Material&      EmissiveTexture(Texture* texture)
        // {
        //     mEmissiveTexture = texture;
        //     return *this;
        // }

        Material();
        Material(Scene* scene);

        void InitFromTinyGltfMaterial(tinygltf::Material& material);

        inline bool IsLoaded() const { return Loaded; }

        static EAlphaMode ParseTinyGltfAlphaMode(std::string_view raw);

        std::string Name = {};

        EAlphaMode AlphaMode     = EAlphaMode::Opaque;
        double     AlphaCutoff   = 0.5;
        bool       IsDoubleSided = false;

        glm::vec4 BaseColorFactor          = {1, 1, 1, 1};
        Texture*  BaseColorTexture         = nullptr;
        double    MetallicFactor           = 1;
        double    RoughnessFactor          = 1;
        Texture*  MetallicRoughnessTexture = nullptr;

        glm::vec3 EmissiveFactor  = glm::vec3(1.0f);
        Texture*  EmissiveTexture = nullptr;

        Texture* NormalTexture    = nullptr;
        Texture* OcclusionTexture = nullptr;

        TexCoordSets TexCoordIndices;

      protected:
        bool Loaded;
    };

}  // namespace hsk