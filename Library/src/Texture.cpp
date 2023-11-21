#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>
#include <iostream>
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
		SDL_Surface* surfaceBuffer{ IMG_Load(path.c_str())};

		if (surfaceBuffer == nullptr)
		{
			std::cout << "Image could not be loaded in Texture.cpp ->LoadFromFIle: " << path.c_str() << std::endl;
			return nullptr;
		}
		else
		{
			std::cout << "Image: " << path.c_str() << "successfully loaded" << std::endl;
		}
		//m_pSurfacePixels = (uint32_t*)surfaceBuffer->pixels;
		//Texture* outputTexture = new Texture{ surfaceBuffer }; //integrated in the return

		//Create & Return a new Texture Object (using SDL_Surface)
		return new Texture{ surfaceBuffer };

		
		//here for debugging, should never be hit
		return nullptr;
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		//TODO
		//Sample the correct texel for the given uv
		const int width{ m_pSurface->w };
		const int height{ m_pSurface->h };
		const float aspecRatio{ static_cast<float>(width) / static_cast<float>(height) };

		const int scaledU{ static_cast<int>(uv.x * width) };
		const int scaledV{ static_cast<int>(uv.y * height) };

		uint32_t m_pSurfacePixel = m_pSurfacePixels[scaledU + (scaledV * width)];
		
		//somehow didnt work when initialising as Uint8* r{}, g{}, b{};
		Uint8* r{ new Uint8{} };
		Uint8* g{ new Uint8{} };
		Uint8* b{ new Uint8{} };


		SDL_GetRGB(m_pSurfacePixel, m_pSurface->format, r, g, b);

		//for reference
		//m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
		//	static_cast<uint8_t>(finalColor.r * 255),
		//	static_cast<uint8_t>(finalColor.g * 255),
		//	static_cast<uint8_t>(finalColor.b * 255));



		ColorRGB outputColor{};

		if (r && g && b)
		{

			outputColor = { //has to reverse?
				//normal
				static_cast<float>(*r) / 255.f,
				static_cast<float>(*g) / 255.f,
				static_cast<float>(*b) / 255.f,

				////reversed
				//static_cast<float>(*b) / 255.f,
				//static_cast<float>(*g) / 255.f,
				//static_cast<float>(*r) / 255.f,

			};
		};




		return outputColor;

		//debug reason, should never hit
		return ColorRGB{ 0, 0, 0, };
	}
}