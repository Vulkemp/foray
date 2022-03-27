#pragma once
#include <stdint.h>
#include <vkbootstrap/VkBootstrap.h>
#include "hsk_logger.hpp"
#include "../osi/hsk_osi.hpp"

namespace hsk
{
	class AppUpdateTiming
	{
	protected:
		float mSecondsPerUpdate = 1.f / 60.f;

	public:
		inline void UpdatesPerSecond(float value) { mSecondsPerUpdate = 1.f / value; }
		inline void SecondsPerUpdate(float value) { mSecondsPerUpdate = value; }
		inline float UpdatesPerSecond() const { return 1.f / mSecondsPerUpdate; }
		inline float SecondsPerUpdate() const { return mSecondsPerUpdate; }

		inline void Set60Fps() { mSecondsPerUpdate = 1.f / 60.f; }
		inline void DisableFpsLock() { mSecondsPerUpdate = 0; }
	};

	/// @brief Application base providing bare minimum of functionality (app lifetime, event handling, vulkan instance management)
	class MinimalAppBase
	{
	public:
		MinimalAppBase() = default;
		virtual ~MinimalAppBase() = default;

#pragma region Lifetime

	public:
		/// @brief Enum representing application lifetime state
		enum class EState
		{
			/// @brief Application is not initialized
			Uninitialized,
			/// @brief Application is initializing
			Preparing,
			/// @brief Application is running
			Running,
			/// @brief Application is requested to stop
			StopRequested,
			/// @brief Application is cleaning up
			Finalizing
		};

	public:
		AppUpdateTiming UpdateTiming;
		OsManager OSI;

	protected:
		EState mState = EState::Uninitialized;
		inline void State(EState state) { PrintStateChange(mState, state);  mState = state; }

		void PrintStateChange(EState oldState, EState newState)
		{
			logger()->info("{} => {}", StateToString(oldState), StateToString(newState));
		}

		std::string StateToString(EState state)
		{
			switch (state)
			{
			case EState::Uninitialized:
				return "Uninitialized";
			case EState::Preparing:
				return "Preparing";
			case EState::Running:
				return "Running";
			case EState::StopRequested:
				return "StopRequested";
			case EState::Finalizing:
				return "Finalizing";
			default:
				return "StateUnknown";
			}
		}

	public:
		inline EState State() const { return mState; }

		/// @brief Runs through the entire lifetime of the app
		int32_t Run();

	protected: // Base methods (Used by the base for lifetime management. Usually don't need to override those)
		/// @brief Inits the SDL subsystem
		virtual void BaseInitSdlSubsystem();
		/// @brief Inits the applications basic vulkan objects
		virtual void BaseInit();
		/// @brief Polls and distributes events from the SDL subsystem
		virtual void BasePollEvents();
		/// @brief Destroys the vulkan instance
		virtual void BaseCleanupVkInstance();
		/// @brief Destroys the SDL subsystem
		virtual void BaseCleanupSdlSubsystem();

	protected: // Virtual methods every application should override
		/// @brief Override this method to alter vulkan instance creation parameters via the instance builder
		inline virtual void BeforeInstanceCreate(vkb::InstanceBuilder& instanceBuilder) {}
		/// @brief Override this method to init your application
		inline virtual void Init() {}
		/// @brief Override this method to render your application
		inline virtual void Render(float delta) {}
		/// @brief Override this method to react to events
		inline virtual void OnEvent(std::shared_ptr<Event> event) {}
		/// @brief Override this method to cleanup your application
		inline virtual void Cleanup() {}
#pragma endregion

#pragma region Vulkan
		vkb::InstanceBuilder mVkbInstanceBuilder{};
		vkb::Instance mVkbInstance;
		VkInstance mInstance{};
		bool mEnableDefaultValidationLayers{true};
#pragma endregion
	};

	// /// @brief Application base providing further infrastructure useful for the vast variety of projects (default window, default single virtual device)
	// class DefaultAppBase : public MinimalAppBase
	// {
	// 	DefaultAppBase() : MinimalAppBase() {}
	// 	virtual ~DefaultAppBase() = default;
	// }
}