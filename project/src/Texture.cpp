#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>

namespace dae
{
	Texture::Texture(SDL_Surface* pSurface) :
		m_pSurface{ pSurface },
		m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
	{
	}

	Texture::~Texture()
	{
		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}
	}

	std::unique_ptr<Texture> Texture::LoadFromFile(const std::string& path)
	{
		SDL_Surface* pSurface = IMG_Load(path.c_str());

		if (pSurface == nullptr)
		{
			return nullptr;
		}

		return std::unique_ptr<Texture>(new Texture(pSurface));
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		int convertedWidth = int(uv.x * m_pSurface->w);
		int convertedHeight = int(uv.y * m_pSurface->h);

		int indexOfPixel = convertedHeight * m_pSurface->w + convertedWidth;

		Uint32 pixel = ((Uint32*)m_pSurface->pixels)[indexOfPixel];

		Uint8 r;
		Uint8 g;
		Uint8 b;

		SDL_GetRGB(pixel, m_pSurface->format, &r, &g, &b);

		return ColorRGB{ r / 255.0f, g / 255.0f, b / 255.0f };
	}
}