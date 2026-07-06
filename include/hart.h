#pragma once
#include <cstdint>
#include <string>

class Hart {
    public:
        Hart();
        bool load_binary(const std::string& filename, uint32_t addr);
        void cycle();
        uint32_t get_pc() const { return pc; }
        uint32_t get_reg(int reg_num) const { return regs[reg_num]; }
        void dump_regs() const;
        bool is_running() const { return running; }
        void load_segment(uint32_t addr, const uint8_t* data, uint32_t size);
        void set_pc(uint32_t addr) { pc = addr; };
        uint32_t get_exit_code() const { return exit_code; }
    private:
        // vars
        static const uint32_t MEM_BASE   = 0x80000000;
        static const uint32_t MEM_SIZE = 1024 * 1024;
        static const uint32_t CSR_MEPC    = 0x341;
        static const uint32_t CSR_MHARTID = 0xF14;
        bool running = true;
        uint8_t mem[MEM_SIZE]{}; // byte addressable, 1 MB total
        uint32_t regs[32]{};
        uint32_t csr[4096]{};
        uint32_t pc{};
        uint32_t exit_code{};

        // functions
        uint32_t read_word(uint32_t addr) const;
        void write_word(uint32_t addr, uint32_t write_data);
        void write_reg(uint32_t reg_num, uint32_t write_data);
        uint8_t read_byte(uint32_t addr) const { return mem[addr - MEM_BASE]; }
        void write_byte(uint32_t addr, uint8_t v) { mem[addr - MEM_BASE] = v; }
        uint32_t read_csr(uint32_t addr) const;
        void write_csr(uint32_t addr, uint32_t val);
        void handle_ecall();
};