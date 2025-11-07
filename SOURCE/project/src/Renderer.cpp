#define PARALEL_EXECUTION

#include <execution>
#include <algorithm>

//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Material.h"
#include "Scene.h"
#include "Utils.h"
#include <iostream>

using namespace dae;

Renderer::Renderer(SDL_Window * pWindow) :
	m_pWindow(pWindow),
	m_pBuffer(SDL_GetWindowSurface(pWindow))
{
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);
	m_pBufferPixels = static_cast<uint32_t*>(m_pBuffer->pixels);
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBuffer, "RayTracing_Buffer.bmp");
}

void Renderer::ToggleShadow()
{
	m_ShadowsEnabled = !m_ShadowsEnabled;
}

void Renderer::SwitchLightingMode()
{
	switch (m_LightMode)
	{
	case LightingMode::ObservedArea:
		m_LightMode = LightingMode::Radiance;
		break;
	case LightingMode::Radiance:
		m_LightMode = LightingMode::BRDF;
		break;
	case LightingMode::BRDF:
		m_LightMode = LightingMode::Combined;
		break;
	case LightingMode::Combined:
		m_LightMode = LightingMode::ObservedArea;
		break;
	default:
		break;
	}
}

void Renderer::RenderPixel(Scene* pScene, uint32_t pixelIndex, float fov, float aspectRatio, const Matrix& cameraToWorld, const Vector3& cameraOrigin) const
{
	auto materials{ pScene->GetMaterials() };

	const uint32_t px{ pixelIndex % m_Width };
	const uint32_t py{ pixelIndex / m_Width };

	float rx{ px + 0.5f };
	float ry{ py + 0.5f };
	float cx{ (2 * (rx / float(m_Width)) - 1) * aspectRatio * fov };
	float cy{ (1 - (2 * (ry / float(m_Height)))) * fov };

	ColorRGB finalColor{};

	HitRecord closestHit{};

	Vector3 rayDirCamera = Vector3{ cx, cy, 1 }.Normalized();
	Vector3 rayDirection = cameraToWorld.TransformVector(rayDirCamera).Normalized();
	Ray hitRay{ cameraOrigin, rayDirection };

	pScene->GetClosestHit(hitRay, closestHit);

	if (closestHit.didHit)
	{
		for (const auto& light : pScene->GetLights())
		{
			Vector3 rayToLight = (light.origin - closestHit.origin);
			float length = rayToLight.Normalize();

			if (Vector3::Dot(closestHit.normal, rayToLight) < 0)
			{
				continue;
			}

			Ray shadowRay{ closestHit.origin + closestHit.normal * 0.001f, rayToLight };
			shadowRay.min = 0.001f;
			shadowRay.max = length;

			ColorRGB brdf = materials[closestHit.materialIndex]->Shade(closestHit, rayToLight, -rayDirection);
			ColorRGB radiance = LightUtils::GetRadiance(light, closestHit.origin);
			float lambertCosineLaw = std::max(0.0f, Vector3::Dot(closestHit.normal, rayToLight));
			ColorRGB observedArea = ColorRGB{ lambertCosineLaw, lambertCosineLaw, lambertCosineLaw };

			if(m_ShadowsEnabled)
			{
				if (!pScene->DoesHit(shadowRay))
				{
					switch (m_LightMode)
					{
					case dae::Renderer::LightingMode::ObservedArea:
						finalColor += observedArea;
						break;
					case dae::Renderer::LightingMode::Radiance:
						finalColor += radiance;
						break;
					case dae::Renderer::LightingMode::BRDF:
						finalColor += brdf;
						break;
					case dae::Renderer::LightingMode::Combined:
						finalColor += radiance * (brdf * observedArea);
						break;
					default:
						break;
					}	
				}
			}
			else
			{
				switch (m_LightMode)
				{
				case dae::Renderer::LightingMode::ObservedArea:
					finalColor += observedArea;
					break;
				case dae::Renderer::LightingMode::Radiance:
					finalColor += radiance;
					break;
				case dae::Renderer::LightingMode::BRDF:
					finalColor += brdf;
					break;
				case dae::Renderer::LightingMode::Combined:
					finalColor += radiance * (brdf * observedArea);
					break;
				default:
					break;
				}
			}
		}
	}
	finalColor.MaxToOne();

	m_pBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBuffer->format,
		static_cast<uint8_t>(finalColor.r * 255),
		static_cast<uint8_t>(finalColor.g * 255),
		static_cast<uint8_t>(finalColor.b * 255));
}

void Renderer::Render(Scene* pScene) const
{
	Camera& camera = pScene->GetCamera();
	Matrix camToWorld = camera.CalculateCameraToWorld();

	float aspectRatio = static_cast<float>(m_Width) / static_cast<float>(m_Height);

	float fovAngle = camera.fovAngle * TO_RADIANS;
	float fov = tanf(fovAngle * 0.5f);

	int totalPixels = m_Width * m_Height;

	#if defined(PARALEL_EXECUTION)
		// parallel logic
		uint32_t amountOfPixels{ uint32_t(m_Width * m_Height) };
		std::vector<uint32_t> pixelIndices{};

		pixelIndices.reserve(amountOfPixels);
		for (uint32_t index{}; index < amountOfPixels; ++index)
		{
			pixelIndices.emplace_back(index);
		}

		std::for_each(std::execution::par, pixelIndices.begin(), pixelIndices.end(), [&](int i)
			{
				RenderPixel(pScene, i, fov, aspectRatio, camToWorld, camera.origin);
			});
	#else
		//synchronous logic (no threading)
		uint32_t amountOfPixels{ uint32_t(m_Width * m_Height) };
		for (uint32_t pixelIndex{}; pixelIndex < amountOfPixels; ++pixelIndex)
		{
			RenderPixel(pScene, pixelIndex, fov, aspectRatio, camToWorld, camera.origin);
		}

	#endif

	//@END
	//Update SDL Surface
	SDL_UpdateWindowSurface(m_pWindow);
}