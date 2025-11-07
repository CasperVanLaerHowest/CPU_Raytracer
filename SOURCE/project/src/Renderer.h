#pragma once
#include <cstdint>
#include "Math.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Scene;
	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer() = default;

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Render(Scene* pScene) const;
		bool SaveBufferToImage() const;

		void ToggleShadow();
		void SwitchLightingMode();

	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pBuffer{};
		uint32_t* m_pBufferPixels{};

		int m_Width{};
		int m_Height{};

		void RenderPixel(Scene* pScene, uint32_t pixelIndex, float fov, float aspectRatio, const Matrix& cameraToWorld, const Vector3& cameraOrigin) const;

		enum class LightingMode {
			ObservedArea,
			Radiance,
			BRDF,
			Combined
		};

		LightingMode m_LightMode{ LightingMode::Combined };
		bool m_ShadowsEnabled{ true };
	};
}
