#include "Emulator.h"

void Emulator::do_DMA_transfer(BYTE data) {
	WORD address = data << 8; // source address is data * 0x100
	for (int i = 0; i < 0xA0; i++) {
		write_memory(0xFE00 + i, read_memory(address + i));
	}
}