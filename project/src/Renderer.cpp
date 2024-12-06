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

	m_Texture = std::make_unique<Texture>(*Texture::LoadFromFile("resources/vehicle_diffuse.png"));
	m_NormalMap = std::make_unique<Texture>(*Texture::LoadFromFile("resources/vehicle_normal.png"));
	m_SpecularMap = std::make_unique<Texture>(*Texture::LoadFromFile("resources/vehicle_specular.png"));
	m_GlossMap = std::make_unique<Texture>(*Texture::LoadFromFile("resources/vehicle_gloss.png"));

	m_Shininess = 25.0f;
	m_Kd = 7.0f;
	m_Ks = 0.5f;

	Utils::ParseOBJ("resources/vehicle.obj", m_WorldMeshes[0].vertices, m_WorldMeshes[0].indices);

	//Initialize Camera
	m_Camera.Initialize(45.f, { .0f, .0f, 0.f });
	m_Camera.aspectRatio = static_cast<float>(m_Width) / static_cast<float>(m_Height);
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	if (m_RotationOn)
	{
		yaw = 1.0f * pTimer->GetElapsed();

		const Vector3 translation{ m_WorldMeshes[0].worldMatrix.GetTranslation() };

		const Matrix translationMatrix = Matrix::CreateTranslation(translation);
		const Matrix reverseTranslationMatrix = Matrix::CreateTranslation(-translation);
		const Matrix rotationMatrix = Matrix::CreateRotationY(yaw);

		m_WorldMeshes[0].worldMatrix *= reverseTranslationMatrix;
		m_WorldMeshes[0].worldMatrix *= rotationMatrix;
		m_WorldMeshes[0].worldMatrix *= translationMatrix;
	}
}

float Renderer::Remap(float depthValue, float min, float max)
{
	return min + depthValue * (max - min);
}


void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	Uint32 clearColor = SDL_MapRGB(m_pBackBuffer->format, 128, 128, 128);
	SDL_FillRect(m_pBackBuffer, nullptr, clearColor);

	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

	m_WorldMeshes[0].vertices_out.reserve(m_WorldMeshes[0].vertices.size());

	VertexTransformationFunction(m_WorldMeshes[0].vertices, m_WorldMeshes[0].vertices_out);

	Vertex_Out firstVertex;
	Vertex_Out secondVertex;
	Vertex_Out thirdVertex;

	size_t index0;
	size_t index1;
	size_t index2;

	for (size_t index{ 0 }; index < m_WorldMeshes[0].indices.size();)
	{
		if (m_WorldMeshes[0].primitiveTopology == PrimitiveTopology::TriangleStrip)
		{
			if (index + 2 >= m_WorldMeshes[0].indices.size())
			{
				break;
			}

			if (index % 2 == 0)
			{
				index0 = m_WorldMeshes[0].indices[index];
				index1 = m_WorldMeshes[0].indices[index + 1];
				index2 = m_WorldMeshes[0].indices[index + 2];

				firstVertex = m_WorldMeshes[0].vertices_out[index0];
				secondVertex = m_WorldMeshes[0].vertices_out[index1];
				thirdVertex = m_WorldMeshes[0].vertices_out[index2];
			}
			else
			{
				index0 = m_WorldMeshes[0].indices[index];
				index1 = m_WorldMeshes[0].indices[index + 2];
				index2 = m_WorldMeshes[0].indices[index + 1];

				firstVertex = m_WorldMeshes[0].vertices_out[index0];
				secondVertex = m_WorldMeshes[0].vertices_out[index1];
				thirdVertex = m_WorldMeshes[0].vertices_out[index2];
			}

			++index;
		}
		else if (m_WorldMeshes[0].primitiveTopology == PrimitiveTopology::TriangleList)
		{
			if (index + 2 >= m_WorldMeshes[0].indices.size())
			{
				break;
			}

			index0 = m_WorldMeshes[0].indices[index];
			index1 = m_WorldMeshes[0].indices[index + 1];
			index2 = m_WorldMeshes[0].indices[index + 2];

			firstVertex = m_WorldMeshes[0].vertices_out[index0];
			secondVertex = m_WorldMeshes[0].vertices_out[index1];
			thirdVertex = m_WorldMeshes[0].vertices_out[index2];

			index += 3;
		}

		// If the z component is further than far and smaller than near - skip the current calculation
		if (!(firstVertex.position.z > FLT_EPSILON && firstVertex.position.z < 1) || !(secondVertex.position.z > FLT_EPSILON && secondVertex.position.z < 1) || !(thirdVertex.position.z > FLT_EPSILON && thirdVertex.position.z < 1))
		{
			continue;
		}

		Vector2 V0{ firstVertex.position.x, firstVertex.position.y };
		Vector2 V1{ secondVertex.position.x, secondVertex.position.y };
		Vector2 V2{ thirdVertex.position.x, thirdVertex.position.y };

		if (V0 == V1 || V1 == V2 || V2 == V0)
		{
			return;
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
			std::ceil(std::max(stepMaxX, thirdVertex.position.x)),
			std::ceil(std::max(stepMaxY, thirdVertex.position.y))
		};

		// Does not exceed screen (Pixels are not floats)
		topLeft.x = int(std::max(0.0f, std::min(topLeft.x, static_cast<float>(m_Width - 1))));
		topLeft.y = int(std::max(0.0f, std::min(topLeft.y, static_cast<float>(m_Height - 1))));

		botRight.x = int(std::max(0.0f, std::min(botRight.x, static_cast<float>(m_Width - 1))));
		botRight.y = int(std::max(0.0f, std::min(botRight.y, static_cast<float>(m_Height - 1))));

		//RENDER LOGIC
		for (int px{int(topLeft.x)}; px < int(botRight.x); ++px)
		{
			for (int py{int(topLeft.y)}; py < int(botRight.y); ++py)
			{
				Vector2 currentPixel{ px + 0.5f, py + 0.5f };

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

					ColorRGB finalColour;

					if (!m_DepthBufferView)
					{
						const auto interUV = (firstVertex.uv / firstVertex.position.w * weightV0 + secondVertex.uv / secondVertex.position.w * weightV1 + thirdVertex.uv / thirdVertex.position.w * weightV2) * interWDepth;
						const auto sampledColour = m_Texture->Sample(interUV);
						const auto normal = Vector3(firstVertex.normal * weightV0 + secondVertex.normal * weightV1 + thirdVertex.normal * weightV2);
						const auto tangent = Vector3(firstVertex.tangent * weightV0 + secondVertex.tangent * weightV1 + thirdVertex.tangent * weightV2);
						const Vector3 pos = firstVertex.position * weightV0 + secondVertex.position * weightV1 + thirdVertex.position * weightV2;
						const auto viewDir = (pos - m_Camera.origin).Normalized();

						const Vertex_Out pixelVertexData{ {}, sampledColour, interUV, normal, tangent, viewDir };

						finalColour = PixelShading(pixelVertexData);
					}
					else if (m_DepthBufferView)
					{
						float depthValue = Remap(zBuffer, 0.995f, 1.0f);

						float colorValue = (depthValue - 0.995f) / (1.0f - 0.995f);

						finalColour = ColorRGB{ colorValue, colorValue, colorValue };
					}

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

	const Matrix worldMatrix = m_WorldMeshes[0].worldMatrix;
	const Matrix megaMatrix = worldMatrix * m_Camera.viewMatrix * Matrix::CreatePerspectiveFovLH(m_Camera.fov, m_Camera.aspectRatio, m_Camera.near, m_Camera.far);

	for (size_t index{0}; index < vertices_in.size(); ++index)
	{
		// Step 1. From world to Camera space + Step 3. Projection
		Vector4 transformedPoint = megaMatrix.TransformPoint(vertices_in[index].position.ToVector4());
		tempVector.emplace_back(transformedPoint, vertices_in[index].color, vertices_in[index].uv);

		// Step 2. Perspective divide
		tempVector[index].position.x = tempVector[index].position.x / tempVector[index].position.w;
		tempVector[index].position.y = tempVector[index].position.y / tempVector[index].position.w;
		tempVector[index].position.z = tempVector[index].position.z / tempVector[index].position.w;

		// Step for additional info calculations
		tempVector[index].normal = worldMatrix.TransformVector(vertices_in[index].normal);
		tempVector[index].tangent = worldMatrix.TransformVector(vertices_in[index].tangent);
	}

	// Step 3. Converting to Raster Space (Screen Space)
	for (size_t index{0}; index < tempVector.size(); ++index)
	{
		tempVector[index].position.x = (tempVector[index].position.x + 1) / 2.0f * m_Width;
		tempVector[index].position.y = (1 - tempVector[index].position.y) / 2.0f * m_Height;
	}
		
	vertices_out = std::move(tempVector);
}

ColorRGB Renderer::PixelShading(const Vertex_Out& v)
{
	Vector3 finalNormal;

	// Sampling normal map
	if (m_NormalMapOn)
	{
		const Vector3 binormal = Vector3::Cross(v.normal, v.tangent);
		const auto tangentToWorldMatrix = Matrix{ v.tangent, binormal, v.normal, Vector3::Zero };

		const ColorRGB normalMapColour = m_NormalMap->Sample(v.uv);
		auto sampledNormal = Vector3{ normalMapColour.r, normalMapColour.g, normalMapColour.b };

		sampledNormal /= 255.0f;
		sampledNormal = 2.0f * sampledNormal - Vector3{ 1.0f, 1.0f, 1.0f };

		sampledNormal = tangentToWorldMatrix.TransformVector(sampledNormal);
		sampledNormal.Normalize();

		finalNormal = sampledNormal;
	}
	else
	{
		finalNormal = v.normal;
	}

	ColorRGB finalColour;

	const ColorRGB specularMapColour = m_SpecularMap->Sample(v.uv) * m_Shininess;

	switch (m_ShadingMode)
	{
	case ShadingMode::ObservedArea:
		m_LightDirection = { 0.577f, -0.577f, 0.577f };
		m_ObservedArea = { std::max(Vector3::Dot(finalNormal, -m_LightDirection), 0.0f) };
		finalColour = ColorRGB{ m_ObservedArea, m_ObservedArea, m_ObservedArea };
		break;
	case ShadingMode::Diffuse:
		//const ColorRGB lambertDiffuse{ (m_Kd * v.color) / M_PI };
		finalColour = (m_Kd * v.color) / M_PI / 255.0f;
		break;
	case ShadingMode::Specular:
		finalColour = specularMapColour / 255.0f;
		break;
	case ShadingMode::Combined:
		m_LightDirection = { 0.577f, -0.577f, 0.577f };
		m_ObservedArea = { std::max(Vector3::Dot(finalNormal, -m_LightDirection), 0.0f) };

		const ColorRGB lambertDiffuse{ (m_Kd * v.color) / M_PI };

		auto reflect = Vector3::Reflect(-m_LightDirection, finalNormal);
		auto cosa = abs(Vector3::Dot(reflect, v.viewDirection));

		const float glossMapValue = m_GlossMap->Sample(v.uv).r;
		const float phong = m_Ks * powf(cosa, glossMapValue);

		const ColorRGB outColor{ (lambertDiffuse * m_ObservedArea + phong * specularMapColour) / 255.0f };
		finalColour = outColor;
		break;
	}

	return finalColour;
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

void Renderer::ToggleRotation()
{
	m_RotationOn = !m_RotationOn;
}

void Renderer::ToggleNormals()
{
	m_NormalMapOn = !m_NormalMapOn;
}

void Renderer::ToggleDepthBuffer()
{
	m_DepthBufferView = !m_DepthBufferView;
}

void Renderer::ToggleShadowMode()
{
	switch (m_ShadingMode)
	{
	case ShadingMode::Combined:
		m_ShadingMode = ShadingMode::ObservedArea;
		break;
	case ShadingMode::ObservedArea:
		m_ShadingMode = ShadingMode::Diffuse;
		break;
	case ShadingMode::Diffuse:
		m_ShadingMode = ShadingMode::Specular;
		break;
	case ShadingMode::Specular:
		m_ShadingMode = ShadingMode::Combined;
		break;
	}
}




