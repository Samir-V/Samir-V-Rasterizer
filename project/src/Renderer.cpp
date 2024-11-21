//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"

#include <iostream>
#include <algorithm>

#include "Maths.h"
#include "Texture.h"
#include "Utils.h"

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_pDepthBufferPixels = new float[m_Width * m_Height];

	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

	m_Texture = Texture::LoadFromFile("resources/uv_grid_2.png");

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });
	m_Camera.aspectRatio = static_cast<float>(m_Width) / static_cast<float>(m_Height);
}

Renderer::~Renderer()
{
	delete m_Texture;
	delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	Uint32 clearColor = SDL_MapRGB(m_pBackBuffer->format, 0, 0, 0);
	SDL_FillRect(m_pBackBuffer, nullptr, clearColor);

	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

	std::vector<Vertex> vertices_world
	{
		// First triangle
		//{{0.0f, 2.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
		//{{1.5f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
		//{{-1.5f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},

		//// Second triangle
		//{{0.0f, 4.0f, 2.0f}, {1.0f, 0.0f, 0.0f}},
		//{{3.0f, -2.0f, 2.0f}, {0.0f, 1.0f, 0.0f}},
		//{{-3.0f, -2.0f, 2.0f}, {0.0f, 0.0f, 1.0f}},
	};

	std::vector<Mesh> meshes_world
	{
		Mesh{
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
		}
	};

	/*std::vector<Mesh> meshes_world
	{
		Mesh{
			{
		{{-3.0f, 3.0f, -2.0f}, colors::White, Vector2{0.0f, 0.0f}},
		{{0.0f, 3.0f, -2.0f}, colors::White, Vector2{0.5f, 0.0f}},
		{{3.0f, 3.0f, -2.0f}, colors::White, Vector2{1.0f, 0.0f}},
		{{-3.0f, 0.0f, -2.0f}, colors::White, Vector2{0.0f, 0.5f}},
		{{0.0f, 0.0f, -2.0f}, colors::White, Vector2{0.5f, 0.5f}},
		{{3.0f, 0.0f, -2.0f}, colors::White, Vector2{1.0f, 0.5f}},
		{{-3.0f, -3.0f, -2.0f}, colors::White, Vector2{0.0f, 1.0f}},
		{{0.0f, -3.0f, -2.0f}, colors::White, Vector2{0.5f, 0.1f}},
		{{3.0f, -3.0f, -2.0f}, colors::White, Vector2{1.0f, 1.0f}},
		},
			{
				3, 0, 1,    1, 4, 3,    4, 1, 2,
				2, 5, 4,    6, 3, 4,    4, 7, 6,
				7, 4, 5,    5, 8, 7
			},

			PrimitiveTopology::TriangleList
		}
	};*/

	meshes_world[0].vertices_out.reserve(meshes_world[0].vertices.size());

	VertexTransformationFunction(meshes_world[0].vertices, meshes_world[0].vertices_out);

	Vertex_Out firstVertex;
	Vertex_Out secondVertex;
	Vertex_Out thirdVertex;

	size_t index0;
	size_t index1;
	size_t index2;

	for (size_t index{ 0 }; index < meshes_world[0].indices.size();)
	{
		if (meshes_world[0].primitiveTopology == PrimitiveTopology::TriangleStrip)
		{
			if (index + 2 >= meshes_world[0].indices.size())
			{
				break;
			}

			if (index % 2 == 0)
			{
				index0 = meshes_world[0].indices[index];
				index1 = meshes_world[0].indices[index + 1];
				index2 = meshes_world[0].indices[index + 2];

				firstVertex = meshes_world[0].vertices_out[index0];
				secondVertex = meshes_world[0].vertices_out[index1];
				thirdVertex = meshes_world[0].vertices_out[index2];
			}
			else
			{
				index0 = meshes_world[0].indices[index];
				index1 = meshes_world[0].indices[index + 2];
				index2 = meshes_world[0].indices[index + 1];

				firstVertex = meshes_world[0].vertices_out[index0];
				secondVertex = meshes_world[0].vertices_out[index1];
				thirdVertex = meshes_world[0].vertices_out[index2];
			}

			++index;
		}
		else if (meshes_world[0].primitiveTopology == PrimitiveTopology::TriangleList)
		{
			if (index + 2 >= meshes_world[0].indices.size())
			{
				break;
			}

			index0 = meshes_world[0].indices[index];
			index1 = meshes_world[0].indices[index + 1];
			index2 = meshes_world[0].indices[index + 2];

			firstVertex = meshes_world[0].vertices_out[index0];
			secondVertex = meshes_world[0].vertices_out[index1];
			thirdVertex = meshes_world[0].vertices_out[index2];

			index += 3;
		}

		auto stepMinX = std::min(firstVertex.position.x, secondVertex.position.x);
		auto stepMinY = std::min(firstVertex.position.y, secondVertex.position.y);

		auto stepMaxX = std::max(firstVertex.position.x, secondVertex.position.x);
		auto stepMaxY = std::max(firstVertex.position.y, secondVertex.position.y);

		// Bounding box coordinates
		Vector2 topLeft
		{
			std::min(stepMinX, thirdVertex.position.x),
			std::min(stepMinY, thirdVertex.position.y)
		};

		Vector2 botRight
		{
			std::max(stepMaxX, thirdVertex.position.x),
			std::max(stepMaxY, thirdVertex.position.y)
		};

		// Does not exceed screen (Pixels are not floats)
		topLeft.x = int(std::max(0.0f, std::min(topLeft.x, static_cast<float>(m_Width - 1))));
		topLeft.y = int(std::max(0.0f, std::min(topLeft.y, static_cast<float>(m_Height - 1))));

		botRight.x = int(std::max(0.0f, std::min(botRight.x, static_cast<float>(m_Width - 1))));
		botRight.y = int(std::max(0.0f, std::min(botRight.y, static_cast<float>(m_Height - 1))));

		//RENDER LOGIC
		for (int px{}; px < m_Width; ++px)
		{
			if (px < topLeft.x || px > botRight.x) continue;

			for (int py{}; py < m_Height; ++py)
			{
				if (py < topLeft.y || py > botRight.y) continue;

				//if (firstVertex.position.w < 0 || secondVertex.position.w < 0 || thirdVertex.position.w < 0) continue;

				Vector2 currentPixel{ px + 0.5f, py + 0.5f };

				Vector2 V0{ firstVertex.position.x, firstVertex.position.y };
				Vector2 V1{ secondVertex.position.x, secondVertex.position.y };
				Vector2 V2{ thirdVertex.position.x, thirdVertex.position.y };

				auto eVector = V1 - V0;
				auto pVector = currentPixel - V0;

				auto cross1 = Vector2::Cross(eVector, pVector);

				if (cross1 < 0.0f) continue;

				eVector = V2 - V1;
				pVector = currentPixel - V1;

				auto cross2 = Vector2::Cross(eVector, pVector);

				if (cross2 < 0.0f) continue;

				eVector = V0 - V2;
				pVector = currentPixel - V2;

				auto cross3 = Vector2::Cross(eVector, pVector);

				if (cross3 < 0.0f) continue;

				// Calculating weights
				auto totalArea = Vector2::Cross(V1 - V0, V2 - V0);

				auto weightV0 = cross2 / totalArea;
				auto weightV1 = cross3 / totalArea;
				auto weightV2 = cross1 / totalArea;

				// Calculating the interpolated depth
				auto zBuffer = 1 / (1 / firstVertex.position.z * weightV0 + 1 / secondVertex.position.z * weightV1 + 1 / thirdVertex.position.z * weightV2);

				if (zBuffer < 0) continue;
				if (zBuffer > 1) continue;

				if (zBuffer < m_pDepthBufferPixels[px + (py * m_Width)])
				{
					m_pDepthBufferPixels[px + (py * m_Width)] = zBuffer;

					auto interWDepth = 1 / (1 / firstVertex.position.w * weightV0 + 1 / secondVertex.position.w * weightV1 + 1 / thirdVertex.position.w * weightV2);

					// Calculating the interpolated colour of the pixel
					/*ColorRGB interColour = (firstVertex.color / firstVertex.position.z * weightV0 + secondVertex.color / secondVertex.position.z * weightV1 + thirdVertex.color / thirdVertex.position.z * weightV2) * interDepth;

					ColorRGB finalColour = interColour;*/


					// Calculating the interpolated UV coordinates
					auto interUV = (firstVertex.uv / firstVertex.position.w * weightV0 + secondVertex.uv / secondVertex.position.w * weightV1 + thirdVertex.uv / thirdVertex.position.w * weightV2) * interWDepth;

					ColorRGB finalColour = m_Texture->Sample(interUV);

					//Update Color in Buffer
					finalColour.MaxToOne();

					m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
						static_cast<uint8_t>(finalColour.r * 255),
						static_cast<uint8_t>(finalColour.g * 255),
						static_cast<uint8_t>(finalColour.b * 255));
				}
			}
		}
	}

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex_Out>& vertices_out) const
{
	std::vector<Vertex_Out> tempVector{};

	tempVector.reserve(vertices_in.size());

	for (size_t index{0}; index < vertices_in.size(); ++index)
	{
		// Step 1. From world to Camera space + Step 3. Projection
		Matrix megaMartix = m_Camera.viewMatrix * Matrix::CreatePerspectiveFovLH(m_Camera.fov, m_Camera.aspectRatio, m_Camera.near, m_Camera.far);

		Vector4 transformedPoint = megaMartix.TransformPoint(vertices_in[index].position.ToVector4());
		tempVector.emplace_back(transformedPoint, vertices_in[index].color, vertices_in[index].uv);

		// Step 2. Perspective divide
		tempVector[index].position.x = tempVector[index].position.x / tempVector[index].position.w;
		tempVector[index].position.y = tempVector[index].position.y / tempVector[index].position.w;
		tempVector[index].position.z = tempVector[index].position.z / tempVector[index].position.w;

		// Step 3. Converting to NDC projective stage - Not used, since it is the part of the Matrix above
		// tempVector[index].position.x = tempVector[index].position.x / (m_Camera.aspectRatio * m_Camera.fov);
		// tempVector[index].position.y = tempVector[index].position.y / m_Camera.fov;
	}

	// Step 4. Converting to Raster Space (Screen Space)
	for (size_t index{0}; index < tempVector.size(); ++index)
	{
		tempVector[index].position.x = (tempVector[index].position.x + 1) / 2.0f * m_Width;
		tempVector[index].position.y = (1 - tempVector[index].position.y) / 2.0f * m_Height;
	}

	vertices_out = std::move(tempVector);
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
