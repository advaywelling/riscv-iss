#include "hart.h"
#include "elf.h"


int main() {
    Hart hart;
    if (!load_elf(hart, "tests/simple.elf")) {
        return 1;
    }
    while (hart.is_running()) {
        hart.cycle();
    }
}