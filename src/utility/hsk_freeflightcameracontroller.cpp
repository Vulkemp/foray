#include "hsk_freeflightcameracontroller.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../osi/hsk_window.hpp"

namespace hsk {

    void FreeFlightCameraController::Init(const VkContext* context, Camera* camera, OsManager* osManager)
    {
        CameraController::Init(context, camera, osManager);

        mInputBinaryKeyW = osManager->Keyboard()->FindButton(EButton::Keyboard_W);
        mInputBinaryKeyA = osManager->Keyboard()->FindButton(EButton::Keyboard_A);
        mInputBinaryKeyS = osManager->Keyboard()->FindButton(EButton::Keyboard_S);
        mInputBinaryKeyD = osManager->Keyboard()->FindButton(EButton::Keyboard_D);
    }
    void FreeFlightCameraController::OnEvent(std::shared_ptr<Event> event)
    {
        if(!mCameraActive)
        {
            return;
        }

        auto inputBinary = std::dynamic_pointer_cast<EventInputBinary>(event);
        if(inputBinary)
        {
            ProcessKeyboardEvent(inputBinary);
        }

        auto mouseMoved = std::dynamic_pointer_cast<EventInputMouseMoved>(event);
        if(mouseMoved)
        {
            ProcessMouseMovedEvent(mouseMoved);
        }
    }

    void FreeFlightCameraController::Update(float delta)
    {
        float deltaTime = 1.0f;
        float speed = mCameraSpeed * deltaTime;
        if(mInputBinaryKeyW->State())
            mCameraPos += speed * mCameraFront;
        if(mInputBinaryKeyS->State())
            mCameraPos -= speed * mCameraFront;
        if(mInputBinaryKeyA->State())
            mCameraPos -= glm::normalize(glm::cross(mCameraFront, mCameraUp)) * speed;
        if(mInputBinaryKeyD->State())
            mCameraPos += glm::normalize(glm::cross(mCameraFront, mCameraUp)) * speed;
        
        auto aspectRatio         = mContext->Swapchain.extent.width / static_cast<float>(mContext->Swapchain.extent.height);
        mCamera->ProjectionMat() = glm::perspective(glm::radians(45.0f), aspectRatio, 1.0f, 5000.0f);
        mCamera->ViewMat() = glm::lookAt(mCameraPos, mCameraPos + mCameraFront, mCameraUp);
        mCamera->Update();
    }

    void FreeFlightCameraController::ProcessKeyboardEvent(std::shared_ptr<EventInputBinary> event)
    {
        auto pressed    = event->Pressed();
        auto buttonType = event->Button()->Button();
        if(buttonType == EButton::Keyboard_Space && pressed)
        {
            mReactOnMouseMoveEvents = !mReactOnMouseMoveEvents;
            mFirstMoveAfterUnlock   = true;
        }
    }

    void FreeFlightCameraController::ProcessMouseMovedEvent(std::shared_ptr<EventInputMouseMoved> event) {
        
        if(!mReactOnMouseMoveEvents)
        {
            return;
        }
        if(mIgnoreNextMouseEvent)
        {
            mIgnoreNextMouseEvent = false;
            return;
        }

        float xpos = event->CurrentX();
        float ypos = event->CurrentY();
        

        float centerX = mContext->Swapchain.extent.width / 2;
        float centerY = mContext->Swapchain.extent.height / 2;

        float xoffset = xpos - centerX;
        float yoffset = centerY - ypos;

        logger()->info("current x,y {}/{} offset {}/{}", xpos, ypos, xoffset, yoffset);

        if(mFirstMoveAfterUnlock)
        {
            // prevent camera jumping when unlocking the camera
            mFirstMoveAfterUnlock = false;
            xoffset               = 0;
            yoffset               = 0;
        }

        // lock cursor to screen center
        auto window = Window::Windows()[0]->GetSdlWindowHandle();  // TODO only assume one window?
        mIgnoreNextMouseEvent = true;
        SDL_WarpMouseInWindow(window, centerX, centerY);
        

        xoffset *= mSensitivity;
        yoffset *= mSensitivity;

        mYaw += xoffset;
        mPitch += yoffset * mInvertYAxis;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if(mPitch > 89.0f)
            mPitch = 89.0f;
        if(mPitch < -89.0f)
            mPitch = -89.0f;

        glm::vec3 front;
        front.x      = cos(glm::radians(mYaw)) * cos(glm::radians(mPitch));
        front.y      = sin(glm::radians(mPitch));
        front.z      = sin(glm::radians(mYaw)) * cos(glm::radians(mPitch));
        mCameraFront = glm::normalize(front);

        mCamera->ViewMat() = glm::lookAt(mCameraPos, mCameraPos + mCameraFront, mCameraUp);
    }
}  // namespace hsk