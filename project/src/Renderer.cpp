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

	m_Texture = Texture::LoadFromFile("resources/vehicle_diffuse.png");
	m_NormalMap = Texture::LoadFromFile("resources/vehicle_normal.png");
	m_SpecularMap = Texture::LoadFromFile("resources/vehicle_specular.png");
	m_GlossMap = Texture::LoadFromFile("resources/vehicle_gloss.png");

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

		float stepMinX = std::min(firstVertex.position.x, secondVertex.position.x);
		float stepMinY = std::min(firstVertex.position.y, secondVertex.position.y);

		float stepMaxX = std::max(firstVertex.position.x, secondVertex.position.x);
		float stepMaxY = std::max(firstVertex.position.y, secondVertex.position.y);

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

				Vector2 eVector = V1 - V0;
				Vector2 pVector = currentPixel - V0;

				float cross1 = Vector2::Cross(eVector, pVector);

				if (cross1 < 0.0f) continue;

				eVector = V2 - V1;
				pVector = currentPixel - V1;

				float cross2 = Vector2::Cross(eVector, pVector);

				if (cross2 < 0.0f) continue;

				eVector = V0 - V2;
				pVector = currentPixel - V2;

				float cross3 = Vector2::Cross(eVector, pVector);

				if (cross3 < 0.0f) continue;

				// Calculating weights
				float totalArea = Vector2::Cross(V1 - V0, V2 - V0);

				float weightV0 = cross2 / totalArea;
				float weightV1 = cross3 / totalArea;
				float weightV2 = 1 - weightV0 - weightV1;

				// Calculating the interpolated depth
				float zBuffer = 1 / (1 / firstVertex.position.z * weightV0 + 1 / secondVertex.position.z * weightV1 + 1 / thirdVertex.position.z * weightV2);

				if (zBuffer < 0) continue;
				if (zBuffer > 1) continue;

				if (zBuffer < m_pDepthBufferPixels[px + (py * m_Width)])
				{
					m_pDepthBufferPixels[px + (py * m_Width)] = zBuffer;

					float interWDepth = 1 / (1 / firstVertex.position.w * weightV0 + 1 / secondVertex.position.w * weightV1 + 1 / thirdVertex.position.w * weightV2);

					ColorRGB finalColour;

					if (!m_DepthBufferView)
					{
						const Vector2 interUV = (firstVertex.uv / firstVertex.position.w * weightV0 + secondVertex.uv / secondVertex.position.w * weightV1 + thirdVertex.uv / thirdVertex.position.w * weightV2) * interWDepth;
						const Vector3 normal = Vector3(firstVertex.normal / firstVertex.position.w * weightV0 + secondVertex.normal / secondVertex.position.w * weightV1 + thirdVertex.normal / thirdVertex.position.w * weightV2) * interWDepth;
						const Vector3 tangent = Vector3(firstVertex.tangent / firstVertex.position.w * weightV0 + secondVertex.tangent / secondVertex.position.w * weightV1 + thirdVertex.tangent / thirdVertex.position.w * weightV2) * interWDepth;
						const Vector3 viewDir = (firstVertex.viewDirection / firstVertex.position.w * weightV0 + secondVertex.viewDirection / secondVertex.position.w * weightV1 + thirdVertex.viewDirection / thirdVertex.position.w * weightV2) * interWDepth;

						const Vertex_Out pixelVertexData{ {}, {}, interUV, normal, tangent, viewDir };

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
	vertices_out.clear();

	vertices_out.reserve(vertices_in.size());


	const Matrix worldMatrix = m_WorldMeshes[0].worldMatrix;
	const Matrix megaMatrix = worldMatrix * m_Camera.viewMatrix * Matrix::CreatePerspectiveFovLH(m_Camera.fov, m_Camera.aspectRatio, m_Camera.near, m_Camera.far);

	for (size_t index{0}; index < vertices_in.size(); ++index)
	{
		// Step 1. From world to Camera space + Step 3. Projection
		Vector4 transformedPoint = megaMatrix.TransformPoint(vertices_in[index].position.ToVector4());
		vertices_out.emplace_back(transformedPoint, vertices_in[index].color, vertices_in[index].uv);

		// Step 2. Perspective divide
		vertices_out[index].position.x = vertices_out[index].position.x / vertices_out[index].position.w;
		vertices_out[index].position.y = vertices_out[index].position.y / vertices_out[index].position.w;
		vertices_out[index].position.z = vertices_out[index].position.z / vertices_out[index].position.w;

		// Step for additional info calculations
		vertices_out[index].normal = worldMatrix.TransformVector(vertices_in[index].normal);
		vertices_out[index].tangent = worldMatrix.TransformVector(vertices_in[index].tangent);
		vertices_out[index].viewDirection = (m_Camera.origin - worldMatrix.TransformPoint(vertices_in[index].position)).Normalized();
	}

	// Step 3. Converting to Raster Space (Screen Space)
	for (size_t index{0}; index < vertices_out.size(); ++index)
	{
		vertices_out[index].position.x = (vertices_out[index].position.x + 1) / 2.0f * m_Width;
		vertices_out[index].position.y = (1 - vertices_out[index].position.y) / 2.0f * m_Height;
	}
}

ColorRGB Renderer::PixelShading(const Vertex_Out& v)
{
	Vector3 finalNormal;

	// Sampling normal map
	if (m_NormalMapOn)
	{
		const Vector3 binormal = Vector3::Cross(v.normal, v.tangent);
		const Matrix tangentToWorldMatrix = Matrix{ v.tangent, binormal, v.normal, Vector3::Zero };

		const ColorRGB normalMapColour = m_NormalMap->Sample(v.uv);
		Vector3 sampledNormal = Vector3{ normalMapColour.r, normalMapColour.g, normalMapColour.b };

		
		sampledNormal = 2.0f * sampledNormal - Vector3{ 1.0f, 1.0f, 1.0f };

		sampledNormal = tangentToWorldMatrix.TransformVector(sampledNormal);

		finalNormal = sampledNormal.Normalized();
	}
	else
	{
		finalNormal = v.normal.Normalized();
	}

	ColorRGB finalColour;

	m_LightDirection = { 0.577f, -0.577f, 0.577f };

	const Vector3 reflect = Vector3::Reflect(-m_LightDirection, finalNormal);
	const float cosa = std::max(0.0f, Vector3::Dot(reflect, -v.viewDirection));

	m_ObservedArea = { std::max(Vector3::Dot(finalNormal, -m_LightDirection), 0.0f) };

	const ColorRGB specularMapColour = m_SpecularMap->Sample(v.uv);
	const float glossMapValue = m_GlossMap->Sample(v.uv).r;

	const float phong = m_Ks * std::pow(cosa, glossMapValue * m_Shininess);

	const ColorRGB lambertDiffuse{ (m_Kd * m_Texture->Sample(v.uv)) / M_PI };

	ColorRGB ambient = { .025f,.025f,.025f };

	switch (m_ShadingMode)
	{
	case ShadingMode::ObservedArea:
		finalColour = ColorRGB{ m_ObservedArea, m_ObservedArea, m_ObservedArea };
		break;
	case ShadingMode::Diffuse:
		finalColour = lambertDiffuse * m_ObservedArea;
		break;
	case ShadingMode::Specular:
		finalColour = ColorRGB{phong, phong, phong};
		break;
	case ShadingMode::Combined:
		finalColour = (lambertDiffuse * m_ObservedArea + specularMapColour * phong + ambient );
		break;
	}

	//finalColour = {Remap01(finalNormal.x, -1,1),  Remap01(finalNormal.y, -1, 1), Remap01(finalNormal.z,-1,1) };

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

float Renderer::Remap01(float value, float start, float stop)
{
	return (value - start) / (stop - start);
}




