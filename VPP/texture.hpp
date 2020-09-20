#pragma once

#include <vector>
#include <SDL2/SDL.h>

class Texture
{
public:
	uint32_t height;
	uint32_t width;
	uint32_t pitch;
	std::vector<float> pixels;
	Texture() : height(0), width(0), pitch(0), pixels() {}
	Texture(const char* filename)
	{
		auto surf = SDL_LoadBMP(filename);
		if (surf == nullptr)
		{
			return;
		}
		pitch = surf->pitch;
		height = surf->h;
		width = surf->w;
		pixels = std::vector<float>(height * width * 4u);
		float temp[] = { 0.f, 0.f, 0.f, 1.f };
		uint8_t colorSize = pitch / width;

		uint8_t* data = (uint8_t*)(surf->pixels);
		for (unsigned int i = 0; i < height; i++)
		{
			for (unsigned int j = 0; j < width; j++)
			{
				for (unsigned int k = 0; k < colorSize; k++)
				{
					temp[k] = data[i * pitch + j * colorSize + k] * 1.f / 255u;
				}
				for (unsigned int k = 0; k < 4u; k++)
				{
					pixels[i * pitch + j * 4u + k] = temp[k];
				}
			}
		}
		SDL_FreeSurface(surf);
	}
	~Texture()
	{
		pixels.clear();
	}
};