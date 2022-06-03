#include "hsk_cameracontroller.hpp"

namespace hsk {

	void CameraController::Init(const VkContext* context, Camera* camera, OsManager* osManager)
    {
        mContext   = context;
        mCamera    = camera;
        mOsManager = osManager;
    }

    void CameraController::OnEvent(std::shared_ptr<Event> event) {}

}