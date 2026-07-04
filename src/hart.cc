#include "hart.h"
#include <iostream>

Hart::Hart() {
    pc = 0x0;
    write_word(0x0, 0x00500093);
    write_word(0x4, 0x00A00113);
    write_word(0x8, 0x002081B3);
    std::cout << std::hex << read_word(0x0) << "\n";
    std::cout << std::hex << read_word(0x4) << "\n";
    std::cout << std::hex << read_word(0x8) << "\n";
}

uint32_t Hart::read_word(uint32_t addr) const {
    uint32_t read_data {};
    for (int i{}; i < 4; i++) {
        read_data |= mem[addr + i] << i*8;
    }
    return read_data;
}

void Hart::write_word(uint32_t addr, uint32_t write_data) {
    for (int i{}; i < 4; i++) {
        mem[addr + i] = (write_data >> i * 8) & 0xFF;
    }
}

int main() {
    Hart hart;
}