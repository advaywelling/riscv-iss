#include <iostream>
#include "hart.h"
#include "elf.h"

int main(int argc, char** argv) {
    const char* path = (argc > 1) ? argv[1] : "tests/multiply.elf";

    Hart hart;
    if (!load_elf(hart, path)) {
        return 1;
    }

    const uint64_t MAX_CYCLES = 10000000; // in case inf loop
    uint64_t cycles = 0;
    while (hart.is_running() && cycles < MAX_CYCLES) {
        hart.cycle();
        cycles++;
    }

    if (!hart.is_running()) {
        uint32_t code = hart.get_exit_code();
        if (code == 0) {
            std::cout << "PASS" << path << "\n";
            return 0;
        }
        std::cout << "FAIL" << path << "  test #" << (code >> 1)
                  << " (exit " << code << ")\n";
        return 1;
    }

    std::cout << "TIMEOUT" << path << "  pc=0x" << std::hex
              << hart.get_pc() << " after " << std::dec << cycles << " cycles\n";
    return 2;
}