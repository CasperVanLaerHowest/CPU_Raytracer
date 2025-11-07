#pragma once
#include <SDL_keyboard.h>
#include <SDL_mouse.h>
#include "Math.h"
#include "Timer.h"

namespace dae
{
	struct Camera final
	{
		Camera() = default;
		Camera(const Vector3& _origin, float _fovAngle) :
			origin{ _origin },
			fovAngle{ _fovAngle }
		{}

		float cameraSpeed{ 5.f };
		Vector3 origin{};
		float fovAngle{ 90.f };
		float getFov() const { return fovAngle; }

		Vector3 forward{ Vector3::UnitZ };
		Vector3 up{ Vector3::UnitY };
		Vector3 right{ Vector3::UnitX };

		float totalPitch{ 0.f };
		float totalYaw{ 0.f };

		Matrix cameraToWorld{};

		Matrix CalculateCameraToWorld()
		{
			//todo: W2
			right = Vector3::Cross(forward, Vector3::UnitY).Normalized();
			up = Vector3::Cross(right, forward).Normalized();
			cameraToWorld = Matrix(
				{ right.x,right.y,right.z,0.f },
				{ up.x,up.y,up.z,0.f },
				{ forward.x,forward.y,forward.z,0.f },
				{ origin.x,origin.y,origin.z,1.f });
			return cameraToWorld;
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);


			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			//todo: W2
			if(pKeyboardState[SDL_SCANCODE_W])
			{
				origin += forward * cameraSpeed * deltaTime;
			}
			if (pKeyboardState[SDL_SCANCODE_S])
			{
				origin -= forward * cameraSpeed * deltaTime;
			}
			if (pKeyboardState[SDL_SCANCODE_A])
			{
				origin -= right * cameraSpeed * deltaTime;
			}
			if (pKeyboardState[SDL_SCANCODE_D])
			{
				origin += right * cameraSpeed * deltaTime;
			}
			if (pKeyboardState[SDL_SCANCODE_Q])
			{
				origin.y -= cameraSpeed * deltaTime;
			}
			if (pKeyboardState[SDL_SCANCODE_E])
			{
				origin.y += cameraSpeed * deltaTime;
			}

			if(mouseState == 0)
				return;

			bool leftPressed = (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
			bool rightPressed = (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;

			if (leftPressed && rightPressed)
			{
				origin.y += cameraSpeed * deltaTime * mouseY;
			}
			else if (rightPressed)
			{
				const float mouseSensitivity{ 0.25f };
				totalYaw -= mouseX * mouseSensitivity;
				totalPitch -= mouseY * mouseSensitivity;
				if (totalPitch > 89.f)
					totalPitch = 89.f;
				else if (totalPitch < -89.f)
					totalPitch = -89.f;
				//Calculate new forward
				forward.x = cosf(totalPitch * TO_RADIANS) * sinf(totalYaw * TO_RADIANS);
				forward.y = sinf(totalPitch * TO_RADIANS);
				forward.z = cosf(totalPitch * TO_RADIANS) * cosf(totalYaw * TO_RADIANS);
				forward.Normalize();
				//Calculate right and up
				right = Vector3::Cross(forward, Vector3::UnitY).Normalized();
				up = Vector3::Cross(right, forward).Normalized();
			}
			else if (leftPressed)
			{
				//Move forward/backward
				origin += forward * cameraSpeed * deltaTime * static_cast<float>(mouseY);
				

				//rotate right/left
				totalYaw += mouseX * 0.1f;
				//Calculate new forward
				forward.x = cosf(totalPitch * TO_RADIANS) * sinf(totalYaw * TO_RADIANS);
				forward.y = sinf(totalPitch * TO_RADIANS);
				forward.z = cosf(totalPitch * TO_RADIANS) * cosf(totalYaw * TO_RADIANS);
				forward.Normalize();
				//Calculate right and up
				right = Vector3::Cross(forward, Vector3::UnitY).Normalized();
				up = Vector3::Cross(right, forward).Normalized();
			}
		}
	};
}
