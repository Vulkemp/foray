#pragma once
#include "../base/foray_framerenderinfo.hpp"
#include "../core/foray_context.hpp"
#include "../core/foray_core_declares.hpp"
#include "../core/foray_descriptorset.hpp"
#include "../core/foray_managedimage.hpp"
#include "../core/foray_shadermanager.hpp"
#include "../foray_event.hpp"
#include "../foray_basics.hpp"
#include "../osi/foray_env.hpp"
#include "foray_renderdomain.hpp"
#include <unordered_map>

namespace foray::stages {

    /// @brief Render stage base class giving a common interface for rendering processes
    class RenderStage
    {
      public:
        RenderStage(core::Context* context = nullptr, RenderDomain* domain = nullptr, int32_t priority = 0);
        virtual ~RenderStage() = default;

        /// @brief Records a frame to cmdBuffer.
        /// @param cmdBuffer Command buffer to record to
        /// @param renderInfo Additional information about the current frame being rendered
        /// @details
        /// # Inheriting
        ///  * Any resources accessed (images, buffers) must be protected by pipeline barriers, unless the providing entity defines them as constants
        ///  * All commands must be submitted to cmdBuffer
        inline virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) {}

        /// @brief Gets a vector to all color attachments that will be included in a texture array and can be referenced in the shader pass.
        std::vector<core::ManagedImage*> GetImageOutputs();
        /// @brief Gets an image output
        /// @param name Identifier
        /// @param noThrow If set, will return nullptr instead of throwing std::exception if no match is found
        core::ManagedImage* GetImageOutput(std::string_view name, bool noThrow = false);

        void SetResizeOrder(int32_t priority);

      protected:

        void InitCallbacks(core::Context* context = nullptr, RenderDomain* domain = nullptr, int32_t priority = 0);

        /// @brief Default implementation accesses mImageOutputs and calls ManagedImage::Resize(extent) on any set image
        /// @param extent New render size
        /// @remark Inheriting stages may override to update descriptor sets
        virtual void OnResized(VkExtent2D extent) {}

        /// @brief Notifies the stage that the shader compiler instance has recompiled a shader
        /// @details Implementation will check through shaders registered in 'mShaderKeys'. If any of them have been marked as recompiled, calls ReloadShaders()
        virtual void OnShadersRecompiled(const std::unordered_set<uint64_t>& recompiled);

        /// @brief Override this to reload all shaders and rebuild pipelines after a registered shader has been recompiled.
        virtual void ReloadShaders() {}

        /// @brief Inheriting types should emplace their images onto this collection to provide them in GetImageOutput interface
        std::unordered_map<std::string, core::ManagedImage*> mImageOutputs;
        /// @brief Inheriting types should emplace their shader keys onto this collection such that if a shader has been recompiled, ReloadShaders() will be called
        std::vector<uint64_t> mShaderKeys;
        /// @brief Context object the renderstage is built upon
        core::Context* mContext = nullptr;
        RenderDomain*  mDomain  = nullptr;

        event::PriorityReceiver<VkExtent2D> mOnResized;
        event::Receiver<const core::ShaderManager::KeySet&> mOnShadersRecompiled;
    };
}  // namespace foray::stages