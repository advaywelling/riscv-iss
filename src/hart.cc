#include "hart.h"

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