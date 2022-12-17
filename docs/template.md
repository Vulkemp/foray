# Template
This is no code capable of running on its own, look to [foray-examples](https://github.com/Vulkemp/foray-examples) if you need that. This is instead just a simple template
## Header
```cpp
#pragma once
#include <foray_api.hpp>

namespace foray-template {

    class TemplateApp : public foray::base::DefaultAppBase // 
    {
      protected:
        virtual void ApiInit() override; // Load the scene and configure render stages
        virtual void ApiOnEvent(const foray::osi::Event* event) override; // Send OS events to active scene

        virtual void ApiOnResized(VkExtent2D size) override; // Send resize event to active scene (to update aspect ratio of camera projection matrix)

        virtual void ApiRender(foray::base::FrameRenderInfo& renderInfo) override; // Record our frame onto the command buffer

        virtual void ApiDestroy() override; // Destroy any allocated


        // TODO: Add additional render stages

        foray::stages::ImageToSwapchainStage mSwapCopyStage; // Copies a frame buffer to the swapchain
        std::unique_ptr<foray::scene::Scene> mScene;
    };

}  // namespace foray-template
```
## Source
```cpp

    void TemplateApp::ApiInit()
    {
        // Load scene
        mScene = std::make_unique<foray::scene::Scene>(&mContext);
        foray::gltf::ModelConverter converter(mScene.get());
        converter.LoadGltfModel(
            // TODO: Add path to glTF file to load
        );

        // Initialize TLAS
        mScene->UpdateTlasManager();
        // Adds a node with camera + freeflight controls
        mScene->UseDefaultCamera(true);

        // Initialize and configure stages
        // TODO: Configure your stages
        foray::core::ManagedImage* mainFrameBuf = nullptr; // TODO: set main frame buffer
        mSwapCopyStage.Init(&mContext, mainFrameBuf);
        mSwapCopyStage.SetFlipY(true); // Flip viewport since scene is loaded as +Y = up, but Vulkan uses -Y = up

        // TODO: Register your stages so they receive the resize, os event and shader compile callbacks
        RegisterRenderStage(&mSwapCopyStage);
    }

    void TemplateApp::ApiOnEvent(const foray::osi::Event* event)
    {
        mScene->InvokeOnEvent(event);
    }

    void TemplateApp::ApiOnResized(VkExtent2D size)
    {
        mScene->InvokeOnResized(size);
    }

    void TemplateApp::ApiRender(foray::base::FrameRenderInfo& renderInfo)
    {
        // Get and begin command buffer
        foray::core::DeviceSyncCommandBuffer& cmdBuffer = renderInfo.GetPrimaryCommandBuffer();
        cmdBuffer.Begin();

        // Update scene (uploads scene specific dynamic data such as node transformations, camera matrices, ...)
        mScene->Update(renderInfo, cmdBuffer);

        // TODO: Record frames

        // Copy ray tracing output to swapchain
        mSwapCopyStage.RecordFrame(cmdBuffer, renderInfo);
        
        // Prepare swapchain image for present and submit command buffer
        renderInfo.GetInFlightFrame()->PrepareSwapchainImageForPresent(cmdBuffer, renderInfo.GetImageLayoutCache());
        cmdBuffer.Submit();
    }

    void TemplateApp::ApiDestroy()
    {
        /// TODO: Destroy any resource allocated
        mSwapCopyStage.Destroy();
        mScene = nullptr;
    }
```
## CMakeLists.txt
```cmake
    project(template) // TODO: Name your project

    # collect sources
    file(GLOB_RECURSE src "*.cpp")
    
    # Make sure there are source files, add_executable would otherwise fail
    if (NOT src)
        message(FATAL_ERROR "Project \"${PROJECT_NAME}\" does not contain any source files")
    endif ()

    # Declare executable
    add_executable(${PROJECT_NAME} ${src})
    
    # Set strict mode for project only
    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS ${STRICT_FLAGS})

    # Set directories via compile macros
    target_compile_options(${PROJECT_NAME} PUBLIC "-DFORAY_SHADER_DIR=\"$CACHE{FORAY_SHADER_DIR}\"")

    # Link foray lib
    target_link_libraries(
    	${PROJECT_NAME}
    	PUBLIC foray
    )

    # Windows requires SDL2 libs linked specifically
    if (WIN32)
    	target_link_libraries(
    		${PROJECT_NAME}
    		PUBLIC ${SDL2_LIBRARIES}
    	)
    endif()


# Configure include directories
target_include_directories(
	${PROJECT_NAME}
	PUBLIC "${CMAKE_SOURCE_DIR}/foray/src"
	PUBLIC "${CMAKE_SOURCE_DIR}/foray/third_party"
	PUBLIC ${Vulkan_INCLUDE_DIR}
)
```
