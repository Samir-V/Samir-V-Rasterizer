#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Maths.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle):
			origin{_origin},
			fovAngle{_fovAngle}
		{
		}


		Vector3 origin{};
		float fovAngle{90.f};
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{};
		float totalYaw{};

		float aspectRatio{};
		float near{0.1f};
		float far{100.0f};

		const float movementSpeed{ 10.0f };
		const float rotSpeed{ 0.1f };

		Matrix invViewMatrix{};
		Matrix viewMatrix{};
		Matrix projectionMatrix{};

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = {0.f,0.f,0.f})
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			origin = _origin;

			CalculateProjectionMatrix();
		}

		void CalculateViewMatrix()
		{
			const Vector3 newRightNormalized = Vector3::Cross(Vector3::UnitY, forward).Normalized();
			right = newRightNormalized;

			const Vector3 newUpNormalized = Vector3::Cross(forward, newRightNormalized).Normalized();
			up = newUpNormalized;

			const Matrix cameraToWorldMatrix(Vector4{ newRightNormalized, 0 }, Vector4{ newUpNormalized, 0 },
			                                 Vector4{ forward, 0 }, Vector4{ origin, 1 });

			invViewMatrix = cameraToWorldMatrix;

			viewMatrix = invViewMatrix.Inverse();
		}

		void CalculateProjectionMatrix()
		{
			projectionMatrix = Matrix::CreatePerspectiveFovLH(fov, aspectRatio, near, far);
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			//Camera Update Logic
			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			if (pKeyboardState[SDL_SCANCODE_W] || pKeyboardState[SDL_SCANCODE_UP])
			{
				origin += forward * movementSpeed * deltaTime;
			}

			if (pKeyboardState[SDL_SCANCODE_S] || pKeyboardState[SDL_SCANCODE_DOWN])
			{
				origin -= forward * movementSpeed * deltaTime;
			}

			if (pKeyboardState[SDL_SCANCODE_D] || pKeyboardState[SDL_SCANCODE_RIGHT])
			{
				origin += right * movementSpeed * deltaTime;
			}

			if (pKeyboardState[SDL_SCANCODE_A] || pKeyboardState[SDL_SCANCODE_LEFT])
			{
				origin -= right * movementSpeed * deltaTime;
			}

			if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT) && mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT)) // if both are pressed
			{
				if (mouseY > 0)
				{
					origin -= Vector3::UnitY * movementSpeed * deltaTime;
				}
				else if (mouseY < 0)
				{
					origin += Vector3::UnitY * movementSpeed * deltaTime;
				}
			}
			else if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) // if left is pressed
			{
				if (mouseY > 0)
				{
					origin -= forward * movementSpeed * deltaTime;
				}
				else if (mouseY < 0)
				{
					origin += forward * movementSpeed * deltaTime;
				}

				const float rotAngleY = mouseX * rotSpeed * deltaTime;
				totalYaw += rotAngleY;
			}
			else if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT)) // if right is pressed
			{
				const float rotAngleX = mouseY * rotSpeed * deltaTime;
				totalPitch -= rotAngleX;

				const float rotAngleY = mouseX * rotSpeed * deltaTime;
				totalYaw += rotAngleY;
			}

			const Matrix rotMatrix = Matrix::CreateRotation(totalPitch, totalYaw, 0);

			forward = rotMatrix.TransformVector(Vector3::UnitZ);
			forward.Normalize();

			//Update Matrices
			CalculateViewMatrix();
		}
	};
}
