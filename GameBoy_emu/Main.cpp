#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "Emulator/Emulator.h"
#include <iostream>

constexpr int WIDTH_MULT{ 4 };
constexpr int HEIGHT_MULT{ 3 };
constexpr int WINDOW_WIDTH{ 160 * WIDTH_MULT };
constexpr int WINDOW_HEIGHT{ 144 * HEIGHT_MULT };

int GetKey(const SDL_Event& event) {
	int key = -1;
	switch (event.key.scancode) {
	case SDL_SCANCODE_RIGHT:
		key = 0;
		break;
	case SDL_SCANCODE_LEFT:
		key = 1;
		break;
	case SDL_SCANCODE_UP:
		key = 2;
		break;
	case SDL_SCANCODE_DOWN:
		key = 3;
		break;
	case SDL_SCANCODE_A:
		key = 4;
		break;
	case SDL_SCANCODE_S:
		key = 5;
		break;
	case SDL_SCANCODE_SPACE:
		key = 6;
		break;
	case SDL_SCANCODE_RETURN:
		key = 7;
		break;
	}

	return key;
}

void DrawGraphics(SDL_Renderer*& renderer, const Emulator& emu) {
	for (int x{ 0 }; x < 160; ++x)
		for (int y{ 0 }; y < 144; ++y) {
			BYTE r{ emu.m_ScreenData[x][y][0] };
			BYTE g{ emu.m_ScreenData[x][y][1] };
			BYTE b{ emu.m_ScreenData[x][y][2] };
			SDL_SetRenderDrawColor(renderer, r, g, b, SDL_ALPHA_OPAQUE);
			for (int yy{ y * HEIGHT_MULT }; yy < (y + 1) * HEIGHT_MULT; ++yy)
				for (int xx{ x * WIDTH_MULT }; xx < (x + 1) * WIDTH_MULT; ++xx) {
					SDL_RenderPoint(renderer, xx, yy);
				}
		}

	SDL_RenderPresent(renderer);
}


int main(int argc, char* argv[])
{
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Event event;

	Emulator emu{};

	SDL_SetAppMetadata("GameBoy Emulator", "1.0", "com.example.GameBoy_Emulator");

	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		SDL_Log("Couldn't initialize SDL: ", SDL_GetError());
		return 3;
	}

	if (!SDL_CreateWindowAndRenderer("kasper2918's GameBoy Emulator",
		WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer))
	{
		SDL_Log("Couldn't create window/renderer: ", SDL_GetError());
		return 3;
	}

	if (argc < 2) {
		std::cout << "Usage: .\\GameBoy_emu.exe [rom_path]";
		return 1;
	}
	emu.LoadGame(argv[1]);

	constexpr double fps{ 60 };
	constexpr double interval{ 1000 / fps };

	Uint64 time2{ SDL_GetTicks() };

	while (true)
	{
		SDL_PollEvent(&event);
		if (event.type == SDL_EVENT_KEY_DOWN) {
			int key{ GetKey(event) };
			if (key != -1)
				emu.KeyPressed(key);
		}
		else if (event.type == SDL_EVENT_KEY_UP) {
			int key{ GetKey(event) };
			if (key != -1)
				emu.KeyReleased(key);
		}
		else if (event.type == SDL_EVENT_QUIT)
			return 0;

		Uint64 current{ SDL_GetTicks() };
		if (time2 + interval < current) {
			emu.Update();
			DrawGraphics(renderer, emu);
			time2 = current;
		}
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();

	return 0;
}