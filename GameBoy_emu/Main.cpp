#include "Emulator/Emulator.h"
#include <memory>

int main() {
	auto emu{ std::make_unique<Emulator>() };
	emu->LoadGame("ROMS/rom.gb");

	return 0;
}