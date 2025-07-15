#include "CPU/Emulator.h"

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

SDL_Renderer* renderer;
SDL_Window* window;
Emulator emu;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
	SDL_SetAppMetadata("GameBoy Emulator", "1.0", "com.example.GameBoy_Emulator");
	
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (!SDL_CreateWindowAndRenderer("GameBoy Emulator",
		WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer))
	{
		SDL_Log("Couldn't initialize window/renderer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	emu.initialize();
	emu.load_game("ROMS/TetrisJ.gb");

	return SDL_APP_CONTINUE;
}

int get_key(SDL_Event*& event) {
	int key = -1;
	switch (event->key.scancode) {
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

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
	if (event->type == SDL_EVENT_KEY_DOWN) {
		int key{ get_key(event) };
		if (key != -1)
			emu.key_pressed(key);
	}
	else if (event->type == SDL_EVENT_KEY_UP) {
		int key{ get_key(event) };
		if (key != -1)
			emu.key_released(key);
	}
	else if (event->type == SDL_EVENT_QUIT)
		return SDL_APP_SUCCESS;

	return SDL_APP_CONTINUE;
}

void draw_graphics(SDL_Renderer*& renderer) {
	for (int x{0}; x < 160; ++x)
		for (int y{ 0 }; y < 144; ++y) {
			BYTE r{ emu.m_screen_data[x][y][0] };
			BYTE g{ emu.m_screen_data[x][y][1] };
			BYTE b{ emu.m_screen_data[x][y][2] };
			SDL_SetRenderDrawColor(renderer, r, g, b, SDL_ALPHA_OPAQUE);
			for (int yy{ y * HEIGHT_MULTIPLIER }; yy < (y + 1) * HEIGHT_MULTIPLIER; ++yy)
				for (int xx{ x * WIDTH_MULTIPLIER }; xx < (x + 1) * WIDTH_MULTIPLIER; ++xx) {
					SDL_RenderPoint(renderer, xx, yy);
				}
		}

	SDL_RenderPresent(renderer);
}

constexpr double fps{ 59.73 };
constexpr double interval{ 1000 / fps };

Uint64 time2{ SDL_GetTicks() };

SDL_AppResult SDL_AppIterate(void* appstate) {
	Uint64 current{ SDL_GetTicks() };
	if (time2 + interval < current) {
		emu.update();
		draw_graphics(renderer);
		time2 = current;
	}

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {

}