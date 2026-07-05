#include "hart.h"
#include "elf.h"


int main() {
    Hart hart;
    if (!load_elf(hart, "tests/multiply.elf")) {
        return 1;
    }
    while (hart.is_running()) {
        hart.cycle();
    }
    hart.dump_regs();
}