#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <memory>

#include "Camera.h"
#include "DataTypes.h"

namespace dae
{
	struct Vertex_Out;
}

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
	class Timer;
	class Scene;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render();

		bool SaveBufferToImage() const;

		void VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex_Out>& vertices_out) const;
		ColorRGB PixelShading(const Vertex_Out& v);
		float Remap(float depthValue, float min, float max);
		float Remap01(float value, float start, float stop);

		void ToggleRotation();
		void ToggleNormals();
		void ToggleDepthBuffer();
		void ToggleShadowMode();

		enum class ShadingMode
		{
			ObservedArea,
			Diffuse,
			Specular,
			Combined
		};

		float yaw{};

	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		Camera m_Camera{};

		std::unique_ptr<Texture> m_Texture;
		std::unique_ptr<Texture> m_NormalMap;
		std::unique_ptr<Texture> m_SpecularMap;
		std::unique_ptr<Texture> m_GlossMap;

		float m_Shininess{};
		float m_Kd{};
		float m_Ks{};
		float m_ObservedArea{};
		Vector3 m_LightDirection{};

		std::vector<Mesh> m_WorldMeshes{
			Mesh
			{
				{},
				{},
				PrimitiveTopology::TriangleList,
				{},
				Matrix::CreateTranslation(0, -5, 64)
			}
		};

		int m_Width{};
		int m_Height{};

		bool m_RotationOn{ true };
		bool m_NormalMapOn{ true };
		bool m_DepthBufferView{ false };

		ShadingMode m_ShadingMode = ShadingMode::Combined;

		std::vector<Vertex_Out> m_TempVector{};
	};
}
