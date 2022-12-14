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
		SDL_Color rgb{};
		//if (uv.x > 1  uv.x < -1)
		//    return {};
		//if (uv.y > 1  uv.y < -1)
		//    return {};

		//Vector2 uvNormalized = uv.Normalized();
		Uint32 u = uv.x * m_pSurface->w;
		Uint32 v = uv.y * m_pSurface->h;

		//Sample the correct data for the given uv
		Uint32 index{ u + v * static_cast<Uint32>(m_pSurface->w) };
		Uint32 p = m_pSurfacePixels[index];
		SDL_GetRGB(p, m_pSurface->format, &rgb.r, &rgb.g, &rgb.b);

		//change color from range 0,255 to 0,1
		ColorRGB rgb2{ rgb.r, rgb.g, rgb.b };
		return rgb2 / 255.0f;
	}
}