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

		enum class ColorMode
		{
			FinalColor,
			DepthBuffer
		};

		float yaw{};

		ColorMode ColorMode = ColorMode::FinalColor;

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

		std::vector<Mesh> m_WorldMeshes{
			Mesh
			{
				{},
				{},
				PrimitiveTopology::TriangleList,
				{}
			}

			/*Mesh{
					{
				{{-3.0f, 3.0f, -2.0f}, colors::White, Vector2{0.0f, 0.0f}},
				{{0.0f, 3.0f, -2.0f}, colors::White, Vector2{0.5f, 0.0f}},
				{{3.0f, 3.0f, -2.0f}, colors::White, Vector2{1.0f, 0.0f}},
				{{-3.0f, 0.0f, -2.0f}, colors::White, Vector2{0.0f, 0.5f}},
				{{0.0f, 0.0f, -2.0f}, colors::White, Vector2{0.5f, 0.5f}},
				{{3.0f, 0.0f, -2.0f}, colors::White, Vector2{1.0f, 0.5f}},
				{{-3.0f, -3.0f, -2.0f}, colors::White, Vector2{0.0f, 1.0f}},
				{{0.0f, -3.0f, -2.0f}, colors::White, Vector2{0.5f, 1.0f}},
				{{3.0f, -3.0f, -2.0f}, colors::White, Vector2{1.0f, 1.0f}},
				},
					{
						3, 0, 4, 1, 5, 2,
						2, 6,
						6, 3, 7, 4, 8, 5
					},

					PrimitiveTopology::TriangleStrip
				}*/
		};

		int m_Width{};
		int m_Height{};
	};
}
