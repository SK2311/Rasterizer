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

	Texture* Texture::LoadFromFile(const std::string& path)
	{
		//TODO
		//Load SDL_Surface using IMG_LOAD
		//Create & Return a new Texture Object (using SDL_Surface)
		Texture* pTexture = new Texture{ IMG_Load(path.c_str()) };
		return pTexture;
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		//TODO
		//Sample the correct texel for the given uv
		int x{ int(uv.x * m_pSurface->w) };
		int y{ int(uv.y * m_pSurface->h) };
		Vector2 pixel{ (float)x, (float)y };

		Uint8 r, g, b;

		SDL_GetRGB(m_pSurfacePixels[x + (y * m_pSurface->w)], m_pSurface->format, &r, &g, &b);

		ColorRGB finalColour{ r / 255.0f, g / 255.0f, b / 255.0f };
		return finalColour;
	}
}