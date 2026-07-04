#pragma once
#include <cstdint>
#include <string>

class Hart {
    public:
        Hart();
        bool load_rom(const std::string& filename);
        void cycle();
        uint32_t get_pc() const { return pc; }
        uint32_t get_reg(int reg_num) const { return regs[reg_num]; }
        void dump_regs() const;
    private:
        static const uint32_t MEM_SIZE = 1024 * 1024;
        uint8_t mem[MEM_SIZE]{}; // byte addressable, 1 MB total
        uint32_t regs[32]{};
        uint32_t pc{};
        uint32_t read_word(uint32_t addr) const;
        void write_word(uint32_t addr, uint32_t write_data);
        void write_reg(uint32_t reg_num, uint32_t write_data);
};